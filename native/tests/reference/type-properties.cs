// Development-only CLR metadata oracle. Never invokes the Pascal compiler.
using System;using System.IO;using System.Text;using System.Collections.Generic;
using System.Reflection;using System.Reflection.Emit;using System.Numerics;
class TypePropertiesReference{
 static string Hex(string value){if(value==null)return "nil";if(value.Length==0)return "-";var b=new StringBuilder();foreach(char c in value)b.Append(((int)c).ToString("x4"));return b.ToString();}
 static void Modules(string path){using(var output=new StreamWriter(path,false,new UTF8Encoding(false))){
  output.WriteLine("# module_UTF16_hex\tPascal_expression\tproperty\tUTF16_hex\terror");
  foreach(var unit in new[]{"Simple module","Space end ","Comma,Module","Equals=Module","Quote'Module","Quote\"Module","Both'\"Module","Bracket[Module]","Plus+Module"}){
   var assembly=AppDomain.CurrentDomain.DefineDynamicAssembly(new AssemblyName{Name=unit,Version=new Version("1.0.0.0")},AssemblyBuilderAccess.Run);
   var record=Shape(assembly.DefineDynamicModule(unit),unit+".MiXeDRecord",0,true);
   foreach(var type in new[]{record,typeof(List<>).MakeGenericType(record)})foreach(var property in new[]{"Name","FullName","Namespace","AssemblyQualifiedName"})
    output.WriteLine(Hex(unit)+"\t"+(type==record ? "typeof(MiXeDRecord)" : "typeof(List<MiXeDRecord>)")+"\t"+property+"\t"+Hex((string)typeof(Type).GetProperty(property).GetValue(type,null))+"\tok");
  }
 }}
 static Type Shape(ModuleBuilder module,string name,int arity,bool value=false,bool face=false){
  var attributes=TypeAttributes.Public|(face?TypeAttributes.Interface|TypeAttributes.Abstract:value?TypeAttributes.Sealed|TypeAttributes.SequentialLayout:0);
  var type=module.DefineType(name,attributes,face?null:value?typeof(ValueType):typeof(object));
  if(arity>0){var names=new string[arity];for(int n=0;n<arity;++n)names[n]="T"+n;type.DefineGenericParameters(names);}
  return type.CreateType();
 }
 static void Main(string[] args){using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){
  output.WriteLine("# module\tPascal_expression\tproperty\tUTF16_hex\terror");
  foreach(string unit in new[]{"Program","Другой.Модуль"}){
   var assembly=AppDomain.CurrentDomain.DefineDynamicAssembly(new AssemblyName{Name=unit,Version=new Version("1.0.0.0")},AssemblyBuilderAccess.Run);
   var module=assembly.DefineDynamicModule(unit);
   var record=Shape(module,unit+".MiXeDRecord",0,true);var box=Shape(module,unit+".MiXeDClass",0);var face=Shape(module,unit+".IFace",0,false,true);
   var colors=module.DefineEnum(unit+".Цвет",TypeAttributes.Public,typeof(int));colors.DefineLiteral("Красный",0);colors.DefineLiteral("Синий",1);var color=colors.CreateType();
   var types=new List<KeyValuePair<string,Type>>();
   Action<string,Type> add=(expression,type)=>types.Add(new KeyValuePair<string,Type>(expression,type));
   add("typeof(integer)",typeof(int));add("typeof(int64)",typeof(long));add("typeof(real)",typeof(double));add("typeof(single)",typeof(float));add("typeof(boolean)",typeof(bool));add("typeof(char)",typeof(char));add("typeof(string)",typeof(string));
   add("typeof(byte)",typeof(byte));add("typeof(shortint)",typeof(sbyte));add("typeof(word)",typeof(ushort));add("typeof(smallint)",typeof(short));add("typeof(longword)",typeof(uint));add("typeof(uint64)",typeof(ulong));add("typeof(BigInteger)",typeof(BigInteger));
   add("typeof(System.Type)",typeof(Type));add("typeof(System.Void)",typeof(void));add("typeof(System.Text.Encoding)",typeof(Encoding));
   add("typeof(MiXeDRecord)",record);add("typeof(MiXeDClass)",box);add("typeof(IFace)",face);add("typeof(Цвет)",color);
   add("typeof(array of integer)",typeof(int[]));add("typeof(array[,] of real)",typeof(double[,]));add("typeof(array of array[,] of char)",typeof(char).MakeArrayType(2).MakeArrayType());add("typeof(array of MiXeDRecord)",record.MakeArrayType());
   add("typeof(List<integer>)",typeof(List<int>));add("typeof(List<array of string>)",typeof(List<string[]>));add("typeof(List<MiXeDRecord>)",typeof(List<>).MakeGenericType(record));add("typeof(array of List<integer>)",typeof(List<int>[]));
   add("typeof(HashSet<integer>)",typeof(HashSet<int>));add("typeof(Dictionary<integer,string>)",typeof(Dictionary<int,string>));add("typeof(Dictionary<string,List<MiXeDRecord>>)",typeof(Dictionary<,>).MakeGenericType(typeof(string),typeof(List<>).MakeGenericType(record)));
   add("typeof(KeyValuePair<integer,string>)",typeof(KeyValuePair<int,string>));add("typeof(ICollection<integer>)",typeof(ICollection<int>));add("typeof(IList<string>)",typeof(IList<string>));add("typeof(ISet<integer>)",typeof(ISet<int>));add("typeof(IDictionary<integer,string>)",typeof(IDictionary<int,string>));
   add("typeof(sequence of integer)",typeof(IEnumerable<int>));add("typeof(IEnumerator<string>)",typeof(IEnumerator<string>));add("typeof(IComparable<integer>)",typeof(IComparable<int>));add("typeof(IEquatable<integer>)",typeof(IEquatable<int>));
   add("typeof(IComparer<integer>)",typeof(IComparer<int>));add("typeof(Comparer<integer>)",typeof(Comparer<int>));add("typeof(IEqualityComparer<string>)",typeof(IEqualityComparer<string>));add("typeof(EqualityComparer<string>)",typeof(EqualityComparer<string>));
   add("typeof((integer,string))",typeof(Tuple<int,string>));add("typeof((integer,string,real))",typeof(Tuple<int,string,double>));
   add("typeof(Func<integer,string>)",typeof(Func<int,string>));add("typeof(Func0<integer>)",typeof(Func<int>));add("typeof(Func2<integer,string,boolean>)",typeof(Func<int,string,bool>));add("typeof(Action0)",typeof(Action));add("typeof(Action<integer>)",typeof(Action<int>));add("typeof(Predicate<integer>)",typeof(Predicate<int>));
   foreach(var item in new[]{new[]{"Collections.Stack`1","Stack<integer>"},new[]{"Collections.Queue`1","Queue<integer>"},new[]{"Collections.DynArray`1","DynArray<integer>"},new[]{"Collections.SimpleSet`1","SimpleSet<integer>"},new[]{"Collections.LinkedList`1","LinkedList<integer>"},new[]{"Collections.LinkedListNode`1","LinkedListNode<integer>"},new[]{"Collections.SingleNode`1","SingleNode<integer>"},new[]{"PABCSystem.NewSet`1","set of integer"}})add("typeof("+item[1]+")",Shape(module,item[0],1).MakeGenericType(typeof(int)));
   add("typeof(AssocArray<string,integer>)",Shape(module,"Collections.AssocArray`2",2).MakeGenericType(typeof(string),typeof(int)));
   foreach(var item in new[]{new[]{"PABCSystem.Text","Text"},new[]{"PABCSystem.IntRange","IntRange"},new[]{"PABCSystem.CharRange","CharRange"},new[]{"PABCSystem.RealRange","RealRange"},new[]{"PABCSystem.EmptyCollection","EmptyCollection"},new[]{"IniFile.TIniFile","TIniFile"},new[]{"School.Addr32","Addr32"},new[]{"School.CalcIP","CalcIP"}})add("typeof("+item[1]+")",Shape(module,item[0],0,item[1].EndsWith("Range")));
   add("new List<integer>().GetType",new List<int>().GetType());add("new List<integer>().GetEnumerator.GetType",new List<int>().GetEnumerator().GetType());add("new HashSet<integer>().GetEnumerator.GetType",new HashSet<int>().GetEnumerator().GetType());
   add("new Dictionary<integer,string>().GetEnumerator.GetType",new Dictionary<int,string>().GetEnumerator().GetType());add("new Dictionary<integer,string>().Keys.GetType",new Dictionary<int,string>().Keys.GetType());add("new Dictionary<integer,string>().Values.GetType",new Dictionary<int,string>().Values.GetType());
   add("new Dictionary<integer,string>().Keys.GetEnumerator.GetType",new Dictionary<int,string>().Keys.GetEnumerator().GetType());add("new Dictionary<integer,string>().Values.GetEnumerator.GetType",new Dictionary<int,string>().Values.GetEnumerator().GetType());
   add("typeof(integer).GetType",typeof(int).GetType());add("Encoding.UTF8.GetType",Encoding.UTF8.GetType());add("Encoding.Unicode.GetType",Encoding.Unicode.GetType());add("Encoding.UTF32.GetType",Encoding.UTF32.GetType());add("Encoding.ASCII.GetType",Encoding.ASCII.GetType());add("Encoding.GetEncoding(1251).GetType",Encoding.GetEncoding(1251).GetType());
   add("Comparer<integer>.Default.GetType",Comparer<int>.Default.GetType());add("Comparer<string>.Default.GetType",Comparer<string>.Default.GetType());add("Comparer<MiXeDRecord>.Default.GetType",typeof(Comparer<>).MakeGenericType(record).GetProperty("Default").GetValue(null,null).GetType());
   add("EqualityComparer<integer>.Default.GetType",EqualityComparer<int>.Default.GetType());add("EqualityComparer<byte>.Default.GetType",EqualityComparer<byte>.Default.GetType());add("EqualityComparer<Цвет>.Default.GetType",typeof(EqualityComparer<>).MakeGenericType(color).GetProperty("Default").GetValue(null,null).GetType());add("EqualityComparer<MiXeDRecord>.Default.GetType",typeof(EqualityComparer<>).MakeGenericType(record).GetProperty("Default").GetValue(null,null).GetType());
   add("default(System.Type)",null);
   foreach(var item in types)foreach(string property in new[]{"Name","FullName","Namespace","AssemblyQualifiedName"}){
    string value=null,error="ok";try{if(property=="Name")value=item.Value.Name;else if(property=="FullName")value=item.Value.FullName;else if(property=="Namespace")value=item.Value.Namespace;else value=item.Value.AssemblyQualifiedName;}catch(NullReferenceException){error="E_NIL";}
    output.WriteLine(unit+"\t"+item.Key+"\t"+property+"\t"+Hex(value)+"\t"+error);
   }
  }
 }Modules(args[1]);}
}
