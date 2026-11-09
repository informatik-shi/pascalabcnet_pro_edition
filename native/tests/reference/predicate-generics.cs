// CLR delegate identity/invocation oracle, with source-derived conversion guards.
using System;using System.IO;using System.Text;using System.Collections.Generic;using System.Linq;using System.Globalization;
#if THREE
using Pred=PABCSystem.Predicate3<int,string,bool>;
using Fun=System.Func<int,string,bool,bool>;
using OtherPred=PABCSystem.Predicate3<string,int,bool>;
#else
using Pred=PABCSystem.Predicate2<int,string>;
using Fun=System.Func<int,string,bool>;
using OtherPred=PABCSystem.Predicate2<string,int>;
#endif
[assembly:System.Reflection.AssemblyVersion("1.0.0.0")]
namespace PABCSystem{public delegate bool Predicate2<A,B>(A a,B b);public delegate bool Predicate3<A,B,C>(A a,B b,C c);}
#if UNICODE_MODULE
namespace Другой.Модуль{
#else
namespace Program{
#endif
class PredicateGenericsReference{
 struct Holder{public Pred Value;}class Box{public Fun Value;}
 static string text;static bool flag;static int calls,created,route;static Pred source,other;static Fun functionSource;
 #if THREE
 static bool Positive(int n,string w,bool b){++calls;return (n>=w.Length)==b;}
 static bool Negative(int n,string w,bool b){++calls;return (n<w.Length)==b;}
 static Pred Make(int value){return (n,w,b)=>{++calls;return (n>=value+w.Length)==b;};}
 static bool CallP(Pred p,int n){return p(n,text,flag);}static bool CallF(Fun f,int n){return f(n,text,flag);}
 const int Arity=3;
#else
 static bool Positive(int n,string w){++calls;return n>=w.Length;}
 static bool Negative(int n,string w){++calls;return n<w.Length;}
 static Pred Make(int value){return (n,w)=>{++calls;return n>=value+w.Length;};}
 static bool CallP(Pred p,int n){return p(n,text);}static bool CallF(Fun f,int n){return f(n,text);}
 const int Arity=2;
#endif
 static Fun ToF(Pred p){return p==null?null:new Fun(p.Invoke);}
 static Pred ToP(Fun f){return f==null?null:new Pred(f.Invoke);}
 static Pred NextP(){++created;Console.Write('N');if(route==1||route==2&&created==2)return null;if(route==3&&created==2)return other;return source;}
 static Fun NextF(){++created;Console.Write('N');if(route==1||route==2&&created==2)return null;return functionSource;}
 static void Run(int seed,int argument,int inputRoute,int op){
  calls=created=0;route=inputRoute;text=seed<0?"":seed==0?"abc":"Яx";flag=route%2==0;source=new Pred(Positive);other=new Pred(Positive);
  if(route==1)source=null;if(route==2){source=Make(seed);other=Make(seed);}if(route==3)source=Negative;
  switch(op){
   case 0:Console.WriteLine(typeof(Pred)==typeof(Fun));Console.WriteLine(new HashSet<Type>(new[]{typeof(Pred),typeof(Fun)}).Count);break;
   case 1:{var f=ToF(source);Console.WriteLine((source.GetType()==typeof(Pred))+" "+(f.GetType()==typeof(Fun)));Console.WriteLine(CallP(source,argument)+" "+CallF(f,argument)+" "+calls);break;}
   case 2:{var a=ToF(source);var b=ToF(source);var equal=EqualityComparer<Fun>.Default.Equals(a,b);Console.WriteLine(equal);Console.WriteLine(new HashSet<Fun>(new[]{a,b}).Count);Console.WriteLine(equal?(EqualityComparer<Fun>.Default.GetHashCode(a)==EqualityComparer<Fun>.Default.GetHashCode(b)).ToString():"-");break;}
   case 3:{var a=ToF(source);var b=ToF(other);Console.WriteLine(EqualityComparer<Pred>.Default.Equals(source,other));Console.WriteLine(EqualityComparer<Fun>.Default.Equals(a,b));Console.WriteLine(new HashSet<Fun>(new[]{a,b}).Count);break;}
   case 4:{var f=ToF(source);var back=ToP(f);Console.WriteLine(EqualityComparer<Pred>.Default.Equals(source,back)+" "+(back==null));if(back!=null)Console.WriteLine(CallP(back,argument)+" "+calls);break;}
   case 5:{var f=ToF(source);source=Negative;Console.WriteLine(CallF(f,argument)+" "+calls);break;}
   case 6:Console.WriteLine("["+string.Join(",",new[]{-2,0,3}.Where(n=>CallP(source,n)))+"]");Console.WriteLine(calls);break;
   case 7:Console.WriteLine(new List<Pred>(new[]{source}).Contains(other));Console.WriteLine(new HashSet<Pred>(new[]{source,other}).Count);break;
   // Original type_table.cs uses the source expression in both branches.
   case 8:{var f=NextP()==null?null:new Fun(NextP().Invoke);Console.WriteLine(" "+created+" "+(f==null));if(f!=null)Console.WriteLine(CallF(f,argument)+" "+calls);break;}
   case 9:{functionSource=ToF(source);created=0;var p=NextF()==null?null:new Pred(NextF().Invoke);Console.WriteLine(" "+created+" "+(p==null));if(p!=null)Console.WriteLine(CallP(p,argument)+" "+calls);break;}
   case 10:{Pred p=null;var f=ToF(p);var back=ToP(f);Console.WriteLine((f==null)+" "+(back==null));break;}
   case 11:{var f=ToF(source);Console.WriteLine((source.GetType()==typeof(Pred))+" "+(f.GetType()==typeof(Fun))+" "+(source.GetType()==f.GetType()));break;}
   case 12:{var f=ToF(source);var a=ToP(f);var b=ToP(f);Console.WriteLine(EqualityComparer<Pred>.Default.Equals(a,b)+" "+EqualityComparer<Pred>.Default.Equals(a,source));Console.WriteLine(new HashSet<Pred>(new[]{a,b,source}).Count);break;}
   case 14:foreach(var t in new[]{typeof(Pred),typeof(Fun),typeof(Pred[]),typeof(List<Pred>)}){Console.WriteLine(t.Name);Console.WriteLine(t.FullName);Console.WriteLine(t.Namespace);Console.WriteLine(t.AssemblyQualifiedName);}break;
   case 15:Console.WriteLine(typeof(OtherPred).Name);Console.WriteLine(typeof(OtherPred).FullName);Console.WriteLine(typeof(OtherPred).AssemblyQualifiedName);Console.WriteLine(new HashSet<Type>{typeof(Pred),typeof(OtherPred),typeof(Fun)}.Count);break;
   case 13:{var h=new Holder{Value=source};var b=new Box{Value=ToF(h.Value)};h.Value=null;Console.WriteLine(CallF(b.Value,argument)+" "+calls);break;}
  }
 }
 static string Hex(string text){if(text.Length==0)return "-";var result=new StringBuilder();foreach(char c in text.Replace("\r\n","\n"))result.Append(((int)c).ToString("x4"));return result.ToString();}
 static void Main(string[] args){CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;var console=Console.Out;using(var output=new StreamWriter(args[0],args.Length>1,new UTF8Encoding(false))){
  if(args.Length==1)output.WriteLine("# module\tarity\tseed\targument\troute\toperation\tUTF16_stdout\terror");
  foreach(int seed in new[]{-2,0,3})foreach(int argument in new[]{-2,0,3})for(int route=0;route<4;++route)for(int op=0;op<16;++op){
   var captured=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(captured);string error="ok";
   try{Run(seed,argument,route,op);}catch(NullReferenceException){error="E_NIL";}catch(ArgumentNullException){error="E_NIL";}catch(ArgumentException){error="E_ARGUMENT";}finally{Console.SetOut(console);}
   output.WriteLine(typeof(PredicateGenericsReference).Namespace+"\t"+Arity+"\t"+seed+"\t"+argument+"\t"+route+"\t"+op+"\t"+Hex(captured.ToString())+"\t"+error);
  }
 }}
}

}
