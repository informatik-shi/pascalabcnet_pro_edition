#include "abc/core.hpp"

#include <array>

namespace abc {

std::string_view type_name(Type type) {
    switch (type) {
    case Type::integer: return "integer";
    case Type::int64: return "int64";
    case Type::real: return "real";
    case Type::boolean: return "boolean";
    case Type::string: return "string";
    case Type::character: return "char";
    case Type::word: return "word";
    case Type::byte: return "byte";
    case Type::shortint: return "shortint";
    case Type::smallint: return "smallint";
    case Type::text_file: return "Text";
    case Type::unit: return "procedure";
    default: return compound(type) ? "compound type" : "<invalid>";
    }
}

std::optional<Type> find_type(std::string_view name) {
    for (auto type : {Type::integer, Type::int64, Type::real, Type::boolean, Type::string, Type::character, Type::word, Type::byte, Type::shortint, Type::smallint})
        if (name == type_name(type)) return type;
    if (name == "double") return Type::real;
    if (name == "text" || name == "textfile") return Type::text_file;
    return std::nullopt;
}

bool numeric(Type type) {
    return type == Type::integer || type == Type::int64 || type == Type::real || small_integer(type);
}

bool can_widen(Type from, Type to) {
    return from == to || (from == Type::character && to == Type::string)
        || (small_integer(from) && (to == Type::integer || to == Type::int64 || to == Type::real))
        || (from==Type::byte && (to==Type::word || to==Type::smallint))
        || (from==Type::shortint && to==Type::smallint)
        || (from == Type::nil && to == Type::string) || (from == Type::integer && to == Type::int64)
        || ((from == Type::integer || from == Type::int64) && to == Type::real);
}

Type value_type(const Value& value) {
    if (std::holds_alternative<char16_t>(value)) return Type::character;
    if (std::holds_alternative<std::uint16_t>(value)) return Type::word;
    if (std::holds_alternative<std::uint8_t>(value)) return Type::byte;
    if (std::holds_alternative<std::int8_t>(value)) return Type::shortint;
    if (std::holds_alternative<std::int16_t>(value)) return Type::smallint;
    if (std::holds_alternative<std::shared_ptr<TextFile>>(value)) return Type::text_file;
    if (std::holds_alternative<NilString>(value)) return Type::string;
    if (const auto* aggregate = std::get_if<std::shared_ptr<Aggregate>>(&value)) return *aggregate ? (*aggregate)->type : Type::invalid;
    if (const auto* cell = std::get_if<std::shared_ptr<Cell>>(&value)) return *cell ? reference_type((*cell)->type) : Type::invalid;
    static constexpr std::array types{
        Type::integer, Type::int64, Type::real, Type::boolean, Type::string};
    return types.at(value.index());
}

const std::u16string& string_text(const Value& value) {
    static const std::u16string empty;
    return std::holds_alternative<NilString>(value) ? empty : std::get<std::u16string>(value);
}

int compare_strings(const Value& left, const Value& right) {
    const bool a = std::holds_alternative<NilString>(left), b = std::holds_alternative<NilString>(right);
    if (a || b) return a == b ? 0 : a ? -1 : 1;
    return string_text(left).compare(string_text(right));
}

std::u16string from_utf8(std::string_view text) {
    std::u16string result;
    for (std::size_t i = 0; i < text.size();) {
        const auto first = static_cast<unsigned char>(text[i++]);
        std::uint32_t cp;
        unsigned extra;
        std::uint32_t minimum;
        if (first < 0x80) { cp = first; extra = 0; minimum = 0; }
        else if ((first & 0xe0) == 0xc0) { cp = first & 0x1f; extra = 1; minimum = 0x80; }
        else if ((first & 0xf0) == 0xe0) { cp = first & 0x0f; extra = 2; minimum = 0x800; }
        else if ((first & 0xf8) == 0xf0) { cp = first & 7; extra = 3; minimum = 0x10000; }
        else throw std::invalid_argument("Invalid UTF-8 leading byte");
        for (unsigned n = 0; n < extra; ++n) {
            if (i == text.size()) throw std::invalid_argument("Truncated UTF-8 sequence");
            const auto next = static_cast<unsigned char>(text[i++]);
            if ((next & 0xc0) != 0x80) throw std::invalid_argument("Invalid UTF-8 continuation");
            cp = (cp << 6) | (next & 0x3f);
        }
        if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
            throw std::invalid_argument("Invalid UTF-8 code point");
        if (cp < 0x10000) result.push_back(static_cast<char16_t>(cp));
        else {
            cp -= 0x10000;
            result.push_back(static_cast<char16_t>(0xd800 + (cp >> 10)));
            result.push_back(static_cast<char16_t>(0xdc00 + (cp & 0x3ff)));
        }
    }
    return result;
}

std::string to_utf8(std::u16string_view text, bool replace_invalid) {
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        std::uint32_t cp = text[i];
        if (cp >= 0xd800 && cp <= 0xdbff) {
            if (i + 1 == text.size() || text[i + 1] < 0xdc00 || text[i + 1] > 0xdfff) {
                if (!replace_invalid) throw std::invalid_argument("Unpaired UTF-16 surrogate");
                cp = 0xfffd;
            } else cp = 0x10000 + ((cp - 0xd800) << 10) + (text[++i] - 0xdc00);
        } else if (cp >= 0xdc00 && cp <= 0xdfff) {
            if (!replace_invalid) throw std::invalid_argument("Unpaired UTF-16 surrogate");
            cp = 0xfffd;
        }
        if (cp < 0x80) result.push_back(static_cast<char>(cp));
        else if (cp < 0x800) {
            result.push_back(static_cast<char>(0xc0 | (cp >> 6)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp < 0x10000) {
            result.push_back(static_cast<char>(0xe0 | (cp >> 12)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else {
            result.push_back(static_cast<char>(0xf0 | (cp >> 18)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        }
    }
    return result;
}

} // namespace abc
