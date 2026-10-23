#include "text_file.hpp"
#include "files.hpp"
#include "diagnostics.hpp"
#include "stdlib.hpp"
#include <algorithm>
#include <charconv>
#include <limits>

namespace abc {
namespace {
constexpr std::size_t file_limit=1'048'576;
bool whitespace(char16_t c) {
    return c==u' ' || (c>=9 && c<=13) || c==0x85 || c==0xa0 || c==0x1680
        || (c>=0x2000 && c<=0x200a) || c==0x2028 || c==0x2029 || c==0x202f || c==0x205f || c==0x3000;
}
void assigned(const TextFile& file, SourceLocation location) {
    if (file.filename.empty()) throw runtime_error(RuntimeError::file_unassigned,location);
}
void readable(const TextFile& file, SourceLocation location) {
    assigned(file,location);
    if (!file.reading) throw runtime_error(RuntimeError::file_read_mode,location);
}
std::u16string read_remaining_line(TextFile& file, SourceLocation location) {
    readable(file,location);
    const auto start=file.cursor;
    while (file.cursor<file.content.size() && file.content[file.cursor]!=u'\r' && file.content[file.cursor]!=u'\n') ++file.cursor;
    return file.content.substr(start,file.cursor-start);
}
std::u16string read_token(TextFile& file, SourceLocation location) {
    readable(file,location);
    while (file.cursor<file.content.size() && whitespace(file.content[file.cursor])) ++file.cursor;
    if (file.cursor==file.content.size()) throw runtime_error(RuntimeError::file_end,location);
    const auto start=file.cursor;
    while (file.cursor<file.content.size() && !whitespace(file.content[file.cursor])) ++file.cursor;
    return file.content.substr(start,file.cursor-start);
}
}

TextFile::~TextFile() {
    try { if (writer.is_open()) { flush({}); writer.close(); } } catch (...) {}
}
TextFile& text_file(const Value& value, SourceLocation location) {
    const auto& file=std::get<std::shared_ptr<TextFile>>(value);
    if (!file) throw runtime_error(RuntimeError::nil_reference,location);
    return *file;
}
void FileContext::prepare_text_open(const std::shared_ptr<TextFile>& file, SourceLocation location) {
    if (file->is_open()) return;
    std::erase_if(open_texts_,[](const auto& weak) { const auto file=weak.lock(); return !file || !file->is_open(); });
    if (open_texts_.size()>=64) throw runtime_error(RuntimeError::file_open_limit,location);
    open_texts_.push_back(file);
}
void TextFile::flush(SourceLocation location) {
    if (!writer.is_open()) return;
    const auto bytes=to_utf8(pending,true);
    if (position>file_limit || bytes.size()>file_limit-position) throw runtime_error(RuntimeError::file_limit,location);
    writer.write(bytes.data(),static_cast<std::streamsize>(bytes.size())); writer.flush();
    if (!writer) throw runtime_error(RuntimeError::file_io,location);
    position+=bytes.size(); size=std::max(size,position); pending.clear();
}
void TextFile::close(SourceLocation location) {
    assigned(*this,location);
    if (!is_open()) throw runtime_error(RuntimeError::file_closed,location);
    if (writer.is_open()) {
        flush(location); writer.close();
        if (writer.fail()) throw runtime_error(RuntimeError::file_io,location);
    }
    reading=false; std::u16string{}.swap(content); cursor=0;
}
void text_assign(FileContext& files, TextFile& file, const Value& name, SourceLocation location) {
    file.filename=files.resolve(string_text(name),location);
}
void text_open(FileContext& files, const std::shared_ptr<TextFile>& pointer, unsigned mode, SourceLocation location) {
    auto& file=text_file(Value{pointer},location); assigned(file,location);
    files.prepare_text_open(pointer,location);
    if (mode==1 && file.writer.is_open()) {
        // Original Rewrite seeks the underlying writer without discarding its buffer.
        file.writer.seekp(0); if (!file.writer) throw runtime_error(RuntimeError::file_io,location);
        file.position=0; return;
    }
    if (mode==2 && file.writer.is_open()) throw runtime_error(RuntimeError::file_io,location);
    if (file.is_open()) file.close(location);
    if (mode==0) {
        file.content=read_text_file(files,file.filename.u16string(),location);
        file.cursor=0; file.reading=true; return;
    }
    std::error_code error;
    const auto existing=std::filesystem::file_size(file.filename,error);
    if (mode==2 && !error && existing>file_limit) throw runtime_error(RuntimeError::file_limit,location);
    if (mode==1) {
        file.writer.open(file.filename,std::ios::binary|std::ios::out|std::ios::trunc);
        file.position=0; file.size=0;
    } else {
        // Ensure append creates an absent file, then retain an explicitly seekable writer.
        { std::ofstream create(file.filename,std::ios::binary|std::ios::app); if (!create) throw runtime_error(RuntimeError::file_io,location); }
        file.writer.open(file.filename,std::ios::binary|std::ios::in|std::ios::out);
        file.writer.seekp(0,std::ios::end);
        if (file.writer) file.position=file.size=static_cast<std::size_t>(file.writer.tellp());
    }
    if (!file.writer) { file.writer.close(); file.writer.clear(); throw runtime_error(RuntimeError::file_io,location); }
    file.pending.clear();
}
void text_write(TextFile& file, std::u16string_view value, SourceLocation location) {
    assigned(file,location);
    if (!file.writer.is_open()) throw runtime_error(RuntimeError::file_write_mode,location);
    if (value.size()>file_limit || file.pending.size()>file_limit-value.size()) throw runtime_error(RuntimeError::file_limit,location);
    auto next=file.pending; next+=value;
    const auto bytes=to_utf8(next,true);
    if (file.position>file_limit || bytes.size()>file_limit-file.position) throw runtime_error(RuntimeError::file_limit,location);
    std::size_t offset=0;
    while (next.size()-offset>=1024) {
        std::size_t count=1024;
        if (next[offset+count-1]>=0xd800 && next[offset+count-1]<=0xdbff) --count;
        const auto chunk=to_utf8(std::u16string_view(next).substr(offset,count),true);
        file.writer.write(chunk.data(),static_cast<std::streamsize>(chunk.size()));
        if (!file.writer) throw runtime_error(RuntimeError::file_io,location);
        file.position+=chunk.size(); file.size=std::max(file.size,file.position); offset+=count;
    }
    file.pending=next.substr(offset);
}
Value text_read(TextFile& file, Type type, SourceLocation location) {
    if (type==Type::string) return read_remaining_line(file,location);
    auto token=read_token(file,location);
    if (type==Type::boolean) {
        for (auto& c:token) if (c>=u'A' && c<=u'Z') c=static_cast<char16_t>(c+32);
        if (token==u"true") return true;
        if (token==u"false") return false;
        throw runtime_error(RuntimeError::number_text,location,"boolean");
    }
    if (type==Type::real) {
        const Value argument=std::move(token);
        return *stdlib::invoke_text(stdlib::Function::str_to_real,std::span<const Value>(&argument,1),location);
    }
    while (!token.empty() && token.back()==0) token.pop_back();
    auto text=std::u16string_view(token);
    if (!text.empty() && (text.front()==u'+' || text.front()==u'-')) text.remove_prefix(1);
    if (text.empty() || std::any_of(text.begin(),text.end(),[](char16_t c){ return c<u'0' || c>u'9'; }))
        throw runtime_error(RuntimeError::number_text,location,type_name(type));
    auto bytes=to_utf8(token);
    auto view=std::string_view(bytes); if (view.starts_with('+')) view.remove_prefix(1);
    std::int64_t value=0;
    const auto converted=std::from_chars(view.data(),view.data()+view.size(),value);
    if (converted.ec!=std::errc{} || (type==Type::integer && (value<INT32_MIN || value>INT32_MAX)))
        throw runtime_error(RuntimeError::number_range,location,type_name(type));
    return type==Type::integer ? Value{static_cast<std::int32_t>(value)} : Value{value};
}
std::u16string text_word(TextFile& file, SourceLocation location) { return read_token(file,location); }
std::u16string text_name(const TextFile& file, bool full, SourceLocation location) {
    if (file.filename.empty()) throw runtime_error(RuntimeError::nil_reference,location);
    try { return (full ? file.filename : file.filename.filename()).u16string(); }
    catch (const std::filesystem::filesystem_error&) { throw runtime_error(RuntimeError::file_io,location); }
}
std::u16string text_read_end(TextFile& file, SourceLocation location) {
    if (!file.reading) throw runtime_error(RuntimeError::nil_reference,location);
    auto result=file.content.substr(file.cursor); file.cursor=file.content.size(); return result;
}
void text_skip_line(TextFile& file, SourceLocation location) {
    (void)read_remaining_line(file,location);
    if (file.cursor<file.content.size()) {
        const auto c=file.content[file.cursor++];
        if (c==u'\r' && file.cursor<file.content.size() && file.content[file.cursor]==u'\n') ++file.cursor;
    }
}
bool text_eof(TextFile& file, bool line, bool skip_space, SourceLocation location) {
    if (!file.is_open()) throw runtime_error(RuntimeError::file_closed,location);
    readable(file,location);
    if (skip_space) while (file.cursor<file.content.size()
        && (line ? file.content[file.cursor]==u' ' || file.content[file.cursor]==u'\t' : whitespace(file.content[file.cursor]))) ++file.cursor;
    return file.cursor==file.content.size() || (line && (file.content[file.cursor]==u'\r' || file.content[file.cursor]==u'\n'));
}
} // namespace abc
