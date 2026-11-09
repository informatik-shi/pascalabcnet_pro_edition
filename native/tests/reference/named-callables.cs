// CLR delegate identity/invocation oracle, with source-derived conversion guards.
using System;using System.IO;using System.Text;using System.Collections.Generic;using System.Linq;using System.Globalization;
[assembly:System.Reflection.AssemblyVersion("1.0.0.0")]
#if UNICODE_MODULE
namespace Другой.Модуль{
#else
namespace Program{
#endif
public delegate bool MiXeDP(int n);public delegate bool OtherF(int n);
public delegate void RefP(ref int n);public delegate void RefF(ref int n);
public delegate void ProcP();public delegate void ProcF();
class NamedCallableReference{
 struct Holder{public MiXeDP Value;}class Box{public OtherF Value;}
 static int calls,created,route;static MiXeDP source,other;static OtherF functionSource;
 static bool Positive(int n){++calls;return n>=0;}static bool Negative(int n){++calls;return n<0;}
 static void Bump(ref int n){++calls;++n;}static void Pulse(){++calls;}
 static MiXeDP Make(int value){return n=>{++calls;return n>=value;};}
 static OtherF ToF(MiXeDP p){return p==null?null:new OtherF(p.Invoke);}
 static MiXeDP ToP(OtherF f){return f==null?null:new MiXeDP(f.Invoke);}
 static MiXeDP NextP(){++created;Console.Write('N');if(route==1||route==2&&created==2)return null;if(route==3&&created==2)return other;return source;}
 static OtherF NextF(){++created;Console.Write('N');if(route==1||route==2&&created==2)return null;return functionSource;}
 static void Run(int seed,int argument,int inputRoute,int op){
  calls=created=0;route=inputRoute;source=new MiXeDP(Positive);other=new MiXeDP(Positive);
  if(route==1)source=null;if(route==2){source=Make(seed);other=Make(seed);}if(route==3)source=Negative;
  switch(op){
   case 0:Console.WriteLine(typeof(MiXeDP)==typeof(OtherF));Console.WriteLine(new HashSet<Type>(new[]{typeof(MiXeDP),typeof(OtherF)}).Count);break;
   case 1:{var f=ToF(source);Console.WriteLine((source.GetType()==typeof(MiXeDP))+" "+(f.GetType()==typeof(OtherF)));Console.WriteLine(source(argument)+" "+f(argument)+" "+calls);break;}
   case 2:{var a=ToF(source);var b=ToF(source);var equal=EqualityComparer<OtherF>.Default.Equals(a,b);Console.WriteLine(equal);Console.WriteLine(new HashSet<OtherF>(new[]{a,b}).Count);Console.WriteLine(equal?(EqualityComparer<OtherF>.Default.GetHashCode(a)==EqualityComparer<OtherF>.Default.GetHashCode(b)).ToString():"-");break;}
   case 3:{var a=ToF(source);var b=ToF(other);Console.WriteLine(EqualityComparer<MiXeDP>.Default.Equals(source,other));Console.WriteLine(EqualityComparer<OtherF>.Default.Equals(a,b));Console.WriteLine(new HashSet<OtherF>(new[]{a,b}).Count);break;}
   case 4:{var f=ToF(source);var back=ToP(f);Console.WriteLine(EqualityComparer<MiXeDP>.Default.Equals(source,back)+" "+(back==null));if(back!=null)Console.WriteLine(back(argument)+" "+calls);break;}
   case 5:{var f=ToF(source);source=Negative;Console.WriteLine(f(argument)+" "+calls);break;}
   case 6:Console.WriteLine("["+string.Join(",",new[]{-2,0,3}.Where(source==null?null:new Func<int,bool>(source.Invoke)))+"]");Console.WriteLine(calls);break;
   case 7:Console.WriteLine(new List<MiXeDP>(new[]{source}).Contains(other));Console.WriteLine(new HashSet<MiXeDP>(new[]{source,other}).Count);break;
   // Original type_table.cs uses the source expression in both branches.
   case 8:{var f=NextP()==null?null:new OtherF(NextP().Invoke);Console.WriteLine(" "+created+" "+(f==null));if(f!=null)Console.WriteLine(f(argument)+" "+calls);break;}
   case 9:{functionSource=ToF(source);created=0;var p=NextF()==null?null:new MiXeDP(NextF().Invoke);Console.WriteLine(" "+created+" "+(p==null));if(p!=null)Console.WriteLine(p(argument)+" "+calls);break;}
   case 10:{MiXeDP p=null;var f=ToF(p);var back=ToP(f);Console.WriteLine((f==null)+" "+(back==null));break;}
   case 11:{var f=ToF(source);Console.WriteLine((source.GetType()==typeof(MiXeDP))+" "+(f.GetType()==typeof(OtherF))+" "+(source.GetType()==f.GetType()));break;}
   case 12:{var f=ToF(source);var a=ToP(f);var b=ToP(f);Console.WriteLine(EqualityComparer<MiXeDP>.Default.Equals(a,b)+" "+EqualityComparer<MiXeDP>.Default.Equals(a,source));Console.WriteLine(new HashSet<MiXeDP>(new[]{a,b,source}).Count);break;}
   case 13:{var h=new Holder{Value=source};var b=new Box{Value=ToF(h.Value)};h.Value=null;Console.WriteLine(b.Value(argument)+" "+calls);break;}
   case 14:foreach(var t in new[]{typeof(MiXeDP),typeof(OtherF),typeof(RefP),typeof(RefF),typeof(ProcP),typeof(ProcF)}){Console.WriteLine(t.Name);Console.WriteLine(t.FullName);Console.WriteLine(t.Namespace);Console.WriteLine(t.AssemblyQualifiedName);}break;
   case 15:Console.WriteLine(typeof(MiXeDP)==typeof(Func<int,bool>));Console.WriteLine(typeof(Func<int,bool>)==typeof(Func<int,bool>));Console.WriteLine(typeof(Func<int,bool>).Name);Console.WriteLine(new HashSet<Type>{typeof(MiXeDP),typeof(OtherF),typeof(Func<int,bool>)}.Count);Console.WriteLine(typeof(MiXeDP[]).Name);Console.WriteLine(typeof(List<MiXeDP>).FullName);break;
   case 16:{RefP p=Bump;RefF f=p==null?null:new RefF(p.Invoke);int n=argument;f(ref n);RefP back=f==null?null:new RefP(f.Invoke);Console.WriteLine(n+" "+calls+" "+(back==p));Console.WriteLine(f.GetType().Name+" "+(f.GetType()==typeof(RefF)));RefP nil=null;RefF empty=nil==null?null:new RefF(nil.Invoke);Console.WriteLine(empty==null);break;}
   case 17:{ProcP p=Pulse,q=Pulse;ProcF a=new ProcF(p.Invoke),b=new ProcF(p.Invoke),c=new ProcF(q.Invoke);a();Console.WriteLine(calls+" "+(a==b)+" "+(a==c));Console.WriteLine(new HashSet<ProcF>{a,b,c}.Count);Console.WriteLine(a.GetType().Name);break;}
  }
 }
 static string Hex(string text){if(text.Length==0)return "-";var result=new StringBuilder();foreach(char c in text.Replace("\r\n","\n"))result.Append(((int)c).ToString("x4"));return result.ToString();}
 static void Main(string[] args){CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;var console=Console.Out;using(var output=new StreamWriter(args[0],args.Length>1,new UTF8Encoding(false))){
  if(args.Length==1)output.WriteLine("# module\tseed\targument\troute\toperation\tUTF16_stdout\terror");
  foreach(int seed in new[]{-2,0,3})foreach(int argument in new[]{-2,0,3})for(int route=0;route<4;++route)for(int op=0;op<18;++op){
   var captured=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(captured);string error="ok";
   try{Run(seed,argument,route,op);}catch(NullReferenceException){error="E_NIL";}catch(ArgumentNullException){error="E_NIL";}catch(ArgumentException){error="E_ARGUMENT";}finally{Console.SetOut(console);}
   output.WriteLine(typeof(MiXeDP).Namespace+"\t"+seed+"\t"+argument+"\t"+route+"\t"+op+"\t"+Hex(captured.ToString())+"\t"+error);
  }
 }}
}

}
