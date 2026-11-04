// Native port of bin/Lib/IniFile.pas.
// Original copyright (c) Ivan Bondarev, Stanislav Mikhalkovich.
// GNU LGPL; see doc/license.txt and doc/copyright.txt.
#pragma once
#include "abc/core.hpp"
#include <span>

namespace abc {
struct Values;class FileContext;
bool ini_signature(const Program& program,Type type,IniMethod method);
std::vector<Type> ini_parameters(Type type,IniMethod method);
Type ini_result(Type type,IniMethod method);
Value ini_invoke(Values& values,FileContext& files,Type type,IniMethod method,std::span<const Value> arguments,const ExecutionOptions& options);
} // namespace abc
