// CLR delegate identity/invocation oracle, with source-derived conversion guards.
using System;using System.IO;using System.Text;using System.Collections.Generic;using System.Linq;using System.Globalization;
class CallableFamilyReference{
 struct Holder{public Predicate<int> Value;}class Box{public Func<int,bool> Value;}
 static int calls,created,route;static Predicate<int> source,other;static Func<int,bool> functionSource;
 static bool Positive(int n){++calls;return n>=0;}static bool Negative(int n){++calls;return n<0;}
 static Predicate<int> Make(int value){return n=>{++calls;return n>=value;};}
 static Func<int,bool> ToF(Predicate<int> p){return p==null?null:new Func<int,bool>(p.Invoke);}
 static Predicate<int> ToP(Func<int,bool> f){return f==null?null:new Predicate<int>(f.Invoke);}
 static Predicate<int> NextP(){++created;Console.Write('N');if(route==1||route==2&&created==2)return null;if(route==3&&created==2)return other;return source;}
 static Func<int,bool> NextF(){++created;Console.Write('N');if(route==1||route==2&&created==2)return null;return functionSource;}
 static void Run(int seed,int argument,int inputRoute,int op){
  calls=created=0;route=inputRoute;source=new Predicate<int>(Positive);other=new Predicate<int>(Positive);
  if(route==1)source=null;if(route==2){source=Make(seed);other=Make(seed);}if(route==3)source=Negative;
  switch(op){
   case 0:Console.WriteLine(typeof(Predicate<int>)==typeof(Func<int,bool>));Console.WriteLine(new HashSet<Type>(new[]{typeof(Predicate<int>),typeof(Func<int,bool>)}).Count);break;
   case 1:{var f=ToF(source);Console.WriteLine((source.GetType()==typeof(Predicate<int>))+" "+(f.GetType()==typeof(Func<int,bool>)));Console.WriteLine(source(argument)+" "+f(argument)+" "+calls);break;}
   case 2:{var a=ToF(source);var b=ToF(source);var equal=EqualityComparer<Func<int,bool>>.Default.Equals(a,b);Console.WriteLine(equal);Console.WriteLine(new HashSet<Func<int,bool>>(new[]{a,b}).Count);Console.WriteLine(equal?(EqualityComparer<Func<int,bool>>.Default.GetHashCode(a)==EqualityComparer<Func<int,bool>>.Default.GetHashCode(b)).ToString():"-");break;}
   case 3:{var a=ToF(source);var b=ToF(other);Console.WriteLine(EqualityComparer<Predicate<int>>.Default.Equals(source,other));Console.WriteLine(EqualityComparer<Func<int,bool>>.Default.Equals(a,b));Console.WriteLine(new HashSet<Func<int,bool>>(new[]{a,b}).Count);break;}
   case 4:{var f=ToF(source);var back=ToP(f);Console.WriteLine(EqualityComparer<Predicate<int>>.Default.Equals(source,back)+" "+(back==null));if(back!=null)Console.WriteLine(back(argument)+" "+calls);break;}
   case 5:{var f=ToF(source);source=Negative;Console.WriteLine(f(argument)+" "+calls);break;}
   case 6:Console.WriteLine("["+string.Join(",",new[]{-2,0,3}.Where(ToF(source)))+"]");Console.WriteLine(calls);break;
   case 7:Console.WriteLine(new List<Predicate<int>>(new[]{source}).Contains(other));Console.WriteLine(new HashSet<Predicate<int>>(new[]{source,other}).Count);break;
   // Original type_table.cs uses the source expression in both branches.
   case 8:{var f=NextP()==null?null:new Func<int,bool>(NextP().Invoke);Console.WriteLine(" "+created+" "+(f==null));if(f!=null)Console.WriteLine(f(argument)+" "+calls);break;}
   case 9:{functionSource=ToF(source);created=0;var p=NextF()==null?null:new Predicate<int>(NextF().Invoke);Console.WriteLine(" "+created+" "+(p==null));if(p!=null)Console.WriteLine(p(argument)+" "+calls);break;}
   case 10:{Predicate<int> p=null;var f=ToF(p);var back=ToP(f);Console.WriteLine((f==null)+" "+(back==null));break;}
   case 11:{var f=ToF(source);Console.WriteLine((source.GetType()==typeof(Predicate<int>))+" "+(f.GetType()==typeof(Func<int,bool>))+" "+(source.GetType()==f.GetType()));break;}
   case 12:{var f=ToF(source);var a=ToP(f);var b=ToP(f);Console.WriteLine(EqualityComparer<Predicate<int>>.Default.Equals(a,b)+" "+EqualityComparer<Predicate<int>>.Default.Equals(a,source));Console.WriteLine(new HashSet<Predicate<int>>(new[]{a,b,source}).Count);break;}
   case 13:{var h=new Holder{Value=source};var b=new Box{Value=ToF(h.Value)};h.Value=null;Console.WriteLine(b.Value(argument)+" "+calls);break;}
  }
 }
 static string Hex(string text){if(text.Length==0)return "-";var result=new StringBuilder();foreach(char c in text.Replace("\r\n","\n"))result.Append(((int)c).ToString("x4"));return result.ToString();}
 static void Main(string[] args){CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;var console=Console.Out;using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){
  output.WriteLine("# seed\targument\troute\toperation\tUTF16_stdout\terror");
  foreach(int seed in new[]{-2,0,3})foreach(int argument in new[]{-2,0,3})for(int route=0;route<4;++route)for(int op=0;op<14;++op){
   var captured=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(captured);string error="ok";
   try{Run(seed,argument,route,op);}catch(NullReferenceException){error="E_NIL";}catch(ArgumentNullException){error="E_NIL";}catch(ArgumentException){error="E_ARGUMENT";}finally{Console.SetOut(console);}
   output.WriteLine(seed+"\t"+argument+"\t"+route+"\t"+op+"\t"+Hex(captured.ToString())+"\t"+error);
  }
 }}
}
