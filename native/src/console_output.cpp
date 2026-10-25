#include "console_output.hpp"
#include "diagnostics.hpp"

namespace abc {
ConsoleOutput::~ConsoleOutput() {
    try { finish(); } catch (...) { } // Preserve the original execution error during cleanup.
}
void ConsoleOutput::emit(std::string_view bytes, SourceLocation location) {
    if (bytes.size()>limit_-used_) throw runtime_error(RuntimeError::output_limit,location);
    try {
        if (!bytes.empty()) output_.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));
        output_.flush();
    } catch (const std::ios_base::failure&) { throw runtime_error(RuntimeError::output_stream,location); }
    if (!output_) throw runtime_error(RuntimeError::output_stream,location);
    used_+=bytes.size();
}
void ConsoleOutput::write(std::u16string_view text, SourceLocation location) {
    if (text.empty()) { emit({},location); return; }
    std::u16string combined;
    combined.reserve(text.size()+(pending_ ? 1U : 0U));
    if (pending_) combined+=pending_;
    combined+=text;
    const auto last=combined.back();
    const char16_t next=last>=0xd800 && last<=0xdbff ? last : char16_t{0};
    if (next) combined.pop_back();
    const auto bytes=to_utf8(combined,true);
    emit(bytes,location);
    pending_=next; pending_location_=location;
}
void ConsoleOutput::finish() {
    if (!pending_) return;
    emit("\xef\xbf\xbd",pending_location_);
    pending_=0;
}
} // namespace abc
