#include "abc/core.hpp"
#include "../src/reflection.hpp"
#include <iostream>
#include <sstream>

namespace {
int failures=0;
void require(bool condition,std::string_view message) {
    if(!condition){++failures;std::cerr<<message<<'\n';}
}
template<class Action> void expect_error(Action action,std::string_view code) {
    try{action();require(false,std::string("Expected ")+std::string(code));}
    catch(const abc::Error& error){
        require(error.code==code,std::string("Expected ")+std::string(code)+", got "+error.code);
        require(!error.russian.empty(),"Diagnostic has no Russian text");
    }
}
void execute(std::string_view source) {
    const auto program=abc::compile(source);
    std::istringstream input;
    std::ostringstream output;
    abc::execute(program,input,output);
}
}

int main() {
    for(const auto source:{
        "uses System.Reflection;begin var p:PropertyInfo;Writeln(p.Name);end.",
        "uses Countries;begin Writeln(typeof(Country).GetProperty('missing').Name);end."})
        expect_error([&]{execute(source);},"E_NIL");
    expect_error([]{execute("uses Countries;begin typeof(Country).GetProperty(nil);end.");},"E_ARGUMENT");
    expect_error([]{execute("uses Countries;begin typeof(Country).GetProperty('Название').GetValue(nil);end.");},"E_TARGET");
    expect_error([]{execute("uses Countries;begin typeof(Country).GetProperty('Название').GetValue(4);end.");},"E_TARGET");
    expect_error([]{execute("uses Countries;begin typeof(Country).GetProperty('Название').GetCustomAttributes(typeof(integer),False);end.");},"E_ARGUMENT");
    expect_error([]{execute("uses Countries;begin typeof(Country).GetProperty('Название').IsDefined(nil,False);end.");},"E_NIL");
    expect_error([]{execute("uses Countries;begin var c:=new Country('A','B',1,'C');var a:=new object[1];typeof(Country).GetProperty('Название').GetValue(c,a);end.");},"E_ARGUMENT");
    for(const auto source:{
        "uses Countries;begin var a:Att;Writeln(a.GetHashCode);end.",
        "uses Countries;begin var a:Att;Writeln(a.Equals(nil));end.",
        "uses Countries;begin var a:Att;Writeln(a.Match(nil));end.",
        "uses Countries;begin var a:Att;Writeln(a.TypeId);end.",
        "uses System;begin var a:Attribute;Writeln(a.IsDefaultAttribute);end."})
        expect_error([&]{execute(source);},"E_NIL");
    for(const auto source:{
        "uses System.Reflection;begin var p:=new PropertyInfo;end.",
        "uses Countries;begin typeof(Country).GetProperty(1);end.",
        "uses Countries;begin typeof(Country).GetProperties(1);end.",
        "uses Countries;begin var a:=new Att(1);Writeln(a.TypeId());end.",
        "uses Countries;begin var a:=new Att(1);Writeln(a.GetHashCode(1));end.",
        "uses Countries;begin var p:=typeof(Country).GetProperty('Название');p.Name:='changed';end.",
        "uses Countries;begin Writeln(typeof(Country).GetProperty('Название').Name());end."})
        expect_error([&]{(void)abc::compile(source);},"E_TYPE");
    auto program=abc::compile("uses Countries;begin Writeln(typeof(Country).GetProperty('Название').Name);end.");
    auto invalid_operation=program;
    for(auto& instruction:invalid_operation.code)if(instruction.op==abc::Op::reflection_call)instruction.operand=32768;
    expect_error([&]{abc::verify(invalid_operation);},"E_BYTECODE");
    auto invalid_shape=program;
    for(auto& type:invalid_shape.types)if(type.name==abc::property_info_class_name)type.fields[1].type=abc::Type::string;
    expect_error([&]{abc::verify(invalid_shape);},"E_BYTECODE");
    auto writable_metadata=program;
    for(auto& instruction:writable_metadata.code)if(instruction.op==abc::Op::reflection_call){
        instruction.op=abc::Op::ref_field;instruction.operand=0;break;
    }
    expect_error([&]{abc::verify(writable_metadata);},"E_BYTECODE");
    if(!failures)std::cout<<"Property reflection diagnostics and bytecode validation passed\n";
    return failures ? 1 : 0;
}
