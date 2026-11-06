using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Collections.Generic;
public static class GenericEnumeratorReference {
 static StreamWriter writer;
 static string Attempt(Func<string> action){try{return action();}catch(InvalidOperationException){return "E_ITERATOR_STATE";}catch(NullReferenceException){return "E_NIL";}catch(NotSupportedException){return "E_NOT_SUPPORTED";}catch(NotImplementedException){return "E_NOT_SUPPORTED";}}
 static void Probe<T>(string name,Func<IEnumerator<T>> factory){
  for(int round=0;round<4;++round){
   IEnumerator<T>[] slots=new IEnumerator<T>[2];slots[0]=factory();slots[1]=slots[0];Random random=new Random(371+round);
   string[] initial={"current","next","current","alias","next","current","dispose","current","next","current","reset","current","next","current","next","next","next","current","next","current","reset","next","current"};
   for(int i=0;i<initial.Length+28;++i){
    string op=i<initial.Length ? initial[i] : new string[]{"next","current","reset","dispose","alias"}[random.Next(5)];int slot=i<initial.Length ? i%2 : random.Next(2);
    string result=Attempt(()=>{if(op=="alias"){slots[1]=slots[0];return Object.ReferenceEquals(slots[0],slots[1]).ToString();}if(op=="current"){T value=slots[slot].Current;return (object)value==null ? "~" : (value is char ? ((int)(char)(object)value).ToString() : value.ToString());}if(op=="next")return slots[slot].MoveNext().ToString();if(op=="reset")slots[slot].Reset();else slots[slot].Dispose();return "ok";});
    writer.WriteLine(name+"\t"+round+"\t"+slot+"\t"+op+"\t"+result);
   }
   slots[0].Dispose();
  }
 }
 static IEnumerable<int> Iterate(){int value=1;while(true)yield return value++;}
 public static void Generate(string output){
  using(writer=new StreamWriter(output,false,new UTF8Encoding(false))){
   writer.WriteLine("# .NET Framework CLR "+Environment.Version+"; generic interfaces, legacy source-specific Current/Dispose/Reset; seed 371+round");
   Probe("array",()=>((IEnumerable<int>)new int[]{1,2,3}).GetEnumerator());
   Probe("empty_array",()=>((IEnumerable<int>)new int[0]).GetEnumerator());
   Probe("list",()=>new List<int>{1,2,3}.GetEnumerator());
   Probe("hashset",()=>new HashSet<int>{1,2,3}.GetEnumerator());
   Probe("keys",()=>new Dictionary<int,int>{{11,1},{22,2},{33,3}}.Keys.GetEnumerator());
   Probe("values",()=>new Dictionary<int,int>{{11,1},{22,2},{33,3}}.Values.GetEnumerator());
   Probe("range",()=>Enumerable.Range(1,3).GetEnumerator());
   Probe("empty_range",()=>Enumerable.Empty<int>().GetEnumerator());
   Probe("empty_repeat",()=>Enumerable.Repeat(7,0).GetEnumerator());
   Probe("range_char",()=>Enumerable.Range(97,3).Select(x=>(char)x).GetEnumerator());
   Probe("empty_range_char",()=>Enumerable.Empty<int>().Select(x=>(char)x).GetEnumerator());
   Probe("repeat",()=>Enumerable.Repeat(7,3).GetEnumerator());
   Probe("where",()=>new int[]{1,2,3}.Where(x=>x!=2).GetEnumerator());
   Probe("where_indexed",()=>new int[]{1,2,3}.Where((x,i)=>i!=1).GetEnumerator());
   Probe("select",()=>new int[]{1,2,3}.Select(x=>x*2).GetEnumerator());
   Probe("select_indexed",()=>new int[]{1,2,3}.Select((x,i)=>x+i).GetEnumerator());
   Probe("iterate",()=>Iterate().GetEnumerator());
   Probe("seqgen",()=>Iterate().Take(3).GetEnumerator());
   Probe("seqgen_index",()=>Enumerable.Range(0,3).Select(i=>i+1).GetEnumerator());
   Probe("seqwhile",()=>Iterate().TakeWhile(x=>x<4).GetEnumerator());
   Probe("string",()=>((IEnumerable<char>)"abc").GetEnumerator());
   Probe("empty_string",()=>((IEnumerable<char>)"").GetEnumerator());
  }
 }
}
