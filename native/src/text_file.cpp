#include "text_file.hpp"
#include "big_integer_runtime.hpp"
#include "files.hpp"
#include "encoding.hpp"
#include "file_reader.hpp"
#include "diagnostics.hpp"
#include "stdlib.hpp"
#include "integer_values.hpp"
#include <algorithm>
#include <charconv>
#include <limits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace abc {
namespace {
constexpr std::size_t file_limit=1'048'576;
bool whitespace(char16_t c) {
    return c==u' ' || (c>=9 && c<=13) || c==0x85 || c==0xa0 || c==0x1680
        || (c>=0x2000 && c<=0x200a) || c==0x2028 || c==0x2029 || c==0x202f || c==0x205f || c==0x3000;
}
std::string encoded(const TextFile& file,TextEncoder& encoder,std::u16string_view text,bool final,SourceLocation location) {
    auto bytes=encoder.encode(text,final,location);
    if(file.preamble_pending && file.encoding)bytes.insert(0,text_encoding_preamble(*file.encoding));
    return bytes;
}
void emit_pending(TextFile& file,bool final,SourceLocation location) {
    if(!file.encoder)throw runtime_error(RuntimeError::invalid_bytecode,location);
    auto next=*file.encoder;
    const auto bytes=encoded(file,next,file.pending,final,location);
    file.writer->write(bytes,location);
    file.pending.clear();file.encoder=next;file.preamble_pending=false;
}
void assigned(const TextFile& file, SourceLocation location) {
    if (file.filename.empty()) throw runtime_error(RuntimeError::file_unassigned,location);
}
void readable(const TextFile& file, SourceLocation location) {
    assigned(file,location);
    if (!file.reader) throw runtime_error(RuntimeError::file_read_mode,location);
}
std::u16string read_remaining_line(TextFile& file,SourceLocation location) {
    readable(file,location);std::u16string result;
    while(const auto next=file.reader->peek_character(location,file.options())) {
        if(*next==u'\r' || *next==u'\n')break;
        result.push_back(*file.reader->read_character(location,file.options()));
    }
    return result;
}
std::u16string read_token(TextFile& file,SourceLocation location) {
    readable(file,location);
    auto next=file.reader->peek_character(location,file.options());
    while(next && whitespace(*next)){(void)file.reader->read_character(location,file.options());next=file.reader->peek_character(location,file.options());}
    if(!next)throw runtime_error(RuntimeError::file_end,location);
    std::u16string result;
    while(next && !whitespace(*next)) {
        result.push_back(*file.reader->read_character(location,file.options()));
        next=file.reader->peek_character(location,file.options());
    }
    return result;
}
}

bool TextFile::is_open() const{return (reader && reader->is_open()) || (writer && writer->is_open());}
const ExecutionOptions& TextFile::options() const{static const ExecutionOptions defaults;return execution_options ? *execution_options : defaults;}
TextFile::~TextFile() {
    try { if(writer && writer->is_open())flush({}); } catch (...) {}
    if(writer)writer->close();
    if(reader)reader->close();
}
TextFile& text_file(const Value& value, SourceLocation location) {
    const auto& file=std::get<std::shared_ptr<TextFile>>(value);
    if (!file) throw runtime_error(RuntimeError::nil_reference,location);
    return *file;
}
void FileContext::prepare_text_open(const std::shared_ptr<TextFile>& file, SourceLocation location) {
    if (file->is_open()) return;
    std::erase_if(open_texts_,[](const auto& weak) { const auto file=weak.lock(); return !file || !file->is_open(); });
    std::erase_if(open_readers_,[](const auto& weak){const auto reader=weak.lock();return !reader || !reader->is_open();});
    if (open_texts_.size()+open_readers_.size()>=64) throw runtime_error(RuntimeError::file_open_limit,location);
    open_texts_.push_back(file);
}
bool FileContext::text_file_locked(const std::filesystem::path& path, bool subtree) const {
#ifdef _WIN32
    const auto locked=[&](std::filesystem::path cursor) {
        while(!cursor.empty()) {
            if(cursor==path)return true;
            std::error_code error;
            if(std::filesystem::equivalent(cursor,path,error) && !error)return true;
            const auto parent=cursor.parent_path();if(!subtree || parent==cursor)break;cursor=parent;
        }
        return false;
    };
    for(const auto& weak:open_texts_)if(const auto file=weak.lock();file && ((file->reader && file->reader->is_open() && locked(file->reader->path())) || (file->writer && file->writer->is_open() && locked(file->writer->path()))))return true;
    for(const auto& weak:open_readers_)if(const auto reader=weak.lock();reader && reader->is_open() && locked(reader->path()))return true;
#else
    (void)path;(void)subtree;
#endif
    return false;
}
void TextFile::flush(SourceLocation location) {
    if(!writer)return;
    emit_pending(*this,true,location);writer->flush(location);
}
void TextFile::close(SourceLocation location) {
    assigned(*this,location);
    if(!reader && !writer)throw runtime_error(RuntimeError::file_closed,location);
    if(reader) {
        reader->close();reader.reset();
        // Source Close takes the reader branch and drops an accompanying writer without flushing its character buffer.
        if(writer){writer->close();writer.reset();}
    } else if(writer) {
        try{flush(location);}catch(...){writer->close();throw;}
        writer->close();writer.reset();
    }
    pending.clear();encoder.reset();preamble_pending=false;
}
void text_assign(FileContext& files, TextFile& file, const Value& name, SourceLocation location) {
    file.filename=files.resolve(string_text(name),location);
}
void text_open(FileContext& files,const std::shared_ptr<TextFile>& pointer,unsigned mode,SourceLocation location,const Value* explicit_encoding) {
    auto& file=text_file(Value{pointer},location);assigned(file,location);
    files.prepare_text_open(pointer,location);
    if(mode==1 && file.writer) {
        // Rewrite seeks the byte stream without flushing the character buffer or changing its codec.
        file.writer->seek_begin(location);return;
    }
    if(mode==0 && file.reader)file.reader->close();
    const auto selected=explicit_encoding ? &encoding_value(*explicit_encoding,location) : files.encoding(location);
    const auto snapshot=selected ? std::optional<TextEncoding>{*selected} : std::nullopt;
    file.execution_options=files.execution_options;
    if(mode==0) {
        auto next=std::make_shared<LineReader>(file.filename,snapshot.value_or(TextEncoding{TextEncodingKind::utf8,true,true}),location);
        const bool had_reader=static_cast<bool>(file.reader);file.reader=std::move(next);
        if(!had_reader && file.writer) {
            try{file.flush(location);}catch(...){file.writer->close();throw;}
            file.writer->close();file.writer.reset();file.pending.clear();file.encoder.reset();
        }
        return;
    }
    auto next=std::make_shared<FileWriter>(file.filename,mode==2,location);
    // Open first: a conflicting reader/writer must stay intact if the new open fails.
    if(mode==1 && file.reader){file.reader->close();file.reader.reset();}
    if(file.writer)file.writer->close();
    file.writer=std::move(next);file.encoding=snapshot;
    file.preamble_pending=file.writer->position()==0 && snapshot && !text_encoding_preamble(*snapshot).empty();
    file.pending.clear();file.encoder.emplace(snapshot.value_or(TextEncoding{TextEncodingKind::utf8,false,false}));
}
void text_erase(const FileContext& files, const TextFile& file, SourceLocation location) {
    assigned(file,location);
    if (files.text_file_locked(file.filename)) throw runtime_error(RuntimeError::file_erase,location);
#ifdef _WIN32
    if (::DeleteFileW(file.filename.c_str()) || ::GetLastError()==ERROR_FILE_NOT_FOUND) return;
#else
    if (::unlink(file.filename.c_str())==0 || errno==ENOENT) return;
#endif
    throw runtime_error(RuntimeError::file_erase,location);
}
void text_rename(FileContext& files, const TextFile& file, const Value& name, SourceLocation location) {
    assigned(file,location);
    if (!files.rename(file.filename.u16string(),string_text(name),false)) throw runtime_error(RuntimeError::file_rename,location);
    // PABCSystem calls File.Move rather than FileInfo.MoveTo: keep the assigned name.
}
void text_write(TextFile& file,std::u16string_view value,SourceLocation location) {
    assigned(file,location);
    if(!file.writer)throw runtime_error(RuntimeError::file_write_mode,location);
    if(!file.writer->is_open())throw runtime_error(RuntimeError::file_io,location);
    if(value.size()>file_limit || file.pending.size()>file_limit-value.size())throw runtime_error(RuntimeError::file_limit,location);
    if(!file.encoder)throw runtime_error(RuntimeError::invalid_bytecode,location);
    std::u16string check=file.pending;check+=value;auto preview=*file.encoder;
    const auto bytes=encoded(file,preview,check,true,location);
    if(file.writer->position()>file_limit || bytes.size()>file_limit-file.writer->position())throw runtime_error(RuntimeError::file_limit,location);
    while(!value.empty()) {
        if(file.pending.size()==1024)emit_pending(file,false,location);
        const auto count=std::min<std::size_t>(1024-file.pending.size(),value.size());
        file.pending.append(value.substr(0,count));value.remove_prefix(count);
    }
}
Value text_read(TextFile& file, Type type, SourceLocation location) {
    if (type==Type::string) return read_remaining_line(file,location);
    if (type==Type::character) {
        readable(file,location);
        const auto next=file.reader->read_character(location,file.options());
        return next ? *next : char16_t{65535}; // StreamReader.Read() returns -1, cast to char by PABCSystem.
    }
    auto token=read_token(file,location);
    if (type==Type::big_integer) return parse_big(token,location);
    if (type==Type::boolean) {
        for (auto& c:token) if (c>=u'A' && c<=u'Z') c=static_cast<char16_t>(c+32);
        if (token==u"true") return true;
        if (token==u"false") return false;
        throw runtime_error(RuntimeError::number_text,location,"boolean");
    }
    if (type==Type::real || type==Type::single) {
        const Value argument=std::move(token);
        return *stdlib::invoke_text(type==Type::single ? stdlib::Function::str_to_single : stdlib::Function::str_to_real,std::span<const Value>(&argument,1),location);
    }
    while (!token.empty() && token.back()==0) token.pop_back();
    auto text=std::u16string_view(token);
    if (!text.empty() && (text.front()==u'+' || text.front()==u'-')) text.remove_prefix(1);
    if (text.empty() || std::any_of(text.begin(),text.end(),[](char16_t c){ return c<u'0' || c>u'9'; }))
        throw runtime_error(RuntimeError::number_text,location,type_name(type));
    auto bytes=to_utf8(token);
    auto view=std::string_view(bytes); if (view.starts_with('+')) view.remove_prefix(1);
    if (wide_unsigned(type)) {
        std::uint64_t value=0;
        const auto converted=std::from_chars(view.data(),view.data()+view.size(),value);
        if (converted.ec!=std::errc{} || converted.ptr!=view.data()+view.size()) throw runtime_error(RuntimeError::number_range,location,type_name(type));
        return checked_unsigned_integer(type,Value{value},location);
    }
    std::int64_t value=0;
    const auto converted=std::from_chars(view.data(),view.data()+view.size(),value);
    if (converted.ec!=std::errc{} || (type==Type::integer && (value<INT32_MIN || value>INT32_MAX)))
        throw runtime_error(RuntimeError::number_range,location,type_name(type));
    if (small_integer(type)) return checked_small_integer(type,value,location);
    return type==Type::integer ? Value{static_cast<std::int32_t>(value)} : Value{value};
}
std::u16string text_word(TextFile& file, SourceLocation location) { return read_token(file,location); }
std::u16string text_name(const TextFile& file, bool full, SourceLocation location) {
    if (file.filename.empty()) throw runtime_error(RuntimeError::nil_reference,location);
    try { return (full ? file.filename : file.filename.filename()).u16string(); }
    catch (const std::filesystem::filesystem_error&) { throw runtime_error(RuntimeError::file_io,location); }
}
std::u16string text_read_end(TextFile& file,SourceLocation location) {
    if(!file.reader)throw runtime_error(RuntimeError::nil_reference,location);
    return file.reader->read_to_end(location,file.options());
}
void text_skip_line(TextFile& file,SourceLocation location) {
    (void)read_remaining_line(file,location);
    auto next=file.reader->read_character(location,file.options());
    if(next && *next==u'\r') {
        const auto following=file.reader->peek_character(location,file.options());
        if(following && *following==u'\n')(void)file.reader->read_character(location,file.options());
    }
}
bool text_eof(TextFile& file,bool line,bool skip_space,SourceLocation location) {
    if(skip_space && file.writer)throw runtime_error(RuntimeError::file_read_mode,location);
    if(!file.reader)throw runtime_error(file.writer ? RuntimeError::file_read_mode : RuntimeError::file_closed,location);
    auto next=file.reader->peek_character(location,file.options());
    while(skip_space && next && (line ? *next==u' ' || *next==u'\t' : whitespace(*next))) {
        (void)file.reader->read_character(location,file.options());next=file.reader->peek_character(location,file.options());
    }
    return !next || (line && (*next==u'\r' || *next==u'\n'));
}
} // namespace abc
