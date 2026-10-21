#include "diagnostics.hpp"

namespace abc {
Error runtime_error(RuntimeError id, SourceLocation location, std::string_view context) {
    const char *code, *english, *russian;
    switch (id) {
    case RuntimeError::integer_overflow:
        code = "E_OVERFLOW"; english = "Integer overflow"; russian = "Переполнение целого числа"; break;
    case RuntimeError::division_zero:
        code = "E_DIV_ZERO"; english = "Division by zero"; russian = "Деление на ноль"; break;
    case RuntimeError::invalid_bytecode:
        code = "E_BYTECODE"; english = "Invalid bytecode"; russian = "Некорректный байт-код"; break;
    case RuntimeError::number_format:
        code = "E_OUTPUT"; english = "Number formatting failed"; russian = "Не удалось преобразовать число в текст"; break;
    case RuntimeError::input_limit:
        code = "E_LIMIT"; english = "Input line exceeds 1 MiB"; russian = "Строка ввода превышает 1 МиБ"; break;
    case RuntimeError::input_stream:
        code = "E_INPUT"; english = "Input stream failed"; russian = "Ошибка потока ввода"; break;
    case RuntimeError::input_end:
        code = "E_INPUT"; english = "Unexpected end of input"; russian = "Неожиданный конец ввода"; break;
    case RuntimeError::input_encoding:
        code = "E_INPUT"; english = "Input must be valid UTF-8"; russian = "Ввод должен быть в корректной кодировке UTF-8"; break;
    case RuntimeError::input_empty:
        code = "E_INPUT"; english = "Expected a value, got an empty line"; russian = "Ожидалось значение, получена пустая строка"; break;
    case RuntimeError::input_boolean:
        code = "E_INPUT"; english = "Expected True or False"; russian = "Ожидалось True или False"; break;
    case RuntimeError::input_real:
        code = "E_INPUT"; english = "Expected a finite real number in range"; russian = "Ожидалось конечное вещественное число в допустимом диапазоне"; break;
    case RuntimeError::input_range:
        code = "E_INPUT"; english = "Input does not fit "; russian = "Введённое значение не соответствует типу "; break;
    case RuntimeError::output_limit:
        code = "E_LIMIT"; english = "Output limit exceeded"; russian = "Превышен предел объёма вывода"; break;
    case RuntimeError::output_stream:
        code = "E_OUTPUT"; english = "Output stream failed"; russian = "Ошибка потока вывода"; break;
    case RuntimeError::instruction_limit:
        code = "E_LIMIT"; english = "Instruction limit exceeded"; russian = "Превышен предел числа выполняемых инструкций"; break;
    case RuntimeError::cancelled:
        code = "E_CANCELLED"; english = "Execution cancelled"; russian = "Выполнение отменено"; break;
    case RuntimeError::string_limit:
        code = "E_LIMIT"; english = "String exceeds 1 Mi UTF-16 code units"; russian = "Строка превышает 1 048 576 кодовых единиц UTF-16"; break;
    case RuntimeError::real_overflow:
        code = "E_OVERFLOW"; english = "Real overflow"; russian = "Переполнение вещественного числа"; break;
    case RuntimeError::math_domain:
        code = "E_DOMAIN"; english = "Argument is outside the function domain"; russian = "Аргумент находится вне области определения функции"; break;
    case RuntimeError::round_digits:
        code = "E_RANGE"; english = "Round digits must be between 0 and 15"; russian = "Число знаков для Round должно быть от 0 до 15"; break;
    case RuntimeError::string_index:
        code = "E_RANGE"; english = "String search position must be at least 1"; russian = "Позиция поиска в строке должна быть не меньше 1"; break;
    default: throw std::logic_error("Unknown runtime diagnostic");
    }
    return Error(code, location, std::string(english) + std::string(context), std::string(russian) + std::string(context));
}

std::string format_diagnostic(const Error& error, std::string_view filename) {
    auto result = std::string(filename) + ':' + std::to_string(error.location.line) + ':'
        + std::to_string(error.location.column) + ": " + error.code + '\n';
    result += "EN: " + std::string(error.what()) + '\n';
    if (!error.russian.empty()) result += "RU: " + error.russian + '\n';
    return result;
}
} // namespace abc
