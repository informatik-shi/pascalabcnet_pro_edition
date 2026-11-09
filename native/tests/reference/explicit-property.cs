// Independent Framework property dispatch recipes; development-only.
using System;using System.IO;using System.Text;
interface IA{int Value{get;set;}int this[int i]{get;set;}bool Flag{get;}int Computed{get;}Func<int,int> Fn{get;}int Enumerator{get;}string KeyCollection{get;}}
interface IB{int Value{get;set;}}
interface IChild:IA{}
interface IResolved:IA,IB{new int Value{get;set;}}
class C:IChild,IB,IResolved{
 int a,b,plain,resolved;public C(int seed){a=seed;b=seed+1;plain=seed+2;resolved=seed+3;}
 public int Value{get{return plain;}set{plain=value;}}public int Forward{get{return 40;}}
 int GetA(){Console.Write("A");return a;}void SetA(int n){Console.Write("a");a=n;}
 int GetResolved(){Console.Write("C");return resolved;}void SetResolved(int n){Console.Write("c");resolved=n;}
 int IA.Value{get{return GetA();}set{SetA(value);}}int IB.Value{get{return b;}set{b=value;}}int IResolved.Value{get{return GetResolved();}set{SetResolved(value);}}
 int IA.this[int i]{get{return Reference.Trace("G",a+i);}set{Console.Write("S");a=value-i;}}
 bool IA.Flag{get{return true;}}int IA.Computed{get{return Reference.Trace("Q",Forward+a+Reference.Bias);}}
 Func<int,int> IA.Fn{get{return n=>n+a+Reference.Bias;}}int IA.Enumerator{get{return 7;}}string IA.KeyCollection{get{return "keys";}}
}
class Reference{
 public static int Bias;public static int Trace(string tag,int value){Console.Write(tag);return value;}
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("X4"));return b.ToString();}
 static void Run(int seed,int route,int nil){
  Bias=seed+2;var c=new C(seed);var other=new C(seed+10);IChild a=c;IB b=c;IResolved resolved=c;
  if(route==1)a=other;else if(route==2)a=(IChild)b;if(nil!=0)a=null;
  Console.Write("Start");resolved.Value=seed+20;
  Console.WriteLine(c.Value+" "+a.Value+" "+b.Value+" "+resolved.Value+" "+a.Flag+" "+a.Enumerator+" "+a.KeyCollection);
  a[2]=a[2]+5;a.Value=a.Value+a[1];var f=a.Fn;
  int first=a.Value,computed=a.Computed,index=a[2],callback=f(4);Console.WriteLine(first+" "+computed+" "+index+" "+callback);
  Bias++;a=null;b=null;resolved=null;c=null;other=null;Console.WriteLine(f(5));
 }
 static void Main(string[] args){var original=Console.Out;int rows=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# seed route nil output_utf16 error; Framework "+Environment.Version);
  foreach(int seed in new[]{-2,0,3,9})for(int route=0;route<3;route++)for(int nil=0;nil<2;nil++){
   var output=new StringWriter();Console.SetOut(output);string error="ok";try{Run(seed,route,nil);}catch(NullReferenceException){error="E_NIL";}finally{Console.SetOut(original);}
   file.WriteLine(seed+"\t"+route+"\t"+nil+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
  }
 }Console.WriteLine(rows+" independent explicit-property outcomes");}
}
