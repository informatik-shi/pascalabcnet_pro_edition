// Independent development-only explicit dispatch recipes; never installed.
using System;using System.IO;using System.Text;
interface IA{int F(int n);string F(string s);Func<int,int> Make(int seed);}
interface IB{int F(int n);void Change(ref int n);int Sum(params int[] a);}
interface IChild:IA{}
interface IResolved:IA,IB{new int F(int n);}
class C:IChild,IB,IResolved{
 int value;public C(int initial){value=initial;}
 public int F(int n){Console.Write("P");return 100+n;}
 int IA.F(int n){Console.Write("A");return n+(++value);}
 string IA.F(string s){Console.Write("S");return s+"!";}
 Func<int,int> IA.Make(int seed){Console.Write("M");return n=>F(n)+seed+value;}
 int IB.F(int n){Console.Write("B");return 200+n+value;}
 void IB.Change(ref int n){Console.Write("R");n+=++value;}
 int IB.Sum(params int[] a){Console.Write("T");int total=value;foreach(int n in a)total+=n;return total;}
 int IResolved.F(int n){Console.Write("Z");return 300+n+value;}
}
class Reference{
 static int Next(ref int n){Console.Write("X");return ++n;}
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("X4"));return b.ToString();}
 static void Run(int seed,int route,int nil){
  var c=new C(seed);var other=new C(seed+10);IChild a=c;IB b=c;
  if(route==1)a=other;else if(route==2)a=(IChild)b;if(nil!=0)a=null;
  Console.WriteLine(c.F(2));IResolved resolved=c;Console.WriteLine(resolved.F(1));int n=seed;
  Console.WriteLine(a.F(Next(ref n)));Console.WriteLine(a.F("cat"));Console.WriteLine(b.F(n));
  b.Change(ref n);Console.WriteLine(n);
  int first=b.Sum(),second=b.Sum(2,3),third=b.Sum(new[]{4,5});Console.WriteLine(first+" "+second+" "+third);
  var f=a.Make(seed);Console.WriteLine(f(7));a.F(1);c=null;other=null;Console.WriteLine(f(8));
 }
 static void Main(string[] args){var original=Console.Out;int count=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# seed route nil output_utf16 error; Framework "+Environment.Version);
  foreach(int seed in new[]{-2,0,3,9})for(int route=0;route<3;route++)for(int nil=0;nil<2;nil++){
   var output=new StringWriter();Console.SetOut(output);string error="ok";try{Run(seed,route,nil);}catch(NullReferenceException){error="E_NIL";}finally{Console.SetOut(original);}
   file.WriteLine(seed+"\t"+route+"\t"+nil+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);count++;
  }
 }Console.WriteLine(count+" independent explicit-interface outcomes");}
}
