// Independent Framework ISet calls; development-only, never installed.
using System;using System.IO;using System.Text;using System.Linq;using System.Collections;using System.Collections.Generic;
class Remainder:IEqualityComparer<int>{public bool Equals(int a,int b){return a%3==b%3;}public int GetHashCode(int n){return n%3;}}
class TraceSet:ISet<int>{
 readonly HashSet<int> data;
 public TraceSet(int size,IEqualityComparer<int> comparer){data=new HashSet<int>(comparer);if(size!=0){data.Add(1);data.Add(2);}}
 public int Count{get{Console.Write("C");return data.Count;}}public bool IsReadOnly{get{Console.Write("O");return false;}}
 public bool Add(int n){Console.Write("A");return data.Add(n);}void ICollection<int>.Add(int n){Console.Write("P");data.Add(n);}
 public void Clear(){Console.Write("L");data.Clear();}public bool Contains(int n){Console.Write("H");return data.Contains(n);}public void CopyTo(int[] a,int i){Console.Write("Y");data.CopyTo(a,i);}public bool Remove(int n){Console.Write("R");return data.Remove(n);}
 public IEnumerator<int> GetEnumerator(){Console.Write("E");return data.GetEnumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 public void ExceptWith(IEnumerable<int> x){Console.Write("X");data.ExceptWith(x);}public void IntersectWith(IEnumerable<int> x){Console.Write("I");data.IntersectWith(x);}
 public bool IsProperSubsetOf(IEnumerable<int> x){Console.Write("j");return data.IsProperSubsetOf(x);}public bool IsProperSupersetOf(IEnumerable<int> x){Console.Write("J");return data.IsProperSupersetOf(x);}
 public bool IsSubsetOf(IEnumerable<int> x){Console.Write("s");return data.IsSubsetOf(x);}public bool IsSupersetOf(IEnumerable<int> x){Console.Write("S");return data.IsSupersetOf(x);}
 public bool Overlaps(IEnumerable<int> x){Console.Write("V");return data.Overlaps(x);}public bool SetEquals(IEnumerable<int> x){Console.Write("Q");return data.SetEquals(x);}
 public void SymmetricExceptWith(IEnumerable<int> x){Console.Write("D");data.SymmetricExceptWith(x);}public void UnionWith(IEnumerable<int> x){Console.Write("U");data.UnionWith(x);}
}
class Reference{
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("X4"));return b.ToString();}
 static void Items(ISet<int> set){foreach(int n in set)Console.Write(n+",");Console.WriteLine();}
 static void Main(string[] args){var original=Console.Out;int rows=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# kind comparer size nil operation item input offset output_utf16 error; Framework "+Environment.Version);
  for(int kind=0;kind<2;kind++)for(int mode=0;mode<2;mode++)foreach(int size in new[]{0,2})for(int nil=0;nil<2;nil++)for(int op=0;op<23;op++)foreach(int item in new[]{-1,1,4})foreach(int input in op>=8&&op<=17?new[]{0,1,2,3,4}:new[]{0})foreach(int offset in op==5?new[]{-1,0,2,4}:new[]{0}){
   IEqualityComparer<int> comparer=mode==0?null:new Remainder();var native=new HashSet<int>(comparer);if(size!=0){native.Add(1);native.Add(2);}ISet<int> set=kind==0?(ISet<int>)native:new TraceSet(size,comparer);if(nil!=0)set=null;
   IEnumerable<int> source=input==0?new int[0]:input==1?new[]{2,3}:input==2?new[]{1,4,4}:input==4?(IEnumerable<int>)set:null;
   var output=new StringWriter();Console.SetOut(output);string error="ok";try{switch(op){
    case 0:Console.WriteLine(set.Count);break;case 1:Console.WriteLine(set.IsReadOnly);break;case 2:Console.WriteLine(set.Add(item));Items(set);break;case 3:set.Clear();Items(set);break;case 4:Console.WriteLine(set.Contains(item));break;
    case 5:var a=new int[4];set.CopyTo(a,offset);Console.WriteLine("["+string.Join(",",a)+"]");break;case 6:Console.WriteLine(set.Remove(item));Items(set);break;case 7:Items(set);break;
    case 8:set.ExceptWith(source);Items(set);break;case 9:set.IntersectWith(source);Items(set);break;case 10:Console.WriteLine(set.IsProperSubsetOf(source));break;case 11:Console.WriteLine(set.IsProperSupersetOf(source));break;
    case 12:Console.WriteLine(set.IsSubsetOf(source));break;case 13:Console.WriteLine(set.IsSupersetOf(source));break;case 14:Console.WriteLine(set.Overlaps(source));break;case 15:Console.WriteLine(set.SetEquals(source));break;
    case 16:set.SymmetricExceptWith(source);Items(set);break;case 17:set.UnionWith(source);Items(set);break;
    case 18:((ICollection<int>)set).Add(item);Items(set);break;case 19:Console.WriteLine(((ICollection<int>)set).Remove(item));Items(set);break;
    case 22:Items(new HashSet<int>(set,comparer));break;case 20:Console.WriteLine(set.AsEnumerable().Count());break;case 21:Console.WriteLine("["+string.Join(",",set.AsEnumerable().ToArray())+"]");break;
   }}catch(NullReferenceException){error="E_NIL";}catch(ArgumentNullException){error="E_NIL";}catch(ArgumentException){error="E_RANGE";}catch(InvalidOperationException){error="E_MODIFIED";}finally{Console.SetOut(original);}
   file.WriteLine(kind+"\t"+mode+"\t"+size+"\t"+nil+"\t"+op+"\t"+item+"\t"+input+"\t"+offset+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
  }
 }Console.WriteLine(rows+" independent ISet outcomes");}
}
