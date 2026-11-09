using System;using System.IO;using System.Text;using System.Collections;using System.Collections.Generic;using System.Globalization;
class ListUserRecordBulkReference{
 struct Item{public int X;public int[] Data;public void Bump(int n){X+=n;}}
 static List<Item> target;static Item[] saved,original;static IEnumerator<Item> probe;static int suppliedCount,capacity,callback;
 static Item Make(int n){return new Item{X=n,Data=new[]{n}};}
 static string Text(IEnumerable<Item> values){var data=new List<int>();foreach(var value in values)data.Add(value.X);return "["+string.Join(",",data)+"]";}
 static string Meta(){var data=new List<int>();foreach(var value in target)data.Add(value.Data==null?-1:value.Data[0]);return "["+string.Join(",",data)+"]";}
 static int Count(){Console.WriteLine('C');if(callback==1){target.Add(Make(90));target.RemoveAt(target.Count-1);}if(callback==2)target.Clear();if(callback==3)target.Capacity+=7;return suppliedCount;}
 static void Copy(Item[] a,int index){
  saved=a;if(callback==4){target.Add(Make(90));target.RemoveAt(target.Count-1);}if(callback==5)target.Capacity+=7;if(callback==6)original[0].Bump(1);
  Console.WriteLine("P "+a.Length+" "+index+" "+Text(target)+" "+target.Capacity);Console.WriteLine("A "+Text(original));
  if(callback!=1&&callback!=2&&callback!=4)Console.WriteLine(probe.MoveNext()+" "+probe.Current.X);
  for(int i=0;i<2;++i){a[index+i]=Make(7+i);Console.WriteLine("W "+i+" "+Text(target));Console.WriteLine("A "+Text(original));}
 }
 static IEnumerator<Item> Enumerator(){Console.WriteLine('E');return new List<Item>(new[]{Make(9)}).GetEnumerator();}
 class Seed:ICollection<Item>{
  public int Count{get{return capacity;}}public bool IsReadOnly{get{return false;}}public void Add(Item n){}public void Clear(){}public bool Contains(Item n){return false;}public bool Remove(Item n){return false;}
  public void CopyTo(Item[] a,int index){original=a;for(int i=0;i<capacity;++i)a[index+i]=Make(i+1);}public IEnumerator<Item> GetEnumerator(){return new List<Item>().GetEnumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 }
 class ClassBag:ICollection<Item>{
  public int Count{get{return ListUserRecordBulkReference.Count();}}public bool IsReadOnly{get{return false;}}public void Add(Item n){}public void Clear(){}public bool Contains(Item n){return false;}public bool Remove(Item n){return false;}
  public void CopyTo(Item[] a,int index){Copy(a,index);}public IEnumerator<Item> GetEnumerator(){return Enumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 }
 struct RecordBag:ICollection<Item>{
  public int Count{get{return ListUserRecordBulkReference.Count();}}public bool IsReadOnly{get{return false;}}public void Add(Item n){}public void Clear(){}public bool Contains(Item n){return false;}public bool Remove(Item n){return false;}
  public void CopyTo(Item[] a,int index){Copy(a,index);}public IEnumerator<Item> GetEnumerator(){return Enumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 }
 static void Run(int owner,int count,int initialCapacity,int index,int op,int action){
  suppliedCount=count;capacity=initialCapacity;callback=action;saved=null;target=new List<Item>(new Seed());target.RemoveRange(3,capacity-3);probe=target.GetEnumerator();probe.MoveNext();
  ICollection<Item> source=owner==0?(ICollection<Item>)new RecordBag():new ClassBag();
  if(op==0)target.AddRange(source);else target.InsertRange(index,source);
  Console.WriteLine(Text(target)+" "+target.Count+" "+target.Capacity);Console.WriteLine("A "+Text(original));
  Console.WriteLine(Text(saved));saved[0].Bump(90);Console.WriteLine(Text(target));saved[0].Data[0]=77;Console.WriteLine(Meta());Console.WriteLine(probe.MoveNext());
 }
 static string Hex(string text){var result=new StringBuilder();foreach(char c in text.Replace("\r\n","\n"))result.Append(((int)c).ToString("x4"));return result.Length==0?"-":result.ToString();}
 public static void Generate(string path){var console=Console.Out;using(var output=new StreamWriter(path,false,new UTF8Encoding(false))){
  output.WriteLine("# owner\tcount\tcapacity\tindex\toperation\tcallback\tstdout_utf16_hex\terror");
  for(int owner=0;owner<2;++owner)foreach(int count in new[]{2,4})foreach(int cap in new[]{3,10})foreach(int index in new[]{0,1,3})for(int op=0;op<2;++op)for(int action=0;action<7;++action){
   var captured=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(captured);string error="ok";
   try{Run(owner,count,cap,index,op,action);}catch(IndexOutOfRangeException){error="E_BOUNDS";}catch(ArgumentException){error="E_RANGE";}catch(InvalidOperationException){error="E_MODIFIED";}finally{Console.SetOut(console);}
   output.WriteLine(owner+"\t"+count+"\t"+cap+"\t"+index+"\t"+op+"\t"+action+"\t"+Hex(captured.ToString())+"\t"+error);
  }
 }}
}
