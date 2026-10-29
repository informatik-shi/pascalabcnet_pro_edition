#include "abc/core.hpp"

namespace abc {
namespace {
using Translation=std::pair<std::string_view,std::string_view>;
constexpr Translation messages[]={
#include "compiler_messages.inc"
};
constexpr Translation prefixes[]={
    {"Unit is not ported yet: '","Модуль пока не перенесён: '"},
    {"Statement is not supported: '","Оператор не поддерживается: '"},
    {"Unknown or unsupported type '","Неизвестный или неподдерживаемый тип '"},
    {"Unknown name '","Неизвестное имя '"},
    {"Constant is outside the range of ","Константа находится вне диапазона типа "},
    {"Text member is not ported: '","Член Text пока не перенесён: '"},
    {"Unknown Addr32 member '","Неизвестный член Addr32 '"},
    {"Unknown CalcIP member '","Неизвестный член CalcIP '"},
    {"Unknown linked node member '","Неизвестный член связанного узла '"},
    {"Unknown linked list member '","Неизвестный член связанного списка '"},
    {"Unknown AssocArray member '","Неизвестный член AssocArray '"},
    {"Unknown collection member '","Неизвестный член коллекции '"},
    {"Unknown stack or queue method '","Неизвестный метод стека или очереди '"},
    {"Unknown record field '","Неизвестное поле записи '"},
    {"Variable is not callable: '","Переменную нельзя вызывать как функцию: '"},
    {"Invalid operand for '","Недопустимый операнд для '"},
    {"Wrong random array/matrix arguments for '","Неверные аргументы случайного массива или матрицы для '"},
    {"Unknown function '","Неизвестная функция '"},
    {"Ambiguous overload for '","Неоднозначная перегрузка функции '"},
    {"No matching overload for '","Не найдена подходящая перегрузка функции '"},
    {"Duplicate constant name '","Повторное имя константы '"},
    {"Duplicate name '","Повторное имя '"},
    {"Wrong argument count for '","Неверное число аргументов функции '"},
};
std::string localized_types(std::string text) {
    const std::string_view name="compound type";
    for (auto at=text.find(name);at!=std::string::npos;at=text.find(name,at+1))
        text.replace(at,name.size(),"составной тип");
    return text;
}
std::string russian_message(std::string_view code,std::string_view english) {
    for (const auto& [message,translation]:messages) if (english==message) return std::string(translation);
    constexpr std::string_view assign="Cannot assign ";
    if (english.starts_with(assign)) {
        const auto split=english.find(" to ",assign.size());
        if (split!=std::string_view::npos)
            return "Нельзя присвоить значение типа "+localized_types(std::string(english.substr(assign.size(),split-assign.size())))
                +" значению типа "+localized_types(std::string(english.substr(split+4)));
    }
    if (english.starts_with("Expected '")) {
        const auto split=english.find("', found '",10);
        if (split!=std::string_view::npos)
            return "Ожидалось '"+std::string(english.substr(10,split-10))+"', получено '"+std::string(english.substr(split+10));
    }
    constexpr std::string_view operands="Incompatible operands for '";
    if (english.starts_with(operands)) {
        auto rest=localized_types(std::string(english.substr(operands.size())));
        const auto split=rest.rfind(" and ");
        if (split!=std::string::npos) rest.replace(split,5," и ");
        return "Несовместимые операнды для '"+rest;
    }
    for (const auto& [prefix,translation]:prefixes)
        if (english.starts_with(prefix)) return std::string(translation)+std::string(english.substr(prefix.size()));
    // Keep diagnostics readable when an internal failure or a new diagnostic
    // has no detailed entry yet; English always preserves its original detail.
    constexpr Translation categories[]={
        {"E_SYNTAX","Синтаксическая ошибка"},
        {"E_TYPE","Несовместимые типы или недопустимые аргументы"},
        {"E_NAME","Неизвестное имя"},
        {"E_DUPLICATE","Повторное объявление имени"},
        {"E_RANGE","Значение находится вне допустимого диапазона"},
        {"E_LIMIT","Превышено ограничение"},
        {"E_UNSUPPORTED","Эта возможность пока не поддерживается"},
        {"E_ENCODING","Некорректная кодировка UTF-8"},
        {"E_HIR","Внутренняя ошибка типизированного представления программы"},
        {"E_BYTECODE","Некорректный байт-код: выполнение невозможно"}
    };
    for (const auto& [category,translation]:categories) if (code==category) return std::string(translation);
    return {};
}
}
Error::Error(std::string code,SourceLocation location,std::string message,std::string russian)
    :std::runtime_error(std::move(message)),code(std::move(code)),location(location),
    russian(russian.empty() ? russian_message(this->code,what()) : std::move(russian)) {}
}
