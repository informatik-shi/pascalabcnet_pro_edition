using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
public static class CollectionEnumeratorReference {
 static StreamWriter writer;
 static List<int> list;
 static HashSet<int> set;
 static List<int>.Enumerator[] ls;
 static HashSet<int>.Enumerator[] hs;
 static IEnumerator<int>[] boxes;
 static string Attempt(Func<string> f){try{return f();}catch(InvalidOperationException){return "E_MODIFIED";}catch(NullReferenceException){return "E_NIL";}}
 static void Emit(string kind,int block,string op,int slot=0,int arg=0){
  bool l=kind=="list";
  string result=Attempt(()=>{
   if(op=="new"){if(l)ls[slot]=list.GetEnumerator();else hs[slot]=set.GetEnumerator();return "ok";}
   if(op=="default"){if(l)ls[slot]=default(List<int>.Enumerator);else hs[slot]=default(HashSet<int>.Enumerator);return "ok";}
   if(op=="copy"){if(l)ls[slot]=ls[arg];else hs[slot]=hs[arg];return "ok";}
   if(op=="next")return (l ? ls[slot].MoveNext() : hs[slot].MoveNext()).ToString();
   if(op=="current")return (l ? ls[slot].Current : hs[slot].Current).ToString();
   if(op=="dispose"){if(l)ls[slot].Dispose();else hs[slot].Dispose();return "ok";}
   if(op=="equal")return (l ? ls[slot].Equals(ls[arg]) : hs[slot].Equals(hs[arg])).ToString();
   if(op=="box"){if(l)boxes[slot]=ls[arg];else boxes[slot]=hs[arg];return "ok";}
   if(op=="inew"){boxes[slot]=l ? ((IEnumerable<int>)list).GetEnumerator() : ((IEnumerable<int>)set).GetEnumerator();return "ok";}
   if(op=="alias"){boxes[slot]=boxes[arg];return "ok";}
   if(op=="inext")return boxes[slot].MoveNext().ToString();
   if(op=="icurrent")return boxes[slot].Current.ToString();
   if(op=="ireset"){boxes[slot].Reset();return "ok";}
   if(op=="idispose"){boxes[slot].Dispose();return "ok";}
   if(op=="identity")return Object.ReferenceEquals(boxes[slot],boxes[arg]).ToString();
   if(op=="type")return boxes[slot]==null ? "nil" : (boxes[slot].GetType()==(l ? typeof(List<int>.Enumerator) : typeof(HashSet<int>.Enumerator))).ToString();
   if(op=="add"){if(l){list.Add(arg);return "ok";}return set.Add(arg).ToString();}
   if(op=="remove")return (l ? list.Remove(arg) : set.Remove(arg)).ToString();
   if(op=="clear"){if(l)list.Clear();else set.Clear();return "ok";}
   if(op=="trim"){if(l)list.TrimExcess();else set.TrimExcess();return "ok";}
   if(op=="set"){if(list.Count>0)list[0]=arg;return "ok";}
   if(op=="reverse"){list.Reverse();return "ok";}
   throw new Exception("Unknown operation");
  });
  writer.WriteLine(kind+"\t"+block+"\t"+op+"\t"+slot+"\t"+arg+"\t"+result);
 }
 static void Kind(string kind){
  Random random=new Random(4197);
  for(int block=0;block<48;++block){
   list=new List<int>();set=new HashSet<int>();ls=new List<int>.Enumerator[4];hs=new HashSet<int>.Enumerator[4];boxes=new IEnumerator<int>[4];
   Emit(kind,block,"current");Emit(kind,block,"dispose");Emit(kind,block,"next");
   for(int i=1;i<=4;++i)Emit(kind,block,"add",0,i);Emit(kind,block,"remove",0,2);
   Emit(kind,block,"new",0);Emit(kind,block,"next",0);Emit(kind,block,"copy",1,0);Emit(kind,block,"equal",1,0);Emit(kind,block,"next",0);Emit(kind,block,"current",1);Emit(kind,block,"dispose",0);Emit(kind,block,"next",0);
   Emit(kind,block,"box",0,1);Emit(kind,block,"alias",1,0);Emit(kind,block,"inext",1);Emit(kind,block,"current",1);Emit(kind,block,"icurrent",0);Emit(kind,block,"identity",0,1);Emit(kind,block,"type",0);Emit(kind,block,"idispose",0);Emit(kind,block,"ireset",1);Emit(kind,block,"inext",0);
   Emit(kind,block,"inew",2);Emit(kind,block,"trim");Emit(kind,block,"inext",2);Emit(kind,block,"add",0,7);Emit(kind,block,"icurrent",0);Emit(kind,block,"inext",0);Emit(kind,block,"ireset",2);Emit(kind,block,"next",1);Emit(kind,block,"current",1);Emit(kind,block,"default",3);Emit(kind,block,"box",3,3);Emit(kind,block,"ireset",3);Emit(kind,block,"icurrent",3);
   string[] ops=kind=="list" ? new string[]{"new","default","copy","next","current","dispose","equal","box","inew","alias","inext","icurrent","ireset","idispose","identity","type","add","remove","clear","trim","set","reverse"} : new string[]{"new","default","copy","next","current","dispose","equal","box","inew","alias","inext","icurrent","ireset","idispose","identity","type","add","remove","clear","trim"};
   for(int i=0;i<40;++i){string op=ops[random.Next(ops.Length)];int slot=random.Next(4);int arg=op=="copy" || op=="box" || op=="alias" || op=="identity" || op=="equal" ? random.Next(4) : random.Next(-4,8);Emit(kind,block,op,slot,arg);}
  }
 }
 public static void Generate(string output){using(writer=new StreamWriter(output,false,new UTF8Encoding(false))){writer.WriteLine("# Independent .NET Framework CLR "+Environment.Version+" List/HashSet concrete structs and boxes; Random(4197)");Kind("list");Kind("hashset");}}
}
