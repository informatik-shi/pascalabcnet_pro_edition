#include "diagnostics.hpp"

namespace abc {
Error runtime_error(RuntimeError id, SourceLocation location, std::string_view context) {
    const char *code, *english, *russian;
    switch (id) {
    case RuntimeError::regex_syntax:
        code="E_REGEX"; english="Invalid regular expression"; russian="Некорректное регулярное выражение"; break;
    case RuntimeError::duplicate_swap_key:
        code="E_KEY"; english="Duplicate swap key"; russian="Повторяющийся ключ замены"; break;
    case RuntimeError::missing_swap_key:
        code="E_KEY"; english="Matched text is not a swap key"; russian="Совпавший текст не является ключом замены"; break;
    case RuntimeError::school_ip_address:
        code="E_FORMAT"; english="Invalid IPv4 address"; russian="Некорректная запись адреса IPv4"; break;
    case RuntimeError::school_ip_suffix:
        code="E_RANGE"; english="IPv4 mask suffix must be between 1 and 31"; russian="Суффикс маски IPv4 должен быть от 1 до 31"; break;
    case RuntimeError::school_ip_mask:
        code="E_FORMAT"; english="Invalid contiguous IPv4 netmask"; russian="Некорректная непрерывная маска сети IPv4"; break;
    case RuntimeError::school_table_variables:
        code="E_RANGE"; english="Truth-table variable count must be between 0 and 30"; russian="Число переменных таблицы истинности должно быть от 0 до 30"; break;
    case RuntimeError::big_integer_limit:
        code="E_LIMIT"; english="BigInteger size limit exceeded"; russian="Превышен допустимый размер BigInteger"; break;
    case RuntimeError::power_exponent:
        code="E_RANGE"; english="Exponent cannot be negative"; russian="Показатель степени не может быть отрицательным"; break;
    case RuntimeError::school_empty_arguments:
        code="E_RANGE"; english="Argument list cannot be empty"; russian="Список аргументов не может быть пустым"; break;
    case RuntimeError::empty_substring:
        code="E_RANGE"; english="Replacement source cannot be empty"; russian="Заменяемая подстрока не может быть пустой"; break;
    case RuntimeError::school_factorization:
        code="E_FACTOR"; english="Prime factorization failed"; russian="Ошибка разложения на простые множители"; break;
    case RuntimeError::school_expanded_digit:
        code="E_RANGE"; english="A digit must be less than the base"; russian="Цифра должна быть меньше основания системы счисления"; break;
    case RuntimeError::school_base:
        code="E_RANGE"; english="Base must be between 2 and 36"; russian="Основание системы счисления должно быть от 2 до 36"; break;
    case RuntimeError::school_digit:
        code="E_FORMAT"; english="Invalid digit in base notation"; russian="Недопустимая цифра в записи числа"; break;
    case RuntimeError::field_width:
        code="E_RANGE"; english="Field width cannot be negative"; russian="Ширина поля не может быть отрицательной"; break;
    case RuntimeError::random_range:
        code = "E_RANGE"; english = "Random bounds are invalid"; russian = "Некорректные границы генератора случайных чисел"; break;
    case RuntimeError::random_source:
        code = "E_RANDOM"; english = "Cannot initialize random source"; russian = "Не удалось инициализировать генератор случайных чисел"; break;
    case RuntimeError::character_digit:
        code = "E_FORMAT"; english = "Character is not a decimal digit from 0 to 9"; russian = "Символ не является десятичной цифрой от 0 до 9"; break;
    case RuntimeError::integer_overflow:
        code = "E_OVERFLOW"; english = "Integer overflow"; russian = "Переполнение целого числа"; break;
    case RuntimeError::collection_changed:
        code = "E_MODIFIED"; english = "Collection was modified during enumeration"; russian = "Коллекция изменена во время перебора"; break;
    case RuntimeError::file_erase:
        code = "E_IO"; english = "Cannot erase text file"; russian = "Не удалось удалить текстовый файл"; break;
    case RuntimeError::file_rename:
        code = "E_IO"; english = "Cannot rename text file"; russian = "Не удалось переименовать текстовый файл"; break;
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
        code = "E_BOUNDS"; english = "Index is outside sequence bounds"; russian = "Индекс находится за границами строки, массива или списка"; break;
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
    case RuntimeError::sequence_empty:
        code = "E_EMPTY"; english = "Sequence contains no elements"; russian = "Последовательность не содержит элементов"; break;
    case RuntimeError::sequence_multiple:
        code = "E_MANY"; english = "Sequence contains more than one element"; russian = "Последовательность содержит более одного элемента"; break;
    case RuntimeError::sequence_count_range:
        code = "E_RANGE"; english = "Sequence count cannot be negative"; russian = "Число элементов последовательности не может быть отрицательным"; break;
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
