// Development-only independent framework recipes. Never installed.
using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
class TraceCollection:ICollection<int> {
 readonly List<int> data;
 public TraceCollection(IEnumerable<int> items){data=new List<int>(items);}
 public int Count{get{Console.Write("C");return data.Count;}}
 public bool IsReadOnly{get{return false;}}
 public void Add(int item){Console.Write("A");data.Add(item);}
 public void Clear(){Console.Write("L");data.Clear();}
 public bool Contains(int item){Console.Write("H");return item==99||data.Contains(item);}
 public void CopyTo(int[] target,int index){Console.Write("P");data.CopyTo(target,index);}
 public bool Remove(int item){Console.Write("R");return data.Remove(item);}
 public IEnumerator<int> GetEnumerator(){Console.Write("G");return data.GetEnumerator();}
 IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
}
class Reference {
 static string Hex(string value){if(value.Length==0)return "-";var b=new StringBuilder();foreach(char c in value)b.Append(((int)c).ToString("X4"));return b.ToString();}
 static string Error(Exception ex,int kind,int op,int index,int length){
  if(ex is NotSupportedException)return "E_READONLY";
  if(ex is NullReferenceException)return "E_NIL";
  if(ex is ArgumentNullException)return "E_NIL";
  if(ex is ArgumentOutOfRangeException)return "E_RANGE";
  if(ex is ArgumentException)return "E_RANGE";
  throw ex;
 }
 static void ArrayText(int[] array){Console.Write("[");Console.Write(string.Join(",",array));Console.WriteLine("]");}
 static void Main(string[] args){
  CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;
  var original=Console.Out;int rows=0;
  using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){
   file.WriteLine("# kind op size nil index targetLength output_utf16 error; .NET Framework "+Environment.Version);
   for(int kind=0;kind<6;kind++)for(int op=0;op<13;op++)foreach(int size in new[]{0,2})foreach(int nil in new[]{0,1})foreach(int index in op==5?new[]{-1,0,1,3,4}:new[]{0})foreach(int length in op==5?new[]{-1,0,1,3}:new[]{3}){
    int[] items=size==0?new int[0]:new[]{1,2};ICollection<int> c;
    var dictionary=new Dictionary<int,int>();foreach(int item in items)dictionary.Add(item,item);
    if(kind==0)c=items;else if(kind==1)c=new List<int>(items);else if(kind==2)c=new HashSet<int>(items);else if(kind==3)c=dictionary.Keys;else if(kind==4)c=dictionary.Values;else c=new TraceCollection(items);
    if(nil!=0)c=null;var target=length<0?null:new int[length];string error="ok";var output=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(output);
    try{
     IEnumerable<int> e=c;
     switch(op){
      case 0:Console.WriteLine(c.Count);break;
      case 1:Console.WriteLine(c.IsReadOnly);break;
      case 2:c.Add(9);Console.WriteLine(c.Count);break;
      case 3:c.Clear();Console.WriteLine(c.Count);break;
      case 4:Console.WriteLine(c.Contains(2));break;
      case 5:c.CopyTo(target,index);ArrayText(target);break;
      case 6:Console.WriteLine(c.Remove(2)+" "+c.Count);break;
      case 7:using(var iterator=c.GetEnumerator()){while(iterator.MoveNext())Console.Write(iterator.Current+",");}Console.WriteLine();break;
      case 8:Console.WriteLine(e.Count());break;
      case 9:Console.WriteLine(e.Contains(99));break;
      case 10:Console.WriteLine(e.Contains(99,null));break;
      case 11:ArrayText(e.ToArray());break;
      case 12:Console.WriteLine(e.Any());break;
     }
    }catch(Exception ex){error=Error(ex,kind,op,index,length);if(c==null&&ex is ArgumentNullException)error="E_NIL";}
    Console.SetOut(original);file.WriteLine(kind+"\t"+op+"\t"+size+"\t"+nil+"\t"+index+"\t"+length+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
   }
  }
  Console.WriteLine(rows+" independent ICollection outcomes");
 }
}
