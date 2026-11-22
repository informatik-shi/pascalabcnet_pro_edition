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
    expect_error([]{execute("type C=class public static property Number:integer read 1;end;begin typeof(C).GetProperty('Number').GetValue(nil,new object[1]);end.");},"E_ARGUMENT");
    expect_error([]{execute("type C=class public static x:integer;static property Number:integer write x;end;begin typeof(C).GetProperty('Number').GetValue(nil);end.");},"E_ARGUMENT");
    auto ordered=abc::compile("type C=class public property First:integer read 1;static property Second:integer read 2;end;begin end.");
    require(ordered.instance_properties.size()==1 && ordered.static_properties.size()==1,"Mixed property metadata was not preserved");
    if(!ordered.instance_properties.empty() && !ordered.static_properties.empty()){
        auto duplicate_order=ordered;
        duplicate_order.static_properties[0].declaration_order=duplicate_order.instance_properties[0].declaration_order;
        expect_error([&]{abc::verify(duplicate_order);},"E_BYTECODE");
    }
    expect_error([]{execute("type A=class public property Number:integer read 1;end;B=class(A)public property Number:string read 'two';end;begin typeof(B).GetProperty('Number');end.");},"E_AMBIGUOUS");
    expect_error([]{execute("type A=class public property Number:integer read 1;end;B=class(A)end;begin typeof(B).GetProperty('Number').GetValue(4);end.");},"E_TARGET");
    // Obtaining a typed loader must not read its data file before invocation.
    execute("uses Countries;begin var loader:()->array of Country:=ЗаполнитьМассивСтран;end.");
    execute("uses Countries,System;begin var loader:System.Func<array of Country>:=Countries.ЗаполнитьМассивСтран;end.");
    for(const auto source:{
        "uses Countries;begin var loader:()->integer:=ЗаполнитьМассивСтран;end.",
        "uses Countries;begin var loader:(integer)->array of Country:=Countries.ЗаполнитьМассивСтран;end.",
        "uses Countries;begin var loader:()->array of integer:=Countries.ЗаполнитьМассивСтран;end."})
        expect_error([&]{(void)abc::compile(source);},"E_TYPE");
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
    for(const auto source:{
        "uses Countries;type C=class public x:integer;[Att('bad')]property Number:integer read x;end;begin end.",
        "uses Countries;type C=class public x:integer;[Att(1),Att(2)]property Number:integer read x;end;begin end.",
        "uses Countries;type C=class public x:integer;[Att()]property Number:integer read x;end;begin end.",
        "uses Countries;type C=class public x:integer;[Att(Random(10))]property Number:integer read x;end;begin end."})
        expect_error([&]{(void)abc::compile(source);},"E_TYPE");
    expect_error([]{execute("type C=class public x:integer;property Number:integer write x;end;begin typeof(C).GetProperty('Number').GetValue(nil);end.");},"E_ARGUMENT");
    auto annotated=abc::compile("uses Countries;type C=class public x:integer;[Att(1)]property Number:integer read x;end;begin end.");
    require(annotated.instance_properties.size()==1,"Instance property metadata was not preserved");
    if(!annotated.instance_properties.empty()){
        auto invalid_getter=annotated;
        invalid_getter.instance_properties[0].getter=invalid_getter.routines.size();
        expect_error([&]{abc::verify(invalid_getter);},"E_BYTECODE");
        auto invalid_result=annotated;
        invalid_result.instance_properties[0].type=abc::Type::real;
        expect_error([&]{abc::verify(invalid_result);},"E_BYTECODE");
        auto invalid_attribute=annotated;
        invalid_attribute.instance_properties[0].attributes[0].arguments[0]=std::u16string{u"bad"};
        expect_error([&]{abc::verify(invalid_attribute);},"E_BYTECODE");
    }
    if(!failures)std::cout<<"Property reflection diagnostics and bytecode validation passed\n";
    return failures ? 1 : 0;
}
