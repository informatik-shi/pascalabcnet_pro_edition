// Independent development-only Framework recipes; never installed.
using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
class TraceList:IList<int>{
 readonly List<int> data;
 public TraceList(IEnumerable<int> items){data=new List<int>(items);}
 public int Count{get{Console.Write("C");return data.Count;}}
 public bool IsReadOnly{get{return false;}}
 public int this[int i]{get{Console.Write("G"+i+";");return data[i];}set{Console.Write("S"+i+";");data[i]=value;}}
 public void Add(int x){Console.Write("A");data.Add(x);}public void Clear(){Console.Write("L");data.Clear();}
 public bool Contains(int x){Console.Write("H");return data.Contains(x);}public void CopyTo(int[] a,int i){Console.Write("P");data.CopyTo(a,i);}
 public bool Remove(int x){Console.Write("R");return data.Remove(x);}public int IndexOf(int x){Console.Write("X");return data.IndexOf(x);}
 public void Insert(int i,int x){Console.Write("I"+i+";");data.Insert(i,x);}public void RemoveAt(int i){Console.Write("D"+i+";");data.RemoveAt(i);}
 public IEnumerator<int> GetEnumerator(){Console.Write("E");return data.GetEnumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
}
class Reference {
 static string Hex(string value){if(value.Length==0)return "-";var b=new StringBuilder();foreach(char c in value)b.Append(((int)c).ToString("X4"));return b.ToString();}
 static string Error(Exception e){if(e is NullReferenceException||e is ArgumentNullException)return "E_NIL";if(e is NotSupportedException)return "E_READONLY";if(e is ArgumentException)return "E_RANGE";if(e is IndexOutOfRangeException)return "E_BOUNDS";throw e;}
 static void Items(IList<int> list){foreach(int x in list)Console.Write(x+",");Console.WriteLine();}
 static int Index(int index){Console.Write("I");return index;}static int Value(){Console.Write("V");return 9;}static IList<int> Receiver(IList<int> c){Console.Write("O");return c;}
 // Pascal's Assign.cs builds separate receiver/index nodes for the setter and getter.
 static void Set(IList<int> c,int index,int value){c[index]=value;}
 static void Main(string[] args){
  CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;var original=Console.Out;int rows=0;
  using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){
   file.WriteLine("# kind operation size nil index output_utf16 error; Framework "+Environment.Version);
   for(int kind=0;kind<3;kind++)for(int op=0;op<14;op++)foreach(int size in new[]{0,2})foreach(int nil in new[]{0,1})foreach(int index in op==5||op==8||op==9||op==11||op==12||op==13?new[]{-1,0,1,2,3}:new[]{0}){
    var items=size==0?new int[0]:new[]{1,2};IList<int> c=kind==0?(IList<int>)items:kind==1?new List<int>(items):(IList<int>)new TraceList(items);if(nil!=0)c=null;
    var output=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(output);string error="ok";
    try{switch(op){
     case 0:Console.WriteLine(c.Count);break;case 1:Console.WriteLine(c.IsReadOnly);break;
     case 2:c.Add(9);Items(c);break;case 3:c.Clear();Items(c);break;case 4:Console.WriteLine(c.Contains(2));break;
     case 5:var a=new int[3];c.CopyTo(a,index);Console.WriteLine("["+string.Join(",",a)+"]");break;
     case 6:Console.WriteLine(c.Remove(2));Items(c);break;case 7:Items(c);break;case 8:Console.WriteLine(c[index]);break;
     case 9:c[index]=9;Items(c);break;case 10:Console.WriteLine(c.IndexOf(2));break;
     case 11:c.Insert(index,9);Items(c);break;case 12:c.RemoveAt(index);Items(c);break;
     case 13:Set(Receiver(c),Index(index),Receiver(c)[Index(index)]+Value());Items(c);break;
    }}catch(Exception e){error=Error(e);}Console.SetOut(original);
    file.WriteLine(kind+"\t"+op+"\t"+size+"\t"+nil+"\t"+index+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
   }
  }Console.WriteLine(rows+" independent IList outcomes");if(args.Length>1)School(args[1]);
 }
 static void School(string filename){
  int rows=0;var original=Console.Out;
  using(var file=new StreamWriter(filename,false,new UTF8Encoding(false))){
   file.WriteLine("# sourceKind targetKind sourceData targetData sourceNil targetNil textNil output_utf16 error; School source algorithm");
   var sources=new[]{new[]{"a","b"},new[]{"a",""},new[]{(string)null,"b"},new string[0]};
   var targets=new[]{new[]{"b","x"},new[]{"z"},new string[0],new[]{(string)null,"x"}};
   for(int sk=0;sk<3;sk++)for(int tk=0;tk<3;tk++)for(int sd=0;sd<4;sd++)for(int td=0;td<4;td++)for(int sn=0;sn<2;sn++)for(int tn=0;tn<2;tn++)for(int textNil=0;textNil<2;textNil++){
    IList<string> source=sn!=0?null:sk==0?(IList<string>)sources[sd]:sk==1?new List<string>(sources[sd]):(IList<string>)new TraceStrings("S",sources[sd]);
    IList<string> target=tn!=0?null:tk==0?(IList<string>)targets[td]:tk==1?new List<string>(targets[td]):(IList<string>)new TraceStrings("T",targets[td]);
    var output=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(output);string error="ok";
    try{var sb=new StringBuilder(textNil!=0?null:"aabbaba");int lastTarget=unchecked(target.Count-1);int lastSource=unchecked(source.Count-1);for(int i=0;i<=lastSource;i++)if(i<=lastTarget)sb.Replace(source[i],target[i]);else sb.Replace(source[i],"");Console.WriteLine(sb.ToString());}
    catch(Exception e){error=Error(e);}Console.SetOut(original);file.WriteLine(sk+"\t"+tk+"\t"+sd+"\t"+td+"\t"+sn+"\t"+tn+"\t"+textNil+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
   }
  }Console.WriteLine(rows+" independent School IList outcomes");
 }
}
class TraceStrings:IList<string>{
 readonly string label;readonly List<string> data;public TraceStrings(string name,IEnumerable<string> items){label=name;data=new List<string>(items);}
 public int Count{get{Console.Write(label+"C;");return data.Count;}}public bool IsReadOnly{get{return false;}}
 public string this[int i]{get{Console.Write(label+"G"+i+";");return data[i];}set{data[i]=value;}}
 public void Add(string x){data.Add(x);}public void Clear(){data.Clear();}public bool Contains(string x){return data.Contains(x);}public void CopyTo(string[] a,int i){data.CopyTo(a,i);}public bool Remove(string x){return data.Remove(x);}
 public int IndexOf(string x){return data.IndexOf(x);}public void Insert(int i,string x){data.Insert(i,x);}public void RemoveAt(int i){data.RemoveAt(i);}
 public IEnumerator<string> GetEnumerator(){Console.Write(label+"E;");return data.GetEnumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
}
