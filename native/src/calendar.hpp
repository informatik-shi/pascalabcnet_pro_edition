#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;
enum class CalendarMethod : std::uint8_t {
    from_ticks, from_ticks_kind,
    date_parts, date_clock, date_clock_kind, date_millis, date_millis_kind,
    span_clock, span_days, span_millis,
    minimum, maximum, zero, now, utc_now, today, specify_kind,
    leap_year, days_in_month,
    year, month, day, hour, minute, second, millisecond, weekday, day_of_year, kind, date, time_of_day, ticks,
    total_days, total_hours, total_minutes, total_seconds, total_milliseconds,
    from_days, from_hours, from_minutes, from_seconds, from_milliseconds,
    add_days, add_hours, add_minutes, add_seconds, add_milliseconds, add_ticks, add_months, add_years,
    add, subtract, difference, negate, duration, hash, compare, compare_static,
    equal, unequal, less, less_equal, greater, greater_equal, equals_static,
    to_string, to_string_format, short_date_text, short_time_text, long_date_text, long_time_text, parse, try_parse,
    to_local_time, to_universal_time, is_daylight_saving
};
inline bool calendar_record(const TypeInfo& type) {
    return type.native_record==NativeRecord::date_time || type.native_record==NativeRecord::time_span;
}
bool calendar_record_shape(std::span<const TypeInfo>,const TypeInfo&);
bool calendar_signature(std::span<const TypeInfo>,Type,std::size_t);
std::vector<Type> calendar_parameters(std::span<const TypeInfo>,Type,std::size_t);
Type calendar_result(std::span<const TypeInfo>,Type,std::size_t);
Value calendar_invoke(Values&,Type,std::size_t,std::span<const Value>);
std::u16string calendar_text(const Value&,std::u16string_view,SourceLocation);
int compare_calendar_values(const Value&,const Value&);
double calendar_ole_date(const Value&,SourceLocation);
std::int32_t calendar_hash_code(const Value&);
inline constexpr std::uint64_t date_time_ticks_mask=(std::uint64_t{1}<<62U)-1;
std::uint64_t date_time_binary_bits(const Value&);
Value date_time_from_binary_bits(Values&,Type,std::uint64_t);
}
