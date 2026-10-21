#include "abc/core.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
int cli(const std::vector<std::string>& arguments) {
    if (arguments.size() == 2 && arguments[1] == "--version") {
        std::cout << "ABC Native 0.1.0-dev (Pascal prototype)\n";
        return 0;
    }
    if (arguments.size() < 3 || arguments.size() > 4 || (arguments[1] != "check" && arguments[1] != "run")) {
        std::cerr << "Usage: abc check|run file.pas [--max-instructions=N]\n";
        return 2;
    }
    const auto& filename = arguments[2];
    try {
        abc::ExecutionOptions options;
        if (arguments.size() == 4) {
            auto option = std::string_view(arguments[3]);
            if (!option.starts_with("--max-instructions=")) throw std::invalid_argument("Unknown option");
            option.remove_prefix(19);
            const auto result = std::from_chars(option.data(), option.data() + option.size(), options.instruction_limit);
            if (result.ec != std::errc{} || result.ptr != option.data() + option.size() || options.instruction_limit == 0)
                throw std::invalid_argument("Instruction limit must be a positive integer");
        }
        const std::u8string utf8_path(filename.begin(), filename.end());
        std::ifstream stream(std::filesystem::path(utf8_path), std::ios::binary);
        if (!stream) { std::cerr << filename << ": E_IO: Cannot open source file\n"; return 2; }
        std::string source;
        char buffer[8192];
        while (stream.read(buffer, sizeof buffer) || stream.gcount()) {
            source.append(buffer, static_cast<std::size_t>(stream.gcount()));
            if (source.size() > 1'048'576) throw abc::Error("E_LIMIT", {}, "Source exceeds 1 MiB prototype limit");
        }
        if (stream.bad()) { std::cerr << filename << ": E_IO: Cannot read source file\n"; return 2; }
        const auto program = abc::compile(source);
        if (arguments[1] == "run") abc::execute(program, std::cin, std::cout, options);
        else std::cout << "OK: " << filename << '\n';
        return 0;
    } catch (const abc::Error& error) {
        std::cerr << abc::format_diagnostic(error, filename);
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "abc: " << error.what() << '\n';
        return 2;
    }
}
} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    // Preserve explicit CRLF inside strings instead of the CRT producing CRCRLF.
    _setmode(_fileno(stdout),_O_BINARY);
    std::vector<std::string> arguments;
    for (int i = 0; i < argc; ++i) {
        std::u16string value;
        for (const wchar_t* ch = argv[i]; *ch; ++ch) value.push_back(static_cast<char16_t>(*ch));
        arguments.push_back(abc::to_utf8(value));
    }
    return cli(arguments);
}
#else
int main(int argc, char** argv) {
    return cli({argv, argv + argc});
}
#endif
