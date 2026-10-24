#include "file_operations.hpp"
#include "text_file.hpp"
#include "diagnostics.hpp"

namespace abc {
namespace {
constexpr std::u16string_view newline=
#ifdef _WIN32
    u"\r\n";
#else
    u"\n";
#endif
constexpr std::size_t string_limit=1'048'576;
}

std::u16string file_operations_read(FileContext& files, const Value& name, SourceLocation location) {
    auto file=std::make_shared<TextFile>();
    text_assign(files,*file,name,location); text_open(files,file,0,location);
    std::u16string result;
    while (!text_eof(*file,false,false,location)) {
        const auto value=text_read(*file,Type::string,location);
        const auto& line=string_text(value);
        if (line.size()>string_limit-newline.size() || result.size()>string_limit-newline.size()-line.size())
            throw runtime_error(RuntimeError::string_limit,location);
        result+=line; result+=newline;
        text_skip_line(*file,location);
    }
    file->close(location);
    return result;
}

void file_operations_write(FileContext& files, const Value& name, const Value& text, SourceLocation location) {
    auto file=std::make_shared<TextFile>();
    text_assign(files,*file,name,location); text_open(files,file,1,location);
    // The source calls Write(Text,object), whose ObjectToString(nil) is "nil".
    text_write(*file,std::holds_alternative<NilString>(text) ? u"nil" : std::u16string_view(string_text(text)),location);
    file->close(location);
}
} // namespace abc
