using System;using System.IO;using System.Text;using System.Linq;using System.Collections;using System.Collections.Generic;
struct Item:IComparable<Item>{public int X;public Item(int n){X=n;}public void Bump(int n){X+=n;}public int CompareTo(Item other){return X.CompareTo(other.X);}}
class CaptureState<T>{
 public T[] Saved;public int Count,ThrowIndex;public Func<int,T> Make;
 public int ReadCount(){Console.Write("C");return Count;}
 public void Copy(T[] a,int index){Console.Write("P");Saved=a;for(int n=0;n<2;n++){if(n==ThrowIndex)throw new DivideByZeroException();a[index+n]=Make(10+n);Console.Write("W");}}
 public IEnumerator<T> Enumerate(){Console.Write("E");return new List<T>().GetEnumerator();}
}
struct RecordCapture<T>:ICollection<T>{
 CaptureState<T> state;public RecordCapture(CaptureState<T> s){state=s;}public int Count{get{return state.ReadCount();}}public bool IsReadOnly{get{return true;}}
 public void CopyTo(T[] a,int index){state.Copy(a,index);}public IEnumerator<T> GetEnumerator(){return state.Enumerate();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 public void Add(T n){}public void Clear(){}public bool Contains(T n){return false;}public bool Remove(T n){return false;}
}
class ClassCapture<T>:ICollection<T>{
 CaptureState<T> state;public ClassCapture(CaptureState<T> s){state=s;}public int Count{get{return state.ReadCount();}}public bool IsReadOnly{get{return true;}}
 public void CopyTo(T[] a,int index){state.Copy(a,index);}public IEnumerator<T> GetEnumerator(){return state.Enumerate();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 public void Add(T n){}public void Clear(){}public bool Contains(T n){return false;}public bool Remove(T n){return false;}
}
static class RecordListAliasReference{
 static void Print<T>(IEnumerable<T> values,Func<T,int> number){Console.WriteLine("["+string.Join(",",values.Select(number))+"]");}
 static void Modify<T>(ref T slot,Func<T,int> number,Func<int,T> make,Func<int> rhs){int old=number(slot);int add=rhs();slot=make(old+add);}
 static void Run<T>(int owner,int count,int fail,int op,int ctor,Func<int,T> make,Func<T,int> number){
  var state=new CaptureState<T>{Count=count,ThrowIndex=fail,Make=make};IEnumerable<T> source=owner==0?(ICollection<T>)new RecordCapture<T>(state):new ClassCapture<T>(state);
  var list=ctor==0?source.ToList():new List<T>(source);Console.WriteLine(" "+list.Count+" "+list.Capacity);if(state.Saved==null){Console.WriteLine("empty");return;}
  var it=list.GetEnumerator();it.MoveNext();
  switch(op){
   case 1:state.Saved[0]=make(80);break;case 2:list[0]=make(81);break;
   case 3:list.Add(make(82));state.Saved[0]=make(83);break;
   case 4:list.Clear();state.Saved[0]=make(84);list.Add(make(85));break;
   case 5:list.Capacity=count+3;state.Saved[0]=make(86);break;
   case 6:Array.Reverse(state.Saved);break;case 7:Array.Sort(state.Saved);break;
   case 8:Modify(ref state.Saved[0],number,make,()=>5);break;
   case 9:if(typeof(T)==typeof(Item))((Item[])(object)state.Saved)[0].Bump(5);else Modify(ref state.Saved[0],number,make,()=>5);break;
   case 10:list.Reverse();break;case 11:list.Sort();break;
   case 12:Modify(ref state.Saved[0],number,make,()=>{list[0]=make(99);return 1;});break;
  }
  Print(list,number);Print(state.Saved,number);Console.WriteLine(it.MoveNext());
 }
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("X4"));return b.ToString();}
 public static void Generate(string path){var original=Console.Out;int rows=0;using(var file=new StreamWriter(path,false,new UTF8Encoding(false))){file.WriteLine("# owner value_kind count fail operation ctor output_utf16 error; Framework "+Environment.Version);
  for(int owner=0;owner<2;owner++)for(int value=0;value<2;value++)foreach(int count in new[]{-1,0,1,2,3})foreach(int fail in new[]{-1,0,1})for(int op=0;op<13;op++)for(int ctor=0;ctor<2;ctor++){
   var output=new StringWriter();Console.SetOut(output);string error="ok";try{if(value==0)Run<int>(owner,count,fail,op,ctor,n=>n,n=>n);else Run<Item>(owner,count,fail,op,ctor,n=>new Item(n),n=>n.X);}catch(OverflowException){error="E_OVERFLOW";}catch(DivideByZeroException){error="E_DIV_ZERO";}catch(IndexOutOfRangeException){error="E_BOUNDS";}catch(InvalidOperationException){error="E_MODIFIED";}finally{Console.SetOut(original);}
   file.WriteLine(owner+"\t"+value+"\t"+count+"\t"+fail+"\t"+op+"\t"+ctor+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
  }
 }Console.WriteLine(rows+" independent retained array/list outcomes");}
}
