#include "abc/core.hpp"
#include <iostream>
#include <sstream>

namespace {
int failures=0;
void require(bool condition,std::string_view message){
    if(!condition){++failures;std::cerr<<message<<'\n';}
}
template<class Action>void expect_error(Action action,std::string_view code){
    try{action();require(false,std::string("Expected ")+std::string(code));}
    catch(const abc::Error& error){
        require(error.code==code,std::string("Expected ")+std::string(code)+", got "+error.code);
        require(*error.what() && !error.russian.empty(),"Expected a bilingual diagnostic");
    }
}
void execute(std::string_view source){
    const auto program=abc::compile(source);
    std::istringstream input;std::ostringstream output;
    abc::execute(program,input,output);
}
}

int main(){
    expect_error([]{execute("type C=class function F:integer;virtual:=1;end;begin var instance:C;var callback:()->integer:=instance.F;end.");},"E_NIL");
    expect_error([]{execute("type C=class function F:integer:=1;end;begin var instance:C;var callback:()->integer:=instance.F;end.");},"E_ARGUMENT");
    for(const auto source:{
        "type C=class function F(n:integer):integer:=n;end;begin var instance:=new C;var callback:()->integer:=instance.F;end.",
        "type C=class function F:integer:=1;end;begin var instance:=new C;var callback:()->string:=instance.F;end.",
        "type C=class function F(n:integer:=7):integer:=n;end;begin var instance:=new C;var callback:()->integer:=instance.F;end.",
        "type A=abstract class function F:integer;abstract;end;B=class(A)function F:integer;override:=1;function Getter:()->integer:=inherited F;end;begin end.",
        "type C=class procedure P(var n:integer):=Inc(n);end;begin var instance:=new C;var callback:integer->():=instance.P;end."})
        expect_error([&]{(void)abc::compile(source);},"E_TYPE");
    auto program=abc::compile("type C=class function F:integer:=1;end;begin var instance:=new C;var callback:()->integer:=instance.F;Writeln(callback());end.");
    auto invalid_operand=program;
    for(auto& instruction:invalid_operand.code)if(instruction.op==abc::Op::require_method_target)instruction.operand=2;
    expect_error([&]{abc::verify(invalid_operand);},"E_BYTECODE");
    auto invalid_type=program;
    for(auto& instruction:invalid_type.code)if(instruction.op==abc::Op::require_method_target)instruction.type=abc::Type::integer;
    expect_error([&]{abc::verify(invalid_type);},"E_BYTECODE");
    if(!failures)std::cout<<"Bound method type, nil-target and bytecode checks passed\n";
    return failures ? 1 : 0;
}
