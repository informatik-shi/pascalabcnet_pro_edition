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
    case RuntimeError::number_text:
        code = "E_FORMAT"; english = "Invalid number text for "; russian = "Некорректная запись числа для типа "; break;
    case RuntimeError::number_range:
        code = "E_OVERFLOW"; english = "Number text is outside the range of "; russian = "Число в строке выходит за диапазон типа "; break;
    case RuntimeError::string_slice:
        code = "E_RANGE"; english = "Invalid string reversal index or length"; russian = "Некорректный индекс или длина разворачиваемой части строки"; break;
    case RuntimeError::decimal_separator:
        code = "E_RANGE"; english = "Decimal separator must not be empty"; russian = "Десятичный разделитель не должен быть пустым"; break;
    case RuntimeError::string_length:
        code = "E_RANGE"; english = "String length must not be negative"; russian = "Длина строки не должна быть отрицательной"; break;
    case RuntimeError::allocation_limit:
        code = "E_LIMIT"; english = "Aggregate allocation limit exceeded"; russian = "Превышен предел выделения памяти для значений"; break;
    case RuntimeError::index_bounds:
        code = "E_BOUNDS"; english = "Index is outside sequence bounds"; russian = "Индекс находится за границами массива или списка"; break;
    case RuntimeError::array_length:
        code = "E_RANGE"; english = "Array length must not be negative"; russian = "Длина массива не должна быть отрицательной"; break;
    case RuntimeError::call_limit:
        code = "E_LIMIT"; english = "Routine call depth limit exceeded"; russian = "Превышена допустимая глубина вызовов подпрограмм"; break;
    case RuntimeError::file_io:
        code = "E_IO"; english = "Cannot open, read or write the file"; russian = "Не удалось открыть, прочитать или записать файл"; break;
    case RuntimeError::file_unassigned:
        code="E_IO"; english="Text file has not been assigned a filename"; russian="Текстовому файлу не назначено имя"; break;
    case RuntimeError::file_closed:
        code="E_IO"; english="Text file is not open"; russian="Текстовый файл не открыт"; break;
    case RuntimeError::file_read_mode:
        code="E_IO"; english="Text file is not open for reading"; russian="Текстовый файл не открыт для чтения"; break;
    case RuntimeError::file_write_mode:
        code="E_IO"; english="Text file is not open for writing"; russian="Текстовый файл не открыт для записи"; break;
    case RuntimeError::file_end:
        code="E_EOF"; english="Cannot read a token past the end of the text file"; russian="Нельзя прочитать значение после конца текстового файла"; break;
    case RuntimeError::file_open_limit:
        code="E_LIMIT"; english="Too many open text files (limit 64)"; russian="Превышен предел числа открытых текстовых файлов (64)"; break;
    case RuntimeError::directory_io:
        code = "E_IO"; english = "Directory operation failed"; russian = "Не удалось выполнить операцию с каталогом"; break;
    case RuntimeError::file_limit:
        code = "E_LIMIT"; english = "File exceeds the 1 MiB limit"; russian = "Размер файла превышает предел 1 МиБ"; break;
    case RuntimeError::file_encoding:
        code = "E_ENCODING"; english = "Text file must contain valid UTF-8"; russian = "Текстовый файл должен содержать корректный UTF-8"; break;
    case RuntimeError::nil_reference:
        code = "E_NIL"; english = "Cannot access a nil reference"; russian = "Нельзя обратиться к пустой ссылке nil"; break;
    case RuntimeError::stack_empty:
        code = "E_EMPTY"; english = "Stack is empty"; russian = "Стек пуст"; break;
    case RuntimeError::queue_empty:
        code = "E_EMPTY"; english = "Queue is empty"; russian = "Очередь пуста"; break;
    case RuntimeError::linked_empty:
        code = "E_EMPTY"; english = "Linked list is empty"; russian = "Связанный список пуст"; break;
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
