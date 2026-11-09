// Actual Framework List/ICollection calls; development-only independent oracle.
using System;using System.IO;using System.Text;using System.Collections;
using System.Collections.Generic;using System.Globalization;
class ListUserBulkReference{
 static List<int> target;static int[] saved,original;static IEnumerator<int> probe;
 static int suppliedCount,failAt,callback,seedCapacity;
 static string Text(IEnumerable<int> values){return "["+string.Join(",",values)+"]";}
 static int Count(){Console.WriteLine('C');if(callback==1){target.Add(90);target.RemoveAt(target.Count-1);}if(callback==2)target.Clear();if(callback==3)target.Capacity+=7;return suppliedCount;}
 static void Copy(int[] array,int index){
  saved=array;if(callback==4){target.Add(90);target.RemoveAt(target.Count-1);}if(callback==5)target.Capacity+=7;if(callback==6)original[0]=42;
  Console.WriteLine("P "+array.Length+" "+index+" "+Text(target)+" "+target.Capacity);Console.WriteLine("A "+Text(original));
  if(callback!=1&&callback!=2&&callback!=4)Console.WriteLine(probe.MoveNext()+" "+probe.Current);
  for(int i=0;i<2;++i){if(i==failAt)throw new DivideByZeroException();array[index+i]=7+i;Console.WriteLine("W "+i+" "+Text(target));Console.WriteLine("A "+Text(original));}
 }
 class Seed:ICollection<int>{
  public int Count{get{return seedCapacity;}}public bool IsReadOnly{get{return false;}}public void Add(int n){}public void Clear(){}public bool Contains(int n){return false;}public bool Remove(int n){return false;}
  public void CopyTo(int[] a,int index){original=a;for(int i=0;i<seedCapacity;++i)a[index+i]=i+1;}public IEnumerator<int> GetEnumerator(){return new List<int>().GetEnumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 }
 static IEnumerator<int> Enumerator(){Console.WriteLine('E');return new List<int>(new[]{9,10}).GetEnumerator();}
 class ClassBag:ICollection<int>{
  public int Count{get{return ListUserBulkReference.Count();}}public bool IsReadOnly{get{return false;}}
  public void Add(int n){}public void Clear(){}public bool Contains(int n){return false;}public bool Remove(int n){return false;}
  public void CopyTo(int[] a,int index){Copy(a,index);}public IEnumerator<int> GetEnumerator(){return Enumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 }
 struct RecordBag:ICollection<int>{
  public int Count{get{return ListUserBulkReference.Count();}}public bool IsReadOnly{get{return false;}}
  public void Add(int n){}public void Clear(){}public bool Contains(int n){return false;}public bool Remove(int n){return false;}
  public void CopyTo(int[] a,int index){Copy(a,index);}public IEnumerator<int> GetEnumerator(){return Enumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 }
 static void Run(int owner,int count,int capacity,int index,int failure,int op,int action){
  suppliedCount=count;failAt=failure;callback=action;seedCapacity=capacity;saved=null;target=new List<int>(new Seed());target.RemoveRange(3,capacity-3);probe=target.GetEnumerator();probe.MoveNext();
  ICollection<int> source=owner==0?(ICollection<int>)new RecordBag():new ClassBag();
  if(op==0)target.AddRange(source);else target.InsertRange(index,source);
  Console.WriteLine(Text(target)+" "+target.Count+" "+target.Capacity);
  Console.WriteLine("A "+Text(original));
  if(saved!=null){Console.WriteLine(Text(saved));saved[0]=99;Console.WriteLine(Text(target));}
  Console.WriteLine(probe.MoveNext());
 }
 static string Hex(string text){if(text.Length==0)return "-";var result=new StringBuilder();foreach(char c in text.Replace("\r\n","\n"))result.Append(((int)c).ToString("x4"));return result.ToString();}
 static void Main(string[] args){
  CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;var console=Console.Out;
  using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){
   output.WriteLine("# owner\tcount\tcapacity\tindex\tfailAt\toperation\tcallback\tstdout_utf16_hex\terror");
   for(int owner=0;owner<2;++owner)foreach(int count in new[]{-2,0,1,2,4})foreach(int capacity in new[]{3,10})foreach(int index in new[]{-1,0,1,3,4})foreach(int failure in new[]{-1,0,1})for(int op=0;op<2;++op)for(int action=0;action<7;++action){
    var captured=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(captured);string error="ok";
    try{Run(owner,count,capacity,index,failure,op,action);}catch(DivideByZeroException){error="E_DIV_ZERO";}catch(IndexOutOfRangeException){error="E_BOUNDS";}catch(ArgumentException){error="E_RANGE";}catch(InvalidOperationException){error="E_MODIFIED";}finally{Console.SetOut(console);}
    output.WriteLine(owner+"\t"+count+"\t"+capacity+"\t"+index+"\t"+failure+"\t"+op+"\t"+action+"\t"+Hex(captured.ToString())+"\t"+error);
   }
  }
  ListUserRecordBulkReference.Generate(args[1]);
 }
}
