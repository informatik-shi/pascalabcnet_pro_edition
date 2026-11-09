#include "abc/core.hpp"
#include "../src/type_names.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view label){++checks;if(!ok){++failures;if(failures<=30)std::cerr<<"FAIL "<<label<<'\n';}}
std::vector<std::string> split(std::string_view text){std::vector<std::string> fields;std::size_t begin=0;for(;;){const auto end=text.find('\t',begin);fields.emplace_back(text.substr(begin,end==text.npos ? end : end-begin));if(end==text.npos)return fields;begin=end+1;}}
std::string decode(std::string_view hex){if(hex=="-")return {};std::u16string value;for(std::size_t n=0;n<hex.size();n+=4)value.push_back(static_cast<char16_t>(std::stoul(std::string(hex.substr(n,4)),nullptr,16)));return abc::to_utf8(value);}
std::string run(std::string_view source,std::string_view module="Program"){
    auto program=abc::compile(source,module);std::istringstream input;std::ostringstream output;abc::execute(program,input,output);return output.str();
}
void reference(){
    const auto fixtures=std::filesystem::path(abc::from_utf8(__FILE__)).parent_path()/"fixtures";
    const std::string prelude="uses Collections,School,IniFile;type MiXeDRecord=record X:integer;end;MiXeDClass=class end;IFace=interface end;Цвет=(Красный,Синий);";
    for(bool modules:{false,true}){
    std::ifstream data(fixtures/(modules ? "type-module-reference.tsv" : "type-properties-reference.tsv"));check(bool(data),"Type property reference exists");std::string row;int rows=0;
    while(std::getline(data,row)){
        if(!row.empty() && row.back()=='\r')row.pop_back();if(row.empty() || row.front()=='#')continue;
        const auto f=split(row);if(f.size()!=5){check(false,"Malformed metadata row");continue;}
        std::string error="ok";std::ostringstream output;std::istringstream input;
        try{
            const auto program=abc::compile(prelude+"begin Write('before');var t:="+f[1]+";var s:=t."+f[2]+";if s=nil then Write('nil') else Write(s);end.",modules ? decode(f[0]) : f[0]);
            abc::execute(program,input,output);
        }catch(const abc::Error& e){error=e.code;check(!e.russian.empty() && *e.what() && e.location.line>0,"Bilingual metadata error with position");}
        const auto expected=std::string("before")+(f[4]=="ok" ? f[3]=="nil" ? "nil" : decode(f[3]) : "");
        check(error==f[4] && output.str()==expected,f[0]+" "+f[1]+"."+f[2]+" actual "+error+" "+output.str());++rows;
    }
    check(rows==(modules ? 72 : 736),"All metadata outcomes checked");
    }
}
void validation(){
    check(run("type R=record end;begin Writeln(TypeToTypeName(typeof(integer)),' ',typeof(integer).Name);Writeln(typeof(R).FullName);end.","Свой.Модуль")=="integer Int32\nСвой.Модуль.R\n","Raw and Pascal type names remain distinct");
    check(run("begin var t:=typeof(List<integer>);Writeln(t.Name,' ',t.GetType.Name);end.")=="List`1 RuntimeType\n","Type values retain their own runtime Type identity");
    for(const auto& source:{"begin typeof(integer).Name:='x';end.","begin Writeln(typeof(integer).Name(1));end.","begin Writeln(1.Namespace);end."}){
        try{abc::compile(source);check(false,"Invalid property use accepted");}catch(const abc::Error& e){check(!e.russian.empty(),"Invalid property use rejected before execution");}
    }
    auto program=abc::compile("begin Writeln(typeof(integer).Name);end.");
    for(unsigned mode=0;mode<5;++mode){
        auto bad=program;
        if(mode==0)bad.module_name.clear();
        if(mode==1)bad.module_name=std::string("a\0b",3);
        if(mode==2)bad.module_name=std::string(1,static_cast<char>(0xff));
        if(mode>=3)for(auto& instruction:bad.code)if(instruction.op==abc::Op::type_operation){if(mode==3)instruction.type=abc::Type::integer;else instruction.operand=999;}
        try{abc::verify(bad);check(false,"Forged metadata accepted");}catch(const abc::Error& e){check(e.code=="E_BYTECODE" && !e.russian.empty(),"Forged metadata rejected");}
    }
    try{abc::compile("begin end.","");check(false,"Empty module name accepted");}catch(const abc::Error& e){check(e.code=="E_TYPE" && !e.russian.empty(),"Invalid module identity rejected");}
    const abc::ReflectedType descriptor{abc::Type::integer};
    for(unsigned mode=0;mode<3;++mode){
        abc::ExecutionOptions options;if(mode==0)options.instruction_limit=1;if(mode==1)options.allocation_limit=0;if(mode==2)options.cancelled=[] {return true;};
        try{abc::reflected_type_property(program,descriptor,abc::TypeOperation::assembly_qualified_name,{},options);check(false,"Metadata work limit ignored");}
        catch(const abc::Error& e){check(e.code==(mode==0 ? "E_LIMIT" : mode==1 ? "E_LIMIT" : "E_CANCELLED") && !e.russian.empty(),"Metadata work limit and cancellation");}
    }
    std::string source="type T0=integer;";
    for(unsigned i=1;i<=15;++i)source+="T"+std::to_string(i)+"=(T"+std::to_string(i-1)+",T"+std::to_string(i-1)+");";
    source+="begin Writeln(typeof(T15).AssemblyQualifiedName);end.";
    try{run(source);check(false,"Exponential generic name output accepted");}catch(const abc::Error& e){check(e.code=="E_LIMIT" && std::string_view(e.what()).starts_with("String exceeds") && !e.russian.empty(),"Generic metadata string limit enforced");}
}
}
int main(){try{reference();validation();}catch(const std::exception& e){check(false,e.what());}std::cout<<checks<<" type property checks, "<<failures<<" failures\n";return failures?1:0;}
