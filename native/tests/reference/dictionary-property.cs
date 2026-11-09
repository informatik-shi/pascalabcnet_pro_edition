// Development-only independent lowering of the shipped Pascal property assignment.
using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Globalization;
class Scenario<T>{
 public Dictionary<string,T> A,B,Root;public int ReceiverMode,KeyMode,RhsMode,ReceiverCalls,KeyCalls;public Func<T> Value;public Action<Scenario<T>> Mutate;
 public Dictionary<string,T> Receiver(){ReceiverCalls++;Console.Write("D"+ReceiverCalls+";");if(ReceiverMode==0)return Root;if(ReceiverMode==1)return ReceiverCalls==1?A:B;if(ReceiverMode==2)return ReceiverCalls==1?B:A;if(ReceiverMode==3)return ReceiverCalls==1?null:A;if(ReceiverMode==4)return ReceiverCalls==1?A:null;return null;}
 public string Key(){KeyCalls++;Console.Write("K"+KeyCalls+";");if(KeyMode==0)return "a";if(KeyMode==1)return KeyCalls==1?"a":"b";if(KeyMode==2)return KeyCalls==1?"b":"a";if(KeyMode==3)return KeyCalls==1?null:"a";if(KeyMode==4)return KeyCalls==1?"a":null;if(KeyMode==5)return KeyCalls==1?"missing":"a";if(KeyMode==6)return KeyCalls==1?"a":"missing";return "missing";}
 void Effect(){Console.Write("R;");if(RhsMode==1)Root=B;else if(RhsMode==2)A.Clear();else if(RhsMode==3)Mutate(this);}
 public T Rhs(){Effect();return Value();}public int Count(){Effect();return 2;}
}
class Reference {
 static string Hex(string value){if(value.Length==0)return "-";var text=new StringBuilder();foreach(char c in value)text.Append(((int)c).ToString("X4"));return text.ToString();}
 static string Error(Exception e){if(e is NullReferenceException||e is ArgumentNullException)return "E_NIL";if(e is KeyNotFoundException)return "E_KEY_NOT_FOUND";if(e is ArgumentException)return "E_RANGE";if(e is OverflowException)return "E_OVERFLOW";throw e;}
 static string Text(object value){if(value==null)return "nil";var items=value as IEnumerable<int>;if(items!=null)return "["+string.Join(",",items)+"]";return Convert.ToString(value,CultureInfo.InvariantCulture);}
 static string DictionaryText<T>(Dictionary<string,T> d){return "{"+string.Join(",",d.Select(p=>"("+p.Key+","+Text(p.Value)+")"))+"}";}
 static void Set<T>(Dictionary<string,T> d,string key,T value){d[key]=value;}
 static void Set(Dictionary<int,int> d,int key,int value){d[key]=value;}
 static void Set(List<int> d,int key,int value){d[key]=value;}
 sealed class Holder {public int Value{get;set;}}
 static int AddOne(int x){return x+1;}
 static string LambdaCase(int family,int capture){
  var callbacks=new List<Func<int,int>>();int bias=7;Func<Func<int,int>> factory=capture!=0?(Func<Func<int,int>>)(()=>x=>x+bias):(()=>AddOne);int value;
  if(family==0){var d=new Dictionary<int,int>{{1,2}};Func<Func<int,int>,Dictionary<int,int>> receiver=f=>{callbacks.Add(f);return d;};Set(receiver(factory()),1,receiver(factory())[1]+5);value=d[1];}
  else if(family==1){var d=new List<int>{2};Func<Func<int,int>,List<int>> receiver=f=>{callbacks.Add(f);return d;};Set(receiver(factory()),0,receiver(factory())[0]+5);value=d[0];}
  else{var d=new Holder{Value=2};Func<Func<int,int>,Holder> receiver=f=>{callbacks.Add(f);return d;};var outer=receiver(factory());outer.Value=receiver(factory()).Value+5;value=d.Value;}
  return value+" "+callbacks.Count+" "+callbacks[0].Equals(callbacks[1])+" "+callbacks[0](1)+"\n";
 }
 static Scenario<T> Make<T>(T a1,T a2,T b1,T b2,T seedValue,int rm,int km,int alias,int rhs,int seed,Func<T> value,Action<Scenario<T>> mutate){var s=new Scenario<T>{A=new Dictionary<string,T>{{"a",a1},{"b",a2}},B=new Dictionary<string,T>{{"a",b1},{"b",b2}},ReceiverMode=rm,KeyMode=km,RhsMode=rhs,Value=value,Mutate=mutate};if(seed!=0)s.A["a"]=seedValue;if(alias!=0)s.B=s.A;s.Root=s.A;return s;}
 static T[] Add<T>(T[] a,T[] b){if(a==null||b==null)throw new ArgumentNullException();var result=new T[a.Length+b.Length];Array.Copy(a,result,a.Length);Array.Copy(b,0,result,a.Length,b.Length);return result;}
 static int[] Repeat(int[] a,int n){if(a==null)throw new ArgumentNullException();var result=new int[a.Length*n];for(int i=0;i<n;i++)Array.Copy(a,0,result,i*a.Length,a.Length);return result;}
 static List<int> Add(List<int> a,List<int> b){if(a==null||b==null)throw new ArgumentNullException();var result=new List<int>(a);result.AddRange(b);return result;}
 static List<int> Repeat(List<int> a,int n){if(a==null)throw new ArgumentNullException();var result=new List<int>();for(int i=0;i<n;i++)result.AddRange(a);return result;}
 static string Repeat(string s,int n){var result=new StringBuilder();for(int i=0;i<n;i++)result.Append(s);return result.ToString();}
 static void Run<T>(Scenario<T> s,Func<T,T,T> combine,Func<T,int,T> repeat,bool count){
  // Source Assign.cs places receiver/key expressions in both setter and getter.
  if(count)Set(s.Receiver(),s.Key(),repeat(s.Receiver()[s.Key()],s.Count()));
  else Set(s.Receiver(),s.Key(),combine(s.Receiver()[s.Key()],s.Rhs()));
  Console.WriteLine("|"+DictionaryText(s.A)+"|"+DictionaryText(s.B)+"|"+object.ReferenceEquals(s.Root,s.B));
 }
 static void Execute(int kind,int op,int rm,int km,int alias,int rhs,int seed){
  if(kind==0){var s=Make(2,3,20,30,0,rm,km,alias,rhs,seed,()=>5,x=>x.A["a"]=99);Run(s,(x,y)=>op==0?checked(x+y):op==1?checked(x-y):checked(x*y),null,false);}
  else if(kind==1){var s=Make(2.5,3.0,20.5,30.0,0.0,rm,km,alias,rhs,seed,()=>2.0,x=>x.A["a"]=99.0);Run(s,(x,y)=>op==0?x+y:op==1?x-y:op==2?x*y:x/y,null,false);}
  else if(kind==2){var s=Make("x","y","u","v",(string)null,rm,km,alias,rhs,seed,()=>"z",x=>x.A["a"]="Q");Run(s,(x,y)=>x+y,Repeat,op==2);}
  else if(kind==3){var s=Make(new[]{1,2},new[]{3,4},new[]{5,6},new[]{7,8},(int[])null,rm,km,alias,rhs,seed,()=>new[]{9},x=>x.A["a"][0]=99);Run(s,Add,Repeat,op==2);}
  else{var s=Make(new List<int>{1,2},new List<int>{3,4},new List<int>{5,6},new List<int>{7,8},(List<int>)null,rm,km,alias,rhs,seed,()=>new List<int>{9},x=>x.A["a"].Add(99));Run(s,Add,Repeat,op==2);}
 }
 static void Main(string[] args){CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;var original=Console.Out;int rows=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# kind op receiver key alias rhs seed output_utf16 error; Framework "+Environment.Version);
  for(int kind=0;kind<5;kind++)foreach(int op in kind==0?new[]{0,1,2}:kind==1?new[]{0,1,2,3}:new[]{0,2})for(int rm=0;rm<6;rm++)for(int km=0;km<8;km++)for(int alias=0;alias<2;alias++)for(int rhs=0;rhs<4;rhs++)for(int seed=0;seed<2;seed++){
   var output=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(output);string error="ok";try{Execute(kind,op,rm,km,alias,rhs,seed);}catch(Exception e){error=Error(e);}Console.SetOut(original);file.WriteLine(kind+"\t"+op+"\t"+rm+"\t"+km+"\t"+alias+"\t"+rhs+"\t"+seed+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
  }
 }Console.WriteLine(rows+" independent dictionary property outcomes");if(args.Length>1){using(var file=new StreamWriter(args[1],false,new UTF8Encoding(false))){for(int family=0;family<3;family++)for(int capture=0;capture<2;capture++)file.WriteLine(family+"\t"+capture+"\t"+Hex(LambdaCase(family,capture)));}Console.WriteLine("6 independent property lambda identity outcomes");}}
}
