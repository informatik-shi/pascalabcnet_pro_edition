// Independent mutable struct/reference/boxing semantics on Framework; development only.
using System;using System.IO;using System.Text;using System.Collections.Generic;
interface ICounter{int Value{get;set;}int Add(int n);void Replace(int n);}
interface ITrace:ICounter{int Calls{get;}bool SameValue(Counter other);ICounter Rebox();}
struct Counter:ITrace,IEquatable<Counter>{
 int x,ticks;public Counter(int n){x=n;ticks=0;}
 public int Value{get{return x;}set{x=value;ticks++;}}public int Calls{get{return ticks;}}
 int ICounter.Value{get{return x+100;}set{x=value-100;ticks++;}}
 public int this[int i]{get{return x+i;}set{x=value-i;ticks++;}}
 public int Add(int n){x+=n;ticks++;return x;}int ICounter.Add(int n){x+=n*2;ticks++;return x+100;}
 public void Replace(int n){this=new Counter(n);}void ICounter.Replace(int n){this=new Counter(n+1);}
 public void Change(ref int n){n+=x;x++;ticks++;}
 public bool Equals(Counter other){ticks++;return x==other.x;}
 public static bool operator==(Counter a,Counter b){return a.x==b.x&&a.ticks==b.ticks;}public static bool operator!=(Counter a,Counter b){return !(a==b);}
 public bool SameValue(Counter other){return this==other;}public ICounter Rebox(){return this;}
}
class Holder{public Counter Data;public Holder(int n){Data=new Counter(n);}}
class Fake:ITrace{int x,ticks;public Fake(int n){x=n;}public int Value{get{return x+200;}set{x=value-200;ticks++;}}public int Calls{get{return ticks;}}public int Add(int n){x+=n*3;ticks++;return x+200;}public void Replace(int n){x=n+2;ticks=0;}public bool SameValue(Counter other){return false;}public ICounter Rebox(){return this;}}
class Reference{
 static void State(Counter c){Console.WriteLine(c.Value+" "+c.Calls);}static void CopyChange(Counter c,int n){c.Add(n);State(c);}static void RefChange(ref Counter c,int n){c.Add(n);}static Counter Copy(Counter c){return c;}
 static void Run(int seed,int n,int route,int op){
  var c=new Counter(seed);var copied=c;ICounter a=c,b=a,fresh=c;if(route==1)a=null;if(route==2)a=fresh;if(route==3)a=new Counter(seed+1);if(route==4)a=new Fake(seed);
  Console.WriteLine(object.ReferenceEquals(a,b)+" "+object.ReferenceEquals(a,fresh));
  switch(op){
   case 0:c.Add(n);State(c);State(copied);break;
   case 1:CopyChange(c,n);State(c);break;
   case 2:RefChange(ref c,n);State(c);break;
   case 3:Console.WriteLine(Copy(c).Add(n));State(c);break;
   case 4:var array=new[]{c};array[0].Add(n);State(array[0]);State(c);break;
   case 5:var holder=new Holder(seed);holder.Data.Add(n);State(holder.Data);break;
   case 6:c.Value+=n;State(c);break;
   case 7:c[n]+=n;State(c);break;
   case 8:a.Value+=n;Console.WriteLine(a.Value+" "+b.Value+" "+fresh.Value);break;
   case 9:Console.WriteLine(a.Add(n));Console.WriteLine(a.Value+" "+b.Value+" "+fresh.Value);State(c);break;
   case 10:a.Replace(n);Console.WriteLine(a.Value+" "+b.Value+" "+((ITrace)a).Calls);break;
   case 11:var unboxed=(Counter)(object)a;unboxed.Add(n);State(unboxed);Console.WriteLine(a.Value);State(c);break;
   case 12:var trace=(ITrace)a;Console.WriteLine(trace.Value+" "+trace.Calls+" "+object.ReferenceEquals(a,trace));break;
   case 13:var eq=(IEquatable<Counter>)a;Console.WriteLine(eq.Equals(c));Console.WriteLine(EqualityComparer<Counter>.Default.Equals(c,new Counter(seed+n))+" "+c.Calls);Console.WriteLine(((ITrace)a).Calls);break;
   case 14:var list=new List<Counter>{c};Console.WriteLine(list.Contains(new Counter(seed+n)));State(list[0]);State(c);break;
   case 15:var comp=EqualityComparer<ICounter>.Default;var equal=comp.Equals(a,fresh);Console.WriteLine(equal+" "+(equal?(comp.GetHashCode(a)==comp.GetHashCode(fresh)).ToString():"-"));break;
   case 16:var slots=new[]{a,b,fresh};Console.WriteLine(slots[0].Add(n));Console.WriteLine(slots[0].Value+" "+slots[1].Value+" "+slots[2].Value);break;
   case 17:c.Replace(n);State(c);State(copied);break;
   case 18:var value=n;c.Change(ref value);Console.WriteLine(value);State(c);break;
   case 19:var records=new List<Counter>{c};records[0].Add(n);State(records[0]);State(c);break;
   case 20:Console.WriteLine(((ITrace)a).SameValue(c));break;
   case 21:var again=((ITrace)a).Rebox();Console.WriteLine(object.ReferenceEquals(a,again));again.Add(n);Console.WriteLine(a.Value+" "+again.Value);break;
  }
 }
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("X4"));return b.ToString();}
 static void Main(string[] args){var original=Console.Out;int rows=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# seed amount route operation output_utf16 error; Framework "+Environment.Version);
  foreach(int seed in new[]{-1,0,2,5})foreach(int n in new[]{-2,0,3})for(int route=0;route<5;route++)for(int op=0;op<22;op++){
   var output=new StringWriter();Console.SetOut(output);string error="ok";try{Run(seed,n,route,op);}catch(NullReferenceException){error="E_NIL";}catch(InvalidCastException){error="E_CAST";}finally{Console.SetOut(original);}
   file.WriteLine(seed+"\t"+n+"\t"+route+"\t"+op+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
  }
 }Console.WriteLine(rows+" independent record method outcomes");if(args.Length>1)RecordCollectionReference.Generate(args[1]);if(args.Length>2)RecordListAliasReference.Generate(args[2]);}
}
