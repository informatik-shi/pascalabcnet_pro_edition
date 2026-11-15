#include "complex_number.hpp"
#include "composite_format.hpp"
#include "runtime_values.hpp"
#include <cmath>
#include <numbers>

namespace abc {
namespace {
using M=ComplexMethod;
struct Parts {double real,imaginary;};
M method(std::size_t operation) {return static_cast<M>(operation&255U);}
Type payload(std::size_t operation) {return static_cast<Type>(operation>>8U);}
Parts parts(const Value& input) {
    const auto& record=std::get<std::shared_ptr<Aggregate>>(input);
    return {std::get<double>(record->items[0]->value),std::get<double>(record->items[1]->value)};
}
Value value(Values& values,Type type,Parts data) {
    auto result=std::get<std::shared_ptr<Aggregate>>(values.zero(type,false,true));
    result->items[0]->value=data.real;
    result->items[1]->value=data.imaginary;
    return result;
}
Parts polar(double length,double phase) {
    return {length*std::cos(phase),length*std::sin(phase)};
}
double magnitude(Parts part) { return std::hypot(part.real,part.imaginary); }
double phase(Parts part) { return std::atan2(part.imaginary,part.real); }
Parts logarithm(Parts part) { return {std::log(magnitude(part)),phase(part)}; }
Parts divide(Parts left,Parts right) {
    // Scale by the larger divisor component to avoid squaring it.
    const auto a=left.real, b=left.imaginary, c=right.real, d=right.imaginary;
    if (std::abs(d)<std::abs(c)) {
        const auto ratio=d/c;
        const auto divisor=c+d*ratio;
        return {(a+b*ratio)/divisor,(b-a*ratio)/divisor};
    }
    const auto ratio=c/d;
    const auto divisor=d+c*ratio;
    return {(b+a*ratio)/divisor,(-a+b*ratio)/divisor};
}
bool zero(Parts part) {return part.real==0 && part.imaginary==0;}
Parts power(Parts base,Parts exponent) {
    if (zero(exponent)) return {1,0};
    if (zero(base)) return {0,0};
    const auto radius=magnitude(base);
    const auto angle=phase(base);
    const auto projected=exponent.real*angle+exponent.imaginary*std::log(radius);
    const auto scale=std::pow(radius,exponent.real)*std::exp(-exponent.imaginary*angle);
    return polar(scale,projected);
}
Parts from_tuple(const Value& input) {
    const auto& tuple=std::get<std::shared_ptr<Aggregate>>(input);
    const auto component=[&](std::size_t i) {
        const auto& item=tuple->items[i]->value;
        return std::holds_alternative<double>(item)?std::get<double>(item):static_cast<double>(std::get<std::int32_t>(item));
    };
    return {component(0),component(1)};
}
}

bool complex_signature(const Program& program,Type owner,std::size_t operation) {
    if (!compound(owner) || static_cast<unsigned>(owner)-64U>=program.types.size()
        || !complex_number_type(type_info(program,owner))) return false;
    if (method(operation)!=M::from_tuple) return method(operation)<=M::to_string && (operation>>8U)==0;
    if ((operation>>8U)>0xffffU) return false;
    const auto tuple=payload(operation);
    if (!compound(tuple) || static_cast<unsigned>(tuple)-64U>=program.types.size()) return false;
    const auto& shape=type_info(program,tuple);
    const auto component=[](const auto& item){return item.type==Type::real || item.type==Type::integer;};
    return shape.kind==CompoundKind::tuple && shape.fields.size()==2
        && component(shape.fields[0]) && component(shape.fields[1]);
}
std::vector<Type> complex_parameters(Type owner,std::size_t operation) {
    switch (method(operation)) {
    case M::zero: case M::one: case M::imaginary_one: return {};
    case M::construct: case M::from_polar: return {Type::real,Type::real};
    case M::from_real: return {Type::real};
    case M::from_tuple: return {payload(operation)};
    case M::logarithm_base: return {owner,Type::real};
    case M::add: case M::subtract: case M::multiply: case M::divide: case M::power:
    case M::equal: case M::not_equal: case M::equals: return {owner,owner};
    default: return {owner};
    }
}
Type complex_result(Type owner,std::size_t operation) {
    switch (method(operation)) {
    case M::real_part: case M::imaginary_part: case M::magnitude: case M::phase: return Type::real;
    case M::equal: case M::not_equal: case M::equals: return Type::boolean;
    case M::to_string: return Type::string;
    default: return owner;
    }
}
Value complex_invoke(Values& values,Type owner,std::size_t operation,std::span<const Value> args) {
    const auto call=method(operation);
    const auto build=[&](Parts result){return value(values,owner,result);};
    const auto real=[&](std::size_t i){return std::get<double>(args[i]);};
    switch (call) {
    case M::zero: return build({0,0});
    case M::one: return build({1,0});
    case M::imaginary_one: return build({0,1});
    case M::construct: return build({real(0),real(1)});
    case M::from_real: return build({real(0),0});
    case M::from_tuple: return build(from_tuple(args[0]));
    case M::from_polar: return build(polar(real(0),real(1)));
    default: break;
    }
    const auto a=parts(args[0]);
    switch (call) {
    case M::real_part: return a.real;
    case M::imaginary_part: return a.imaginary;
    case M::magnitude: return magnitude(a);
    case M::phase: return phase(a);
    case M::negate: return build({-a.real,-a.imaginary});
    case M::conjugate: return build({a.real,-a.imaginary});
    case M::reciprocal: return build(zero(a)?a:divide({1,0},a));
    case M::square_root: return build(polar(std::sqrt(magnitude(a)),phase(a)/2));
    case M::exponential: return build(polar(std::exp(a.real),a.imaginary));
    case M::logarithm: return build(logarithm(a));
    case M::logarithm_base: return build(divide(logarithm(a),logarithm({real(1),0})));
    case M::logarithm10: {
        const auto result=logarithm(a);
        return build({result.real*std::numbers::log10e,result.imaginary*std::numbers::log10e});
    }
    case M::sine: return build({std::sin(a.real)*std::cosh(a.imaginary),std::cos(a.real)*std::sinh(a.imaginary)});
    case M::cosine: return build({std::cos(a.real)*std::cosh(a.imaginary),-std::sin(a.real)*std::sinh(a.imaginary)});
    case M::to_string: {
        const auto location=values.location;
        return u"("+format_scalar(Value{a.real},location)+u", "+format_scalar(Value{a.imaginary},location)+u")";
    }
    default: break;
    }
    const auto b=parts(args[1]);
    switch (call) {
    case M::add: return build({a.real+b.real,a.imaginary+b.imaginary});
    case M::subtract: return build({a.real-b.real,a.imaginary-b.imaginary});
    case M::multiply: return build({a.real*b.real-a.imaginary*b.imaginary,a.real*b.imaginary+a.imaginary*b.real});
    case M::divide: return build(divide(a,b));
    case M::power: return build(power(a,b));
    case M::equal: return a.real==b.real && a.imaginary==b.imaginary;
    case M::not_equal: return a.real!=b.real || a.imaginary!=b.imaginary;
    case M::equals: {
        const auto equal=[](double x,double y){return x==y || (std::isnan(x) && std::isnan(y));};
        return equal(a.real,b.real) && equal(a.imaginary,b.imaginary);
    }
    default: throw runtime_error(RuntimeError::invalid_bytecode,values.location);
    }
}
std::u16string complex_pascal_text(const Value& input,SourceLocation location) {
    const auto part=parts(input);
    return format_scalar(Value{part.real},location)+(part.imaginary>=0?u"+":u"")
        +format_scalar(Value{part.imaginary},location)+u"i";
}
}
