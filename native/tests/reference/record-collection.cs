using System;using System.IO;using System.Text;using System.Linq;using System.Collections;using System.Collections.Generic;
struct Cursor:IEnumerator<int>{
 int[] data;int index;public Cursor(int[] values){data=values;index=-1;}
 public bool MoveNext(){index++;return index<data.Length;}public int Current{get{return data[index];}}object IEnumerator.Current{get{return Current;}}
 public void Reset(){this=new Cursor(data);}public void Dispose(){Console.Write("D");}
}
struct Bag:ICollection<int>{
 List<int> data;int countReads,copyCalls;public Bag(int seed){data=new List<int>{seed,seed+1};countReads=0;copyCalls=0;}
 public int Reads{get{return countReads;}}public int Copies{get{return copyCalls;}}public int[] Values(){return data.ToArray();}
 public int Count{get{countReads++;return data.Count;}}public bool IsReadOnly{get{return false;}}
 public void Add(int n){data.Add(n);}public void Clear(){data.Clear();}public bool Contains(int n){return data.Contains(n);}public bool Remove(int n){return data.Remove(n);}
 public void CopyTo(int[] target,int index){copyCalls++;data.CopyTo(target,index);}public IEnumerator<int> GetEnumerator(){return new Cursor(data.ToArray());}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
}
static class RecordCollectionReference{
 static void ArrayText(IEnumerable<int> a){Console.WriteLine("["+string.Join(",",a)+"]");}
 static void Stats(ICollection<int> c,Bag original){var copy=(Bag)(object)c;Console.WriteLine(copy.Reads+" "+copy.Copies+" "+original.Reads);}
 static void Run(int seed,int item,int nil,int op){
  var b=new Bag(seed);ICollection<int> c=b;if(nil!=0)c=null;IEnumerable<int> e=c;
  switch(op){
   case 0:foreach(int n in e)Console.Write(n+";");Console.WriteLine();Stats(c,b);break;
   case 1:Console.WriteLine(e.Count());Stats(c,b);break;
   case 2:ArrayText(e.ToArray());Stats(c,b);break;
   case 3:ArrayText(e.ToList());Stats(c,b);break;
   case 4:var target=new int[4];c.CopyTo(target,1);ArrayText(target);Stats(c,b);break;
   case 5:c.Add(item);ArrayText(b.Values());Stats(c,b);break;
   case 6:var it=e.GetEnumerator();var alias=it;it.MoveNext();Console.WriteLine(alias.Current);var copy=(Cursor)(object)it;copy.MoveNext();Console.WriteLine(copy.Current+" "+alias.Current);it.Reset();it.MoveNext();Console.WriteLine(it.Current);it.Dispose();Console.WriteLine();Stats(c,b);break;
   case 7:foreach(int n in e){Console.Write(n+";");break;}Console.WriteLine();Stats(c,b);break;
   case 8:var invalid=e.GetEnumerator();Console.WriteLine(invalid.Current);break;
   case 9:c.Clear();ArrayText(b.Values());Stats(c,b);break;
   case 10:IEnumerable<int> direct=b;var comparer=EqualityComparer<IEnumerable<int>>.Default;var equal=comparer.Equals(e,direct);Console.WriteLine(object.ReferenceEquals(c,e)+" "+object.ReferenceEquals(e,direct)+" "+equal+" "+(equal?(comparer.GetHashCode(e)==comparer.GetHashCode(direct)).ToString():"-"));break;
   case 11:ArrayText(new HashSet<int>(e));Stats(c,b);break;
   case 12:Console.WriteLine(c.Remove(item));ArrayText(b.Values());Stats(c,b);break;
   case 13:Console.WriteLine(c.Contains(item));Stats(c,b);break;
  }
 }
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("X4"));return b.ToString();}
 public static void Generate(string path){var original=Console.Out;int rows=0;using(var file=new StreamWriter(path,false,new UTF8Encoding(false))){file.WriteLine("# seed item nil operation output_utf16 error; Framework "+Environment.Version);
  foreach(int seed in new[]{-1,0,2})foreach(int item in new[]{seed,seed+1,9})for(int nil=0;nil<2;nil++)for(int op=0;op<14;op++){
   var output=new StringWriter();Console.SetOut(output);string error="ok";try{Run(seed,item,nil,op);}catch(NullReferenceException){error="E_NIL";}catch(ArgumentNullException){error="E_NIL";}catch(IndexOutOfRangeException){error="E_BOUNDS";}finally{Console.SetOut(original);}
   file.WriteLine(seed+"\t"+item+"\t"+nil+"\t"+op+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
  }
 }Console.WriteLine(rows+" independent record collection outcomes");}
}
