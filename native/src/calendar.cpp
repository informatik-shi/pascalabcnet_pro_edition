#include "calendar.hpp"
#include "diagnostics.hpp"
#include "runtime_values.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iterator>
#include <limits>
#include <string_view>

namespace abc {
namespace {
using M=CalendarMethod;
using namespace std::chrono;
constexpr std::int64_t ticks_per_millisecond=10'000;
constexpr std::int64_t ticks_per_second=1'000*ticks_per_millisecond;
constexpr std::int64_t ticks_per_minute=60*ticks_per_second;
constexpr std::int64_t ticks_per_hour=60*ticks_per_minute;
constexpr std::int64_t ticks_per_day=24*ticks_per_hour;
constexpr std::int64_t date_max_ticks=3'155'378'975'999'999'999;
constexpr auto epoch=sys_days{year{1}/January/1};
constexpr auto unix_epoch=sys_days{year{1970}/January/1};
constexpr auto unix_epoch_ticks=(unix_epoch-epoch).count()*ticks_per_day;
constexpr int unspecified_kind=0, utc_kind=1, local_kind=2, ambiguous_daylight_kind=3;

struct CalendarValue {std::int64_t ticks; std::int32_t kind;};
struct ClockFields {int hour,minute,second,millisecond;};
constexpr auto method(std::size_t operation) {return static_cast<M>(operation&255U);}
constexpr auto payload(std::size_t operation) {return static_cast<Type>(operation>>8U);}
const TypeInfo* find_type(std::span<const TypeInfo> types,Type type) {
    const auto number=static_cast<unsigned>(type);
    if (!compound(type) || number-64U>=types.size()) return nullptr;
    return &types[number-64U];
}
bool tagged_enum(const TypeInfo* info,std::string_view name,std::span<const std::string_view> enumerators) {
    if (!info || info->kind!=CompoundKind::enumeration || info->name!=name || info->enum_flags
        || info->enum_members.size()!=enumerators.size()) return false;
    for (std::size_t i=0;i<enumerators.size();++i)
        if (info->enum_members[i].text!=enumerators[i] || info->enum_members[i].value!=static_cast<std::int32_t>(i)) return false;
    return true;
}
constexpr std::array<std::string_view,3> kinds{"Unspecified","Utc","Local"};
constexpr std::array<std::string_view,7> weekdays{"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
constexpr std::array<std::string_view,12> months{"January","February","March","April","May","June",
    "July","August","September","October","November","December"};
Error bad_date(SourceLocation location) {
    return Error("E_RANGE",location,"DateTime value is outside the supported calendar range",
        "Значение DateTime выходит за границы календаря");
}
Error bad_time(SourceLocation location) {
    return Error("E_RANGE",location,"DateTime time component is out of range",
        "Компонент времени DateTime выходит за допустимые границы");
}
Error bad_kind(SourceLocation location) {
    return Error("E_RANGE",location,"Invalid DateTimeKind value","Недопустимое значение DateTimeKind");
}
Error bad_fraction(SourceLocation location) {
    return Error("E_RANGE",location,"Time interval requires a finite number",
        "Для интервала времени требуется конечное число");
}
std::int64_t safe_add(std::int64_t first,std::int64_t second,SourceLocation location) {
    if ((second>0 && first>INT64_MAX-second) || (second<0 && first<INT64_MIN-second))
        throw runtime_error(RuntimeError::integer_overflow,location);
    return first+second;
}
std::int64_t safe_subtract(std::int64_t first,std::int64_t second,SourceLocation location) {
    if ((second>0 && first<INT64_MIN+second) || (second<0 && first>INT64_MAX+second))
        throw runtime_error(RuntimeError::integer_overflow,location);
    return first-second;
}
std::int32_t valid_kind(std::int32_t kind,SourceLocation location) {
    if (kind<unspecified_kind || kind>local_kind) throw bad_kind(location);
    return kind;
}
std::int64_t date_ticks(std::int64_t ticks,SourceLocation location) {
    if (ticks<0 || ticks>date_max_ticks) throw bad_date(location);
    return ticks;
}
std::int64_t shift_date(std::int64_t base,std::int64_t offset,SourceLocation location) {
    if (offset < -base || offset > date_max_ticks-base) throw bad_date(location);
    return base+offset;
}
std::int64_t date_only(int y,int m,int d,SourceLocation location) {
    if (y<1 || y>9999 || m<1 || m>12 || d<1 || d>31) throw bad_date(location);
    const year_month_day fields=year{y}/month{static_cast<unsigned>(m)}/day{static_cast<unsigned>(d)};
    if (!fields.ok()) throw bad_date(location);
    return (sys_days{fields}-epoch).count()*ticks_per_day;
}
year_month_day date_fields(std::int64_t ticks) {
    return year_month_day{epoch+days{ticks/ticks_per_day}};
}
ClockFields clock_fields(std::int64_t ticks) {
    return {static_cast<int>(ticks/ticks_per_hour%24),static_cast<int>(ticks/ticks_per_minute%60),
        static_cast<int>(ticks/ticks_per_second%60),static_cast<int>(ticks/ticks_per_millisecond%1000)};
}
std::int64_t clock_ticks(int h,int m,int s,int ms,SourceLocation location) {
    if (h<0 || h>23 || m<0 || m>59 || s<0 || s>59 || ms<0 || ms>999) throw bad_time(location);
    return h*ticks_per_hour+m*ticks_per_minute+s*ticks_per_second+ms*ticks_per_millisecond;
}
std::int64_t span_components(int d,int h,int m,int s,int ms,SourceLocation location) {
    const auto millis=d*(ticks_per_day/ticks_per_millisecond)+h*(ticks_per_hour/ticks_per_millisecond)
        +m*(ticks_per_minute/ticks_per_millisecond)+s*(ticks_per_second/ticks_per_millisecond)+ms;
    if (millis<INT64_MIN/ticks_per_millisecond || millis>INT64_MAX/ticks_per_millisecond)
        throw runtime_error(RuntimeError::integer_overflow,location);
    return millis*ticks_per_millisecond;
}
std::int64_t interval(double number,std::int64_t scale,bool for_date,SourceLocation location) {
    if (!std::isfinite(number)) throw bad_fraction(location);
    const auto millis=number*static_cast<double>(scale/ticks_per_millisecond)+(number>=0?0.5:-0.5);
    if (for_date && (millis<=-static_cast<double>(date_max_ticks/ticks_per_millisecond+1)
        || millis>=static_cast<double>(date_max_ticks/ticks_per_millisecond+1))) throw bad_date(location);
    if (millis<static_cast<double>(INT64_MIN/ticks_per_millisecond)
        || millis>static_cast<double>(INT64_MAX/ticks_per_millisecond))
        throw runtime_error(RuntimeError::integer_overflow,location);
    return static_cast<std::int64_t>(millis)*ticks_per_millisecond;
}
std::int64_t add_months(std::int64_t ticks,std::int64_t months_to_add,SourceLocation location) {
    const auto base=date_fields(ticks);
    const auto month_index=(static_cast<std::int64_t>(static_cast<int>(base.year()))-1)*12
        +static_cast<unsigned>(base.month())-1+months_to_add;
    if (month_index<0 || month_index>=9999*12) throw bad_date(location);
    const auto year_part=year{static_cast<int>(month_index/12+1)};
    const auto month_part=month{static_cast<unsigned>(month_index%12+1)};
    const auto last_day=static_cast<unsigned>((year_part/month_part/last).day());
    const auto result=year_part/month_part/day{std::min(static_cast<unsigned>(base.day()),last_day)};
    return (sys_days{result}-epoch).count()*ticks_per_day+ticks%ticks_per_day;
}
CalendarValue calendar_value(const Value& raw) {
    const auto& value=std::get<std::shared_ptr<Aggregate>>(raw);
    return {std::get<std::int64_t>(value->items[0]->value),
        value->items.size()>1?std::get<std::int32_t>(value->items[1]->value):unspecified_kind};
}
Value make_calendar(Values& values,Type type,std::int64_t ticks,std::int32_t kind=unspecified_kind) {
    auto object=std::get<std::shared_ptr<Aggregate>>(values.zero(type,false,true));
    object->items[0]->value=ticks;
    if (object->items.size()>1) object->items[1]->value=kind;
    return object;
}
Error local_zone_error(SourceLocation location) {
    return Error("E_RANGE",location,"Local time zone cannot represent this DateTime",
        "Эта дата выходит за диапазон локального часового пояса");
}
std::optional<std::tm> local_fields(std::time_t unix_time) {
    std::tm local{};
#ifdef _WIN32
    if (localtime_s(&local,&unix_time)!=0) return {};
#else
    if (!localtime_r(&unix_time,&local)) return {};
#endif
    return local;
}
bool same_wall_clock(const std::tm& a,const std::tm& b) {
    return a.tm_year==b.tm_year && a.tm_mon==b.tm_mon && a.tm_mday==b.tm_mday
        && a.tm_hour==b.tm_hour && a.tm_min==b.tm_min && a.tm_sec==b.tm_sec;
}
std::optional<std::time_t> wall_clock_time(std::tm& fields,int dst) {
    fields.tm_isdst=dst;
    errno=0;
    const auto result=std::mktime(&fields);
    if (result==std::time_t{-1} && errno) return {};
    return result;
}
std::optional<std::time_t> local_to_unix(CalendarValue value) {
    const auto date=date_fields(value.ticks);
    const auto time=clock_fields(value.ticks);
    std::tm wall{};
    wall.tm_year=static_cast<int>(date.year())-1900;
    wall.tm_mon=static_cast<int>(static_cast<unsigned>(date.month()))-1;
    wall.tm_mday=static_cast<int>(static_cast<unsigned>(date.day()));
    wall.tm_hour=time.hour;
    wall.tm_min=time.minute;
    wall.tm_sec=time.second;
    auto requested=wall;
    // An explicit local wall time uses standard time during the overlap.
    const auto preferred=wall_clock_time(requested,value.kind==ambiguous_daylight_kind?1:0);
    if (preferred && same_wall_clock(requested,wall)) return preferred;
    auto normalized=wall;
    const auto detected=wall_clock_time(normalized,-1);
    if (detected && same_wall_clock(normalized,wall)) return detected;
    // During a skipped interval the standard-time offset remains defined.
    return preferred?preferred:detected;
}
std::optional<CalendarValue> utc_to_local(std::int64_t ticks,SourceLocation location) {
    using ticks_duration=duration<std::int64_t,std::ratio<1,10'000'000>>;
    const auto unix_ticks=ticks_duration{ticks-unix_epoch_ticks};
    const auto seconds_floor=floor<seconds>(unix_ticks);
    const auto remainder=(unix_ticks-seconds_floor).count();
    const auto local=local_fields(static_cast<std::time_t>(seconds_floor.count()));
    if (!local) return {};
    const auto y=local->tm_year+1900;
    if (y<1) return CalendarValue{0,local_kind};
    if (y>9999) return CalendarValue{date_max_ticks,local_kind};
    const auto local_ticks=date_only(y,local->tm_mon+1,local->tm_mday,location)
        +clock_ticks(local->tm_hour,local->tm_min,local->tm_sec,0,location)+remainder;
    auto kind=local_kind;
    if (local->tm_isdst>0) {
        auto standard=*local;
        if (const auto alternative=wall_clock_time(standard,0); alternative && same_wall_clock(standard,*local)
            && *alternative!=static_cast<std::time_t>(seconds_floor.count())) kind=ambiguous_daylight_kind;
    }
    return CalendarValue{local_ticks,kind};
}
CalendarValue to_universal(CalendarValue date,SourceLocation location) {
    if (date.kind==utc_kind) return date;
    const auto unix_time=local_to_unix(date);
    if (!unix_time) throw local_zone_error(location);
    const auto ticks=static_cast<std::int64_t>(*unix_time)*ticks_per_second+unix_epoch_ticks
        +date.ticks%ticks_per_second;
    return {std::clamp<std::int64_t>(ticks,0,date_max_ticks),utc_kind};
}
CalendarValue current_date(M call,SourceLocation location) {
    using ticks_duration=duration<std::int64_t,std::ratio<1,10'000'000>>;
    const auto now=unix_epoch_ticks+duration_cast<ticks_duration>(system_clock::now().time_since_epoch()).count();
    if (call==M::utc_now) return {now,utc_kind};
    auto local=utc_to_local(now,location);
    if (!local) throw local_zone_error(location);
    if (call==M::today) return {local->ticks-local->ticks%ticks_per_day,local_kind};
    return *local;
}
void digits(std::u16string& text,std::uint64_t number,std::size_t width=1) {
    char16_t buffer[20];
    auto cursor=std::end(buffer);
    do {
        *--cursor=static_cast<char16_t>(u'0'+number%10U);
        number/=10U;
    } while (number);
    const auto length=static_cast<std::size_t>(std::end(buffer)-cursor);
    if (length<width) text.append(width-length,u'0');
    text.append(cursor,std::end(buffer));
}
void ascii(std::u16string& text,std::string_view word) {
    for (const auto ch:word) text.push_back(static_cast<char16_t>(ch));
}
struct TextCursor {
    std::u16string_view text;
    std::size_t offset=0;

    void spaces() {
        while (offset<text.size() && (text[offset]==u' ' || text[offset]==u'\r' || text[offset]==u'\n' || text[offset]==u'\t'))
            ++offset;
    }
    bool take(char16_t ch) {
        if (offset==text.size() || text[offset]!=ch) return false;
        ++offset;
        return true;
    }
    std::optional<int> number(std::size_t max_digits) {
        const auto first=offset;
        int value=0;
        while (offset<text.size() && text[offset]>=u'0' && text[offset]<=u'9') {
            if (offset-first==max_digits) return {};
            value=value*10+(text[offset++]-u'0');
        }
        if (offset==first) return {};
        return value;
    }
    bool done() const {return offset==text.size();}
    std::optional<int> fraction() {
        const auto first=offset;
        auto value=number(7);
        if (!value) return {};
        for (auto count=offset-first;count<7;++count) *value*=10;
        return value;
    }
    std::optional<int> two_digits() {
        if (text.size()-offset<2 || text[offset]<u'0' || text[offset]>u'9'
            || text[offset+1]<u'0' || text[offset+1]>u'9') return {};
        const auto number=10*(text[offset]-u'0')+(text[offset+1]-u'0');
        offset+=2;
        return number;
    }
};
std::optional<CalendarValue> parse_date(std::u16string_view text,SourceLocation location) {
    TextCursor input{text};
    input.spaces();
    const auto first_index=input.offset;
    const auto first=input.number(4);
    if (!first) return {};
    const auto width=input.offset-first_index;
    const auto separator=input.take(u'/')?u'/':input.take(u'-')?u'-':u'\0';
    if (!separator) return {};
    const auto second=input.number(2);
    if (!second || !input.take(separator)) return {};
    const auto last_index=input.offset;
    const auto third=input.number(4);
    if (!third) return {};
    const bool year_first=width==4;
    auto y=year_first?*first:*third;
    const auto m=year_first?*second:*first;
    const auto d=year_first?*third:*second;
    if (!year_first && input.offset-last_index<=2) y+=y<=29?2000:1900;
    if (y<1 || y>9999 || m<1 || m>12 || d<1 || d>31) return {};
    const auto fields=year{y}/month{static_cast<unsigned>(m)}/day{static_cast<unsigned>(d)};
    if (!fields.ok()) return {};
    const auto base=(sys_days{fields}-epoch).count()*ticks_per_day;
    const auto date_end=input.offset;
    input.spaces();
    if (input.done()) return CalendarValue{base,unspecified_kind};
    if (input.offset==date_end && !input.take(u'T') && !input.take(u't')) return {};
    auto hour=input.number(2);
    if (!hour || !input.take(u':')) return {};
    auto minute=input.number(2);
    if (!minute || *minute>=60) return {};
    int second_value=0;
    int fraction=0;
    if (input.take(u':')) {
        const auto seconds_part=input.number(2);
        if (!seconds_part || *seconds_part>=60) return {};
        second_value=*seconds_part;
        if (input.take(u'.')) {
            const auto fractional=input.fraction();
            if (!fractional) return {};
            fraction=*fractional;
        }
    }
    input.spaces();
    std::optional<bool> afternoon;
    if (input.take(u'P') || input.take(u'p')) afternoon=true;
    else if (input.take(u'A') || input.take(u'a')) afternoon=false;
    if (afternoon) {
        if ((!input.take(u'M') && !input.take(u'm')) || *hour<1 || *hour>12) return {};
        *hour=(*hour%12)+(*afternoon?12:0);
        input.spaces();
    }
    std::optional<int> utc_offset;
    if (input.take(u'Z') || input.take(u'z')) utc_offset=0;
    else {
        const bool positive=input.take(u'+');
        if (positive || input.take(u'-')) {
            const auto hours=input.two_digits();
            if (!hours) return {};
            int minutes=0;
            if (input.take(u':') || (!input.done() && input.text[input.offset]>=u'0' && input.text[input.offset]<=u'9')) {
                const auto part=input.two_digits();
                if (!part) return {};
                minutes=*part;
            }
            if (*hours>14 || minutes>=60 || (*hours==14 && minutes!=0)) return {};
            utc_offset=(positive?1:-1)*(*hours*60+minutes);
        }
    }
    input.spaces();
    if (!input.done() || *hour>=24) return {};
    const auto wall=base+*hour*ticks_per_hour+*minute*ticks_per_minute+second_value*ticks_per_second+fraction;
    if (!utc_offset) return CalendarValue{wall,unspecified_kind};
    const auto utc=wall-*utc_offset*ticks_per_minute;
    if (utc<0 || utc>date_max_ticks) return {};
    return utc_to_local(utc,location);
}
std::optional<CalendarValue> parse_span(std::u16string_view text) {
    TextCursor input{text};
    input.spaces();
    const bool negative=input.take(u'-');
    const auto first=input.number(8);
    if (!first) return {};
    int days_count=0,h=0,m=0,s=0,subsecond=0;
    const bool separated_days=input.take(u'.');
    if (separated_days) {
        days_count=*first;
        const auto hours=input.number(2);
        if (!hours || !input.take(u':')) return {};
        h=*hours;
        const auto minutes=input.number(2);
        if (!minutes) return {};
        m=*minutes;
    } else if (input.take(u':')) {
        h=*first;
        const auto minutes=input.number(2);
        if (!minutes) return {};
        m=*minutes;
    } else days_count=*first;
    if (input.take(u':')) {
        const auto seconds_value=input.number(2);
        if (!seconds_value) return {};
        s=*seconds_value;
        if (!separated_days && input.take(u':')) {
            const auto remaining=input.number(2);
            if (!remaining) return {};
            days_count=h;
            h=m;
            m=s;
            s=*remaining;
        }
        if (input.take(u'.')) {
            const auto value=input.fraction();
            if (!value) return {};
            subsecond=*value;
        }
    }
    input.spaces();
    if (!input.done() || days_count>INT64_MAX/ticks_per_day || h>=24 || m>=60 || s>=60) return {};
    const auto duration=static_cast<std::uint64_t>(days_count)*ticks_per_day
        +static_cast<std::uint64_t>(h)*ticks_per_hour+static_cast<std::uint64_t>(m)*ticks_per_minute
        +static_cast<std::uint64_t>(s)*ticks_per_second+static_cast<std::uint64_t>(subsecond);
    constexpr auto positive_limit=static_cast<std::uint64_t>(INT64_MAX);
    if (duration>positive_limit+static_cast<unsigned>(negative)) return {};
    const auto ticks=negative?std::bit_cast<std::int64_t>(std::uint64_t{0}-duration):static_cast<std::int64_t>(duration);
    return CalendarValue{ticks,unspecified_kind};
}
std::u16string span_text(std::int64_t ticks,std::u16string_view format,SourceLocation location) {
    const bool general=format==u"g",full=format==u"G";
    if (!general && !full && !format.empty() && format!=u"c" && format!=u"t" && format!=u"T")
        throw Error("E_FORMAT",location,"Unsupported TimeSpan format","Формат TimeSpan ещё не поддерживается");
    const auto bits=static_cast<std::uint64_t>(ticks);
    const auto count=ticks<0?std::uint64_t{0}-bits:bits;
    const auto days_part=count/ticks_per_day;
    auto fraction=count%ticks_per_second;
    std::u16string text;
    if (ticks<0) text.push_back(u'-');
    if (days_part || full) {
        digits(text,days_part);
        text.push_back(general || full?u':':u'.');
    }
    digits(text,count/ticks_per_hour%24,general?1:2);
    text.push_back(u':');
    digits(text,count/ticks_per_minute%60,2);
    text.push_back(u':');
    digits(text,count/ticks_per_second%60,2);
    if (fraction || full) {
        std::size_t fraction_width=7;
        if (general)
            while (fraction%10==0) {fraction/=10;--fraction_width;}
        text.push_back(u'.');
        digits(text,fraction,fraction_width);
    }
    return text;
}
std::int64_t local_offset_minutes(CalendarValue value,SourceLocation location) {
    const auto utc=local_to_unix(value);
    if (!utc) throw local_zone_error(location);
    using ticks_duration=duration<std::int64_t,std::ratio<1,10'000'000>>;
    const auto wall_seconds=floor<seconds>(ticks_duration{value.ticks-unix_epoch_ticks}).count();
    return (wall_seconds-static_cast<std::int64_t>(*utc))/60;
}
void append_offset(std::u16string& text,std::int64_t minutes,std::size_t width) {
    text.push_back(minutes<0?u'-':u'+');
    const auto magnitude=static_cast<std::uint64_t>(std::abs(minutes));
    digits(text,magnitude/60,width==1?1:2);
    if (width>=3) {
        text.push_back(u':');
        digits(text,magnitude%60,2);
    }
}
Error date_format_error(SourceLocation location) {
    return Error("E_FORMAT",location,"Invalid DateTime format","Некорректный формат DateTime");
}
std::u16string custom_date_text(CalendarValue value,std::u16string_view format,SourceLocation location) {
    const auto fields=date_fields(value.ticks);
    const auto clock=clock_fields(value.ticks);
    const auto day_number=static_cast<unsigned>(fields.day());
    const auto month_number=static_cast<unsigned>(fields.month());
    const auto year_number=static_cast<unsigned>(static_cast<int>(fields.year()));
    const auto day_name=weekdays[static_cast<std::size_t>((value.ticks/ticks_per_day+1)%7)];
    const auto month_name=months[month_number-1];
    std::u16string text;
    const auto render=[&](char16_t token,std::size_t width) {
        const auto narrow=std::min<std::size_t>(width,2);
        switch (token) {
        case u'y':
            digits(text,width<=2?year_number%100:year_number,width);
            break;
        case u'M':
            if (width<=2) digits(text,month_number,width);
            else ascii(text,width==3?month_name.substr(0,3):month_name);
            break;
        case u'd':
            if (width<=2) digits(text,day_number,width);
            else ascii(text,width==3?day_name.substr(0,3):day_name);
            break;
        case u'H': digits(text,static_cast<unsigned>(clock.hour),narrow); break;
        case u'h': digits(text,static_cast<unsigned>(clock.hour%12==0?12:clock.hour%12),narrow); break;
        case u'm': digits(text,static_cast<unsigned>(clock.minute),narrow); break;
        case u's': digits(text,static_cast<unsigned>(clock.second),narrow); break;
        case u't':
            text.push_back(clock.hour<12?u'A':u'P');
            if (width>=2) text.push_back(u'M');
            break;
        case u'f': case u'F': {
            if (width>7) throw date_format_error(location);
            auto fraction=static_cast<std::uint64_t>(value.ticks%ticks_per_second);
            for (auto i=width;i<7;++i) fraction/=10;
            auto count=width;
            if (token==u'F') {
                while (count && fraction%10==0) {fraction/=10;--count;}
                if (!count && text.ends_with(u'.')) text.pop_back();
            }
            if (count) digits(text,fraction,count);
            break;
        }
        case u'K':
            if (value.kind==utc_kind) text.append(width,u'Z');
            if (value.kind>=local_kind) {
                const auto offset=local_offset_minutes(value,location);
                for (std::size_t i=0;i<width;++i) append_offset(text,offset,3);
            }
            break;
        case u'g':
            text+=u"A.D.";
            break;
        case u'z':
            append_offset(text,local_offset_minutes(value,location),width);
            break;
        default:
            text.append(width,token);
            break;
        }
    };
    for (std::size_t i=0;i<format.size();) {
        const auto token=format[i++];
        if (token==u'\'' || token==u'"') {
            while (i<format.size() && format[i]!=token) {
                if (format[i]==u'\\' && ++i==format.size()) throw date_format_error(location);
                text.push_back(format[i++]);
            }
            if (i==format.size()) throw date_format_error(location);
            ++i;
        } else if (token==u'\\') {
            if (i==format.size()) throw date_format_error(location);
            text.push_back(format[i++]);
        } else if (token==u'%') {
            if (i==format.size() || format[i]==u'%' || format[i]==u'\\' || format[i]==u'\'' || format[i]==u'"')
                throw date_format_error(location);
            render(format[i++],1);
        } else {
            const auto start=i-1;
            while (i<format.size() && format[i]==token) ++i;
            render(token,i-start);
        }
        if (text.size()>1'048'576) throw runtime_error(RuntimeError::string_limit,location);
    }
    return text;
}
std::u16string date_text(CalendarValue date,std::u16string_view format,SourceLocation location) {
    if (format.size()>1) return custom_date_text(date,format,location);
    if (format==u"U") {date=to_universal(date,location);format=u"F";}
    if (format.empty()) format=u"G";
    const auto fields=date_fields(date.ticks);
    const auto clock=clock_fields(date.ticks);
    const auto y=static_cast<unsigned>(static_cast<int>(fields.year()));
    const auto m=static_cast<unsigned>(fields.month());
    const auto d=static_cast<unsigned>(fields.day());
    std::u16string text;
    const auto iso_date=[&] {
        digits(text,y,4); text.push_back(u'-');
        digits(text,m,2); text.push_back(u'-'); digits(text,d,2);
    };
    const auto time=[&](bool international,bool show_seconds) {
        const auto hour=international?clock.hour:(clock.hour%12==0?12:clock.hour%12);
        digits(text,static_cast<unsigned>(hour),international?2:1);
        text.push_back(u':'); digits(text,static_cast<unsigned>(clock.minute),2);
        if (show_seconds) {text.push_back(u':'); digits(text,static_cast<unsigned>(clock.second),2);}
        if (!international) text+=clock.hour<12?u" AM":u" PM";
    };
    if (format==u"O" || format==u"o" || format==u"s" || format==u"u") {
        iso_date();
        text.push_back(format==u"u"?u' ':u'T');
        time(true,true);
        if (format==u"O" || format==u"o") {
            text.push_back(u'.'); digits(text,static_cast<std::uint64_t>(date.ticks%ticks_per_second),7);
            if (date.kind==utc_kind) text.push_back(u'Z');
            if (date.kind>=local_kind) append_offset(text,local_offset_minutes(date,location),3);
        } else if (format==u"u") text.push_back(u'Z');
        return text;
    }
    if (format==u"R" || format==u"r") {
        ascii(text,weekdays[static_cast<std::size_t>((date.ticks/ticks_per_day+1)%7)].substr(0,3));
        text+=u", "; digits(text,d,2); text.push_back(u' '); ascii(text,months[m-1].substr(0,3));
        text.push_back(u' '); digits(text,y,4); text.push_back(u' '); time(true,true); text+=u" GMT";
        return text;
    }
    if (format==u"M" || format==u"m" || format==u"Y" || format==u"y") {
        ascii(text,months[m-1]); text.push_back(u' ');
        if (format==u"M" || format==u"m") digits(text,d);
        else digits(text,y,4);
        return text;
    }
    if (format==u"D" || format==u"F" || format==u"f") {
        ascii(text,weekdays[static_cast<std::size_t>((date.ticks/ticks_per_day+1)%7)]);
        text+=u", "; ascii(text,months[m-1]); text.push_back(u' '); digits(text,d);
        text+=u", "; digits(text,y,4);
        if (format!=u"D") {text.push_back(u' '); time(false,format==u"F");}
        return text;
    }
    if (format==u"d" || format==u"G" || format==u"g") {
        digits(text,m); text.push_back(u'/'); digits(text,d); text.push_back(u'/'); digits(text,y,4);
        if (format!=u"d") {text.push_back(u' '); time(false,format==u"G");}
        return text;
    }
    if (format==u"t" || format==u"T") {
        time(false,format==u"T");
        return text;
    }
    throw Error("E_FORMAT",location,"Unsupported DateTime format","Формат DateTime ещё не поддерживается");
}
}
bool calendar_record_shape(std::span<const TypeInfo> types,const TypeInfo& info) {
    const bool date=info.native_record==NativeRecord::date_time;
    if (!calendar_record(info) || info.kind!=CompoundKind::record || info.name!=(date?"datetime":"timespan")
        || info.fields.size()!=(date?2U:1U)) return false;
    const auto field=[](const Local& item,std::string_view name,Type type){
        return item.name==name && !item.global && item.type==type;
    };
    if (!field(info.fields[0],"_ticks",Type::int64)) return false;
    if (!date) return info.element==Type::invalid && info.key==Type::invalid;
    const auto span=find_type(types,info.element);
    return field(info.fields[1],"_kind",Type::integer) && span && span->native_record==NativeRecord::time_span
        && tagged_enum(find_type(types,info.key),"system.datetimekind",kinds);
}
bool calendar_signature(std::span<const TypeInfo> types,Type owner,std::size_t operation) {
    const auto info=find_type(types,owner);
    if (!info || !calendar_record_shape(types,*info)) return false;
    const auto call=method(operation);
    if (call>M::is_daylight_saving) return false;
    const bool date=info->native_record==NativeRecord::date_time;
    if (call==M::weekday) return date && (operation>>8U)<=0xffffU
        && tagged_enum(find_type(types,payload(operation)),"system.dayofweek",weekdays);
    if ((operation>>8U)!=0) return false;
    switch (call) {
    case M::from_ticks_kind: case M::date_parts: case M::date_clock: case M::date_clock_kind: case M::date_millis:
    case M::date_millis_kind: case M::now: case M::utc_now: case M::today: case M::specify_kind: case M::leap_year:
    case M::days_in_month: case M::year: case M::month: case M::day_of_year: case M::kind: case M::date:
    case M::time_of_day: case M::difference: case M::add_days: case M::add_hours: case M::add_minutes:
    case M::add_seconds: case M::add_milliseconds: case M::add_ticks: case M::add_months: case M::add_years:
    case M::short_date_text: case M::short_time_text: case M::long_date_text: case M::long_time_text:
    case M::to_local_time: case M::to_universal_time: case M::is_daylight_saving:
        return date;
    case M::span_clock: case M::span_days: case M::span_millis: case M::zero: case M::total_days:
    case M::total_hours: case M::total_minutes: case M::total_seconds: case M::total_milliseconds: case M::from_days:
    case M::from_hours: case M::from_minutes: case M::from_seconds: case M::from_milliseconds: case M::negate: case M::duration:
        return !date;
    default: return true;
    }
}
std::vector<Type> calendar_parameters(std::span<const TypeInfo> types,Type owner,std::size_t operation) {
    const auto& info=*find_type(types,owner);
    const auto interval_type=info.native_record==NativeRecord::date_time?info.element:owner;
    constexpr auto integer=Type::integer;
    constexpr auto tick_type=Type::int64;
    switch (method(operation)) {
    case M::parse: return {Type::string};
    case M::try_parse: return {Type::string,reference_type(owner)};
    case M::from_ticks: return {tick_type};
    case M::from_ticks_kind: return {tick_type,info.key};
    case M::date_parts: case M::span_clock: return {integer,integer,integer};
    case M::span_days: return {integer,integer,integer,integer};
    case M::span_millis: return {integer,integer,integer,integer,integer};
    case M::date_clock: return {integer,integer,integer,integer,integer,integer};
    case M::date_millis: return {integer,integer,integer,integer,integer,integer,integer};
    case M::date_clock_kind: return {integer,integer,integer,integer,integer,integer,info.key};
    case M::date_millis_kind: return {integer,integer,integer,integer,integer,integer,integer,info.key};
    case M::minimum: case M::maximum: case M::zero: case M::now: case M::utc_now: case M::today: return {};
    case M::leap_year: return {integer};
    case M::days_in_month: return {integer,integer};
    case M::specify_kind: return {owner,info.key};
    case M::from_days: case M::from_hours: case M::from_minutes: case M::from_seconds: case M::from_milliseconds:
        return {Type::real};
    case M::add_days: case M::add_hours: case M::add_minutes: case M::add_seconds: case M::add_milliseconds:
        return {owner,Type::real};
    case M::add_ticks: return {owner,tick_type};
    case M::add_months: case M::add_years: return {owner,integer};
    case M::add: case M::subtract: return {owner,interval_type};
    case M::difference: case M::compare: case M::compare_static: case M::equal: case M::unequal: case M::less:
    case M::less_equal: case M::greater: case M::greater_equal: case M::equals_static: return {owner,owner};
    case M::to_string_format: return {owner,Type::string};
    default: return {owner};
    }
}
Type calendar_result(std::span<const TypeInfo> types,Type owner,std::size_t operation) {
    const auto& info=*find_type(types,owner);
    switch (method(operation)) {
    case M::weekday: return payload(operation);
    case M::kind: return info.key;
    case M::time_of_day: case M::difference: return info.element;
    case M::ticks: return Type::int64;
    case M::year: case M::month: case M::day: case M::hour: case M::minute: case M::second: case M::millisecond:
    case M::day_of_year: case M::days_in_month: case M::hash: case M::compare: case M::compare_static:
        return Type::integer;
    case M::try_parse: case M::is_daylight_saving: case M::leap_year: case M::equal: case M::unequal:
    case M::less: case M::less_equal: case M::greater:
    case M::greater_equal: case M::equals_static: return Type::boolean;
    case M::total_days: case M::total_hours: case M::total_minutes: case M::total_seconds: case M::total_milliseconds:
        return Type::real;
    case M::to_string: case M::to_string_format: case M::short_date_text: case M::short_time_text:
    case M::long_date_text: case M::long_time_text: return Type::string;
    default: return owner;
    }
}
Value calendar_invoke(Values& values,Type owner,std::size_t operation,std::span<const Value> args) {
    const auto call=method(operation);
    const auto location=values.location;
    const auto& info=type_info(values.program,owner);
    const bool date=info.native_record==NativeRecord::date_time;
    const auto integer=[&](std::size_t i){return std::get<std::int32_t>(args[i]);};
    const auto kind=[&](std::size_t i){return valid_kind(std::get<EnumValue>(args[i]).value,location);};
    const auto ticks=[&](std::size_t i){return std::get<std::int64_t>(args[i]);};
    const auto real=[&](std::size_t i){return std::get<double>(args[i]);};
    const auto build=[&](std::int64_t value,std::int32_t kind_value=unspecified_kind){
        return make_calendar(values,owner,value,kind_value);
    };
    const auto from_date=[&](std::int64_t clock=0,std::int32_t date_kind=unspecified_kind){
        return build(date_only(integer(0),integer(1),integer(2),location)+clock,date_kind);
    };
    switch (call) {
    case M::parse: case M::try_parse: {
        const auto& text=string_text(args[0]);
        const auto result=date?parse_date(text,location):parse_span(text);
        if (call==M::parse) {
            if (!result) throw Error("E_FORMAT",location,date?"Invalid DateTime text":"Invalid TimeSpan text",
                date?"Некорректная строка DateTime":"Некорректная строка TimeSpan");
            return build(result->ticks,result->kind);
        }
        const auto output=result.value_or(CalendarValue{});
        values.write(std::get<std::shared_ptr<Cell>>(args[1]),build(output.ticks,output.kind));
        return result.has_value();
    }
    case M::from_ticks: return build(date?date_ticks(ticks(0),location):ticks(0));
    case M::from_ticks_kind: return build(date_ticks(ticks(0),location),kind(1));
    case M::date_parts: return from_date();
    case M::date_clock: return from_date(clock_ticks(integer(3),integer(4),integer(5),0,location));
    case M::date_clock_kind: return from_date(clock_ticks(integer(3),integer(4),integer(5),0,location),kind(6));
    case M::date_millis: return from_date(clock_ticks(integer(3),integer(4),integer(5),integer(6),location));
    case M::date_millis_kind: return from_date(clock_ticks(integer(3),integer(4),integer(5),integer(6),location),kind(7));
    case M::span_clock: return build(span_components(0,integer(0),integer(1),integer(2),0,location));
    case M::span_days: return build(span_components(integer(0),integer(1),integer(2),integer(3),0,location));
    case M::span_millis: return build(span_components(integer(0),integer(1),integer(2),integer(3),integer(4),location));
    case M::minimum: return build(date?0:INT64_MIN);
    case M::maximum: return build(date?date_max_ticks:INT64_MAX);
    case M::zero: return build(0);
    case M::now: case M::utc_now: case M::today: {
        const auto current=current_date(call,location);
        return build(current.ticks,current.kind);
    }
    case M::leap_year:
        date_only(integer(0),1,1,location);
        return year{integer(0)}.is_leap();
    case M::days_in_month: {
        const auto first=date_fields(date_only(integer(0),integer(1),1,location));
        return static_cast<std::int32_t>(static_cast<unsigned>((first.year()/first.month()/last).day()));
    }
    case M::from_days: case M::from_hours: case M::from_minutes: case M::from_seconds: case M::from_milliseconds: {
        constexpr std::array constants{ticks_per_day,ticks_per_hour,ticks_per_minute,ticks_per_second,ticks_per_millisecond};
        const auto index=static_cast<unsigned>(call)-static_cast<unsigned>(M::from_days);
        return build(interval(real(0),constants[index],false,location));
    }
    default: break;
    }
    const auto first=calendar_value(args[0]);
    const auto t=first.ticks;
    const auto shifted=[&](std::int64_t offset){return build(shift_date(t,offset,location),first.kind);};
    switch (call) {
    case M::specify_kind: return build(t,kind(1));
    case M::year: return static_cast<std::int32_t>(static_cast<int>(date_fields(t).year()));
    case M::month: return static_cast<std::int32_t>(static_cast<unsigned>(date_fields(t).month()));
    case M::day: return date?static_cast<std::int32_t>(static_cast<unsigned>(date_fields(t).day()))
        :static_cast<std::int32_t>(t/ticks_per_day);
    case M::hour: return static_cast<std::int32_t>(clock_fields(t).hour);
    case M::minute: return static_cast<std::int32_t>(clock_fields(t).minute);
    case M::second: return static_cast<std::int32_t>(clock_fields(t).second);
    case M::millisecond: return static_cast<std::int32_t>(clock_fields(t).millisecond);
    case M::weekday: return EnumValue{payload(operation),static_cast<std::int32_t>((t/ticks_per_day+1)%7)};
    case M::day_of_year: {
        const auto y=date_fields(t).year();
        return static_cast<std::int32_t>((epoch+days{t/ticks_per_day}-sys_days{y/January/1}).count()+1);
    }
    case M::kind: return EnumValue{info.key,std::min(first.kind,local_kind)};
    case M::date: return build(t-t%ticks_per_day,first.kind);
    case M::time_of_day: return make_calendar(values,info.element,t%ticks_per_day);
    case M::ticks: return t;
    case M::total_days: return static_cast<double>(t)*(1.0/ticks_per_day);
    case M::total_hours: return static_cast<double>(t)*(1.0/ticks_per_hour);
    case M::total_minutes: return static_cast<double>(t)*(1.0/ticks_per_minute);
    case M::total_seconds: return static_cast<double>(t)*(1.0/ticks_per_second);
    case M::total_milliseconds: {
        constexpr auto upper=static_cast<double>(INT64_MAX/ticks_per_millisecond);
        constexpr auto lower=static_cast<double>(INT64_MIN/ticks_per_millisecond);
        return std::clamp(static_cast<double>(t)*(1.0/ticks_per_millisecond),lower,upper);
    }
    case M::add_days: case M::add_hours: case M::add_minutes: case M::add_seconds: case M::add_milliseconds: {
        constexpr std::array constants{ticks_per_day,ticks_per_hour,ticks_per_minute,ticks_per_second,ticks_per_millisecond};
        const auto index=static_cast<unsigned>(call)-static_cast<unsigned>(M::add_days);
        return shifted(interval(real(1),constants[index],true,location));
    }
    case M::add_ticks: return shifted(ticks(1));
    case M::add_months: return build(add_months(t,integer(1),location),first.kind);
    case M::add_years: return build(add_months(t,static_cast<std::int64_t>(integer(1))*12,location),first.kind);
    case M::add: {
        const auto interval_ticks=calendar_value(args[1]).ticks;
        return date?shifted(interval_ticks):build(safe_add(t,interval_ticks,location));
    }
    case M::subtract: {
        const auto interval_ticks=calendar_value(args[1]).ticks;
        if (!date) return build(safe_subtract(t,interval_ticks,location));
        if (interval_ticks>t || interval_ticks<t-date_max_ticks) throw bad_date(location);
        return build(t-interval_ticks,first.kind);
    }
    case M::difference: return make_calendar(values,info.element,t-calendar_value(args[1]).ticks);
    case M::negate: return build(safe_subtract(0,t,location));
    case M::duration: return build(t<0?safe_subtract(0,t,location):t);
    case M::hash: return calendar_hash_code(args[0]);
    case M::compare: case M::compare_static: {
        const auto second=calendar_value(args[1]).ticks;
        return static_cast<std::int32_t>((t>second)-(t<second));
    }
    case M::equal: case M::equals_static: return t==calendar_value(args[1]).ticks;
    case M::unequal: return t!=calendar_value(args[1]).ticks;
    case M::less: return t<calendar_value(args[1]).ticks;
    case M::less_equal: return t<=calendar_value(args[1]).ticks;
    case M::greater: return t>calendar_value(args[1]).ticks;
    case M::greater_equal: return t>=calendar_value(args[1]).ticks;
    case M::to_string: return calendar_text(args[0],{},location);
    case M::to_string_format: return calendar_text(args[0],string_text(args[1]),location);
    case M::short_date_text: return date_text(first,u"d",location);
    case M::short_time_text: return date_text(first,u"t",location);
    case M::long_date_text: return date_text(first,u"D",location);
    case M::long_time_text: return date_text(first,u"T",location);
    case M::to_universal_time: {
        const auto converted=to_universal(first,location);
        return build(converted.ticks,converted.kind);
    }
    case M::to_local_time: {
        if (first.kind>=local_kind) return build(first.ticks,first.kind);
        const auto local=utc_to_local(first.ticks,location);
        if (!local) throw local_zone_error(location);
        return build(local->ticks,local->kind);
    }
    case M::is_daylight_saving: {
        if (first.kind==utc_kind) return false;
        const auto unix_time=local_to_unix(first);
        const auto local=unix_time?local_fields(*unix_time):std::nullopt;
        if (!local) throw local_zone_error(location);
        return local->tm_isdst>0;
    }
    default: throw runtime_error(RuntimeError::invalid_bytecode,location);
    }
}
std::u16string calendar_text(const Value& value,std::u16string_view format,SourceLocation location) {
    const auto& aggregate=std::get<std::shared_ptr<Aggregate>>(value);
    const auto data=calendar_value(value);
    return aggregate->items.size()==2?date_text(data,format,location):span_text(data.ticks,format,location);
}
int compare_calendar_values(const Value& left,const Value& right) {
    const auto a=calendar_value(left).ticks,b=calendar_value(right).ticks;
    return (a>b)-(a<b);
}
double calendar_ole_date(const Value& value,SourceLocation location) {
    using namespace std::chrono;
    constexpr auto offset_days=(sys_days{year{1899}/December/30}-epoch).count();
    constexpr auto minimum_days=(sys_days{year{100}/January/1}-epoch).count();
    constexpr auto millis_per_day=ticks_per_day/ticks_per_millisecond;
    const auto ticks=calendar_value(value).ticks;
    if(ticks>=ticks_per_day && ticks<minimum_days*ticks_per_day)
        throw Error("E_OVERFLOW",location,"DateTime is outside the OLE Automation date range",
            "Дата DateTime выходит за диапазон дат OLE Automation");
    const auto millis=(ticks<ticks_per_day ? ticks : ticks-offset_days*ticks_per_day)/ticks_per_millisecond;
    const auto day=(millis<0 ? millis-millis_per_day+1 : millis)/millis_per_day;
    const auto time=millis-day*millis_per_day;
    const auto encoded=day*millis_per_day+(day<0 ? -time : time);
    return static_cast<double>(encoded)/static_cast<double>(millis_per_day);
}
std::int32_t calendar_hash_code(const Value& value) {
    const auto bits=std::bit_cast<std::uint64_t>(calendar_value(value).ticks);
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(bits^(bits>>32)));
}
std::uint64_t date_time_binary_bits(const Value& value) {
    const auto date=calendar_value(value);
    return static_cast<std::uint64_t>(date.ticks)|(static_cast<std::uint64_t>(date.kind)<<62U);
}
Value date_time_from_binary_bits(Values& values,Type type,std::uint64_t data) {
    const auto ticks=date_ticks(static_cast<std::int64_t>(data&date_time_ticks_mask),values.location);
    return make_calendar(values,type,ticks,static_cast<std::int32_t>(data>>62U));
}
}
