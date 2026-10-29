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
int command_error(std::string_view filename,std::string code,std::string english,std::string russian) {
    std::cerr << abc::format_diagnostic(abc::Error(std::move(code),{},std::move(english),std::move(russian)),filename);
    return 2;
}
int cli(const std::vector<std::string>& arguments) {
    if (arguments.size() == 2 && arguments[1] == "--version") {
        std::cout << "ABC Native 0.1.0-dev (Pascal prototype)\n";
        return 0;
    }
    if (arguments.size() < 3 || arguments.size() > 4 || (arguments[1] != "check" && arguments[1] != "run")) {
        std::cerr << "EN: Usage: abc check|run file.pas [--max-instructions=N]\n"
            << "RU: Использование: abc check|run file.pas [--max-instructions=N]\n";
        return 2;
    }
    const auto& filename = arguments[2];
    try {
        abc::ExecutionOptions options;
        if (arguments.size() == 4) {
            auto option = std::string_view(arguments[3]);
            if (!option.starts_with("--max-instructions="))
                return command_error(filename,"E_OPTION","Unknown option","Неизвестный параметр командной строки");
            option.remove_prefix(19);
            const auto result = std::from_chars(option.data(), option.data() + option.size(), options.instruction_limit);
            if (result.ec != std::errc{} || result.ptr != option.data() + option.size() || options.instruction_limit == 0)
                return command_error(filename,"E_OPTION","Instruction limit must be a positive integer","Предел числа инструкций должен быть положительным целым числом");
        }
        const std::u8string utf8_path(filename.begin(), filename.end());
        std::ifstream stream(std::filesystem::path(utf8_path), std::ios::binary);
        if (!stream) return command_error(filename,"E_IO","Cannot open source file","Не удалось открыть исходный файл");
        std::string source;
        char buffer[8192];
        while (stream.read(buffer, sizeof buffer) || stream.gcount()) {
            source.append(buffer, static_cast<std::size_t>(stream.gcount()));
            if (source.size() > 1'048'576) throw abc::Error("E_LIMIT", {}, "Source exceeds 1 MiB prototype limit");
        }
        if (stream.bad()) return command_error(filename,"E_IO","Cannot read source file","Не удалось прочитать исходный файл");
        const auto program = abc::compile(source);
        if (arguments[1] == "run") abc::execute(program, std::cin, std::cout, options);
        else std::cout << "OK: " << filename << '\n';
        return 0;
    } catch (const abc::Error& error) {
        std::cerr << abc::format_diagnostic(error, filename);
        return 1;
    } catch (const std::exception& error) {
        return command_error(filename,"E_LAUNCH",std::string("Startup error: ")+error.what(),std::string("Ошибка запуска: ")+error.what());
    }
}
} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    // Preserve explicit CRLF inside strings instead of the CRT producing CRCRLF.
    _setmode(_fileno(stdout),_O_BINARY);
    _setmode(_fileno(stdin),_O_BINARY); // ConsoleInput decodes UTF-8 and retains CR/LF itself.
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
