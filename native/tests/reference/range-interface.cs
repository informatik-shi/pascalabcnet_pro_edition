// Independent Framework boxing and source IntRange/CharRange recipes, development-only.
using System;using System.IO;using System.Text;using System.Linq;using System.Collections;using System.Collections.Generic;
struct RangeValue<T>:ICollection<T>,IEquatable<RangeValue<T>>{
 readonly T low,high;
 public RangeValue(T l,T h){low=l;high=h;}
 public static int Number(T n){return Convert.ToInt32(n);}
 public static T At(int n){return typeof(T)==typeof(char)?(T)(object)unchecked((char)n):(T)(object)n;}
 public int Low{get{return Number(low);}}public int High{get{return Number(high);}}
 public int Count{get{return Math.Max(0,unchecked(High-Low+1));}}bool ICollection<T>.IsReadOnly{get{return true;}}
 void ICollection<T>.Add(T n){throw new InvalidOperationException();}bool ICollection<T>.Remove(T n){throw new InvalidOperationException();}void ICollection<T>.Clear(){throw new InvalidOperationException();}
 public bool Contains(T n){int value=Number(n);return value>=Low&&value<=High;}
 public void CopyTo(T[] a,int offset){int last=unchecked(Math.Min(Count,unchecked(a.Length-offset))-1);for(long i=0;i<=last;i++)a[unchecked((int)i+offset)]=At(unchecked(Low+(int)i));}
 public IEnumerator<T> GetEnumerator(){return Enumerable.Range(Low,High<Low?0:unchecked(High-Low+1)).Select(At).GetEnumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 public bool Equals(RangeValue<T> other){return Low==other.Low&&High==other.High;}
 public override bool Equals(object other){return other is RangeValue<T>&&Equals((RangeValue<T>)other);}
 public override int GetHashCode(){uint l=(uint)Low,h=(uint)High;if(typeof(T)==typeof(char)){l|=l<<16;h|=h<<16;}return unchecked((int)(l*668265263U^h));}
}
class Reference{
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("X4"));return b.ToString();}
 static void ArrayText<T>(IEnumerable<T> a){Console.WriteLine("["+string.Join(",",a.Select(RangeValue<T>.Number))+"]");}
 static void Run<T>(int low,int high,int op,int item,int offset,int length,int nil,int nilArray){
  var r=new RangeValue<T>(RangeValue<T>.At(low),RangeValue<T>.At(high));var original=r;
  ICollection<T> a=r,b=a,fresh=r;if(nil!=0)a=null;T value=RangeValue<T>.At(item);
  switch(op){
   case 0:Console.WriteLine(a.Count);break;case 1:Console.WriteLine(a.IsReadOnly);break;case 2:Console.WriteLine(a.Contains(value));break;
   case 3:T[] data=nilArray!=0?null:new T[length];a.CopyTo(data,offset);ArrayText(data);break;
   case 4:foreach(T n in a)Console.Write(RangeValue<T>.Number(n)+",");Console.WriteLine();break;
   case 5:a.Add(value);break;case 6:Console.WriteLine(a.Remove(value));break;case 7:a.Clear();break;
   case 8:Console.WriteLine(object.ReferenceEquals(a,b)+" "+object.ReferenceEquals(a,fresh));r=new RangeValue<T>(RangeValue<T>.At(low+1),RangeValue<T>.At(high+1));Console.WriteLine(((RangeValue<T>)(object)a).Equals(original));break;
   case 9:var copy=(RangeValue<T>)(object)a;Console.WriteLine(copy.Low+" "+copy.High+" "+copy.Equals(r));break;
   case 10:IEnumerable<T> e=a;Console.WriteLine(object.ReferenceEquals(a,e)+" "+object.ReferenceEquals(e,(IEnumerable<T>)a)+" "+object.ReferenceEquals(e,(IEnumerable<T>)r));break;
   case 11:Console.WriteLine(((IEquatable<RangeValue<T>>)a).Equals(r));break;
   case 12:var comparer=EqualityComparer<ICollection<T>>.Default;Console.WriteLine(comparer.Equals(a,fresh)+" "+(comparer.GetHashCode(a)==comparer.GetHashCode(fresh)));break;
   case 13:IEnumerable<T> source=a;Console.WriteLine(source.Count());ArrayText(source.ToArray());break;
   case 14:ICollection<T> array=new[]{RangeValue<T>.At(low),RangeValue<T>.At(high)};Console.WriteLine(new List<ICollection<T>>{a}.Contains(array)+" "+new HashSet<ICollection<T>>{a,fresh}.Count);break;
  }
 }
 static void Main(string[] args){var original=Console.Out;int rows=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# kind low high op item offset length nil nil_array output_utf16 error; Framework "+Environment.Version);
  for(int kind=0;kind<2;kind++)foreach(var bounds in kind==0?new[]{new[]{0,-1},new[]{0,0},new[]{1,3},new[]{-1,1}}:new[]{new[]{0,0},new[]{100,97},new[]{97,99},new[]{65534,65535}})
  for(int op=0;op<15;op++)foreach(int item in new[]{bounds[0]-1,bounds[0],bounds[1]+1})for(int nil=0;nil<2;nil++)foreach(int offset in op==3?new[]{-1,0,1,3,5,int.MinValue,int.MaxValue}:new[]{0})foreach(int length in op==3?new[]{0,2,4}:new[]{0})foreach(int nilArray in op==3?new[]{0,1}:new[]{0}){
   var output=new StringWriter();Console.SetOut(output);string error="ok";try{if(kind==0)Run<int>(bounds[0],bounds[1],op,item,offset,length,nil,nilArray);else Run<char>(bounds[0],bounds[1],op,item,offset,length,nil,nilArray);}
   catch(NullReferenceException){error="E_NIL";}catch(ArgumentNullException){error="E_NIL";}catch(IndexOutOfRangeException){error="E_BOUNDS";}catch(ArgumentOutOfRangeException){error="E_RANGE";}catch(InvalidOperationException){error="E_READONLY";}finally{Console.SetOut(original);}
   file.WriteLine(kind+"\t"+bounds[0]+"\t"+bounds[1]+"\t"+op+"\t"+item+"\t"+offset+"\t"+length+"\t"+nil+"\t"+nilArray+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
  }
 }Console.WriteLine(rows+" independent range-interface outcomes");}
}
