#include "abc/core.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc,char** argv){
    const bool setters=argc==2 && std::string_view(argv[1])=="--setters";
    if(argc!=1 && !setters){std::cerr<<"Unexpected conversion test arguments\n";return 1;}
    constexpr std::array<std::pair<std::string_view,std::string_view>,12> types{{
        {"boolean","True"},{"char","'A'"},{"shortint","shortint(1)"},{"byte","byte(1)"},
        {"smallint","smallint(1)"},{"word","word(1)"},{"integer","integer(1)"},
        {"longword","longword(1)"},{"int64","int64(1)"},{"uint64","uint64(1)"},
        {"single","single(1)"},{"real","real(1)"}}};
    std::ifstream reference(setters ? "reflection-value-widening.out" : "reflection-index-widening.out");
    if(!reference){std::cerr<<"Source property conversion reference was not found\n";return 1;}
    int failures=0,count=0;
    std::string line;
    while(std::getline(reference,line)){
        if(!line.empty() && line.back()=='\r')line.pop_back();
        const auto arrow=line.find('>'),separator=line.find('|');
        if(arrow==line.npos || separator==line.npos || arrow>=separator){++failures;continue;}
        const auto from=line.substr(0,arrow),target=line.substr(arrow+1,separator-arrow-1),expected=line.substr(separator+1);
        const auto input_type=std::find_if(types.begin(),types.end(),[&](const auto& item){return item.first==from;});
        const auto target_type=std::find_if(types.begin(),types.end(),[&](const auto& item){return item.first==target;});
        if(input_type==types.end() || target_type==types.end()){++failures;continue;}
        ++count;
        const auto index_source=std::string("program IndexConversionProbe;uses System;type C=class public function Get(n:")
            +target+"):"+target+":=n;property Item[index:"+target+"]:"+target
            +" read Get;end;begin var instance:=new C;var indices:=new object[1];indices[0]:="
            +std::string(input_type->second)+";Writeln(typeof(C).GetProperty('Item').GetValue(instance,indices).GetType.Name);end.";
        const auto value_source=std::string("program PropertyValueConversionProbe;uses System;type C=class public stored:")
            +target+";property Item:"+target+" read stored write stored;end;begin var instance:=new C;"
            +"var value:Object:="+std::string(input_type->second)+";typeof(C).GetProperty('Item').SetValue(instance,value);"
            +"Writeln(typeof(C).GetProperty('Item').GetValue(instance).GetType.Name);end.";
        try{
            const auto program=abc::compile(setters ? value_source : index_source);
            std::istringstream input;std::ostringstream output;
            abc::execute(program,input,output);
            if(expected=="ArgumentException" || output.str()!=expected+"\n"){
                ++failures;std::cerr<<line<<" -> "<<output.str();
            }
        }catch(const abc::Error& error){
            if(expected!="ArgumentException" || error.code!="E_ARGUMENT" || error.russian.empty()){
                ++failures;std::cerr<<line<<" -> "<<error.code<<": "<<error.what()<<'\n';
            }
        }
    }
    if(count!=144){++failures;std::cerr<<"Expected 144 source conversions, read "<<count<<'\n';}
    std::cout<<count<<" source "<<(setters ? "property value" : "index")<<" conversions checked, "<<failures<<" failures\n";
    return failures ? 1 : 0;
}
