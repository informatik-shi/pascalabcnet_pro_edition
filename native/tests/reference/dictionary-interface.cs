// Independent Framework protocol recipes; development-only, never installed.
using System;using System.IO;using System.Text;using System.Linq;using System.Collections;using System.Collections.Generic;using System.Globalization;
class TraceDictionary:IDictionary<string,int>{
 readonly Dictionary<string,int> data=new Dictionary<string,int>();
 public TraceDictionary(int size){if(size!=0){data.Add("a",1);data.Add("b",2);}}
 public int Count{get{Console.Write("C");return data.Count;}}public bool IsReadOnly{get{Console.Write("O");return false;}}
 public ICollection<string> Keys{get{Console.Write("K");return data.Keys;}}public ICollection<int> Values{get{Console.Write("V");return data.Values;}}
 public int this[string key]{get{Console.Write("G");return data[key];}set{Console.Write("S");data[key]=value;}}
 public void Add(string key,int value){Console.Write("A");data.Add(key,value);}public void Add(KeyValuePair<string,int> p){Console.Write("P");((ICollection<KeyValuePair<string,int>>)data).Add(p);}
 public bool Remove(string key){Console.Write("D");return data.Remove(key);}public bool Remove(KeyValuePair<string,int> p){Console.Write("R");return ((ICollection<KeyValuePair<string,int>>)data).Remove(p);}
 public void Clear(){Console.Write("L");data.Clear();}public bool ContainsKey(string key){Console.Write("F");return data.ContainsKey(key);}public bool Contains(KeyValuePair<string,int> p){Console.Write("H");return ((ICollection<KeyValuePair<string,int>>)data).Contains(p);}
 public void CopyTo(KeyValuePair<string,int>[] a,int i){Console.Write("Y");((ICollection<KeyValuePair<string,int>>)data).CopyTo(a,i);}public bool TryGetValue(string key,out int value){Console.Write("T");return data.TryGetValue(key,out value);}
 public IEnumerator<KeyValuePair<string,int>> GetEnumerator(){Console.Write("E");return data.GetEnumerator();}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
}
class Reference{
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("X4"));return b.ToString();}
 static string Error(Exception e,int operation){if(e is NullReferenceException||e is ArgumentNullException)return "E_NIL";if(e is KeyNotFoundException)return "E_KEY_NOT_FOUND";if(e is NotSupportedException)return "E_READONLY";if(e is ArgumentException)return operation==2||operation==13?"E_KEY_DUPLICATE":"E_RANGE";throw e;}
 static string Pair(KeyValuePair<string,int> p){return "("+(p.Key??"nil")+","+p.Value+")";}
 static void ArrayText(IEnumerable<KeyValuePair<string,int>> items){Console.WriteLine("["+string.Join(",",items.Select(Pair))+"]");}
 static void Items(IDictionary<string,int> d){foreach(var p in d)Console.Write((p.Key??"nil")+"="+p.Value+",");Console.WriteLine();}
 static void Main(string[] args){CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;var original=Console.Out;int rows=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# kind operation size nil key value offset output_utf16 error; Framework "+Environment.Version);
  for(int kind=0;kind<2;kind++)for(int op=0;op<21;op++)foreach(int size in new[]{0,2})for(int nil=0;nil<2;nil++)for(int keyKind=0;keyKind<3;keyKind++)foreach(int value in new[]{1,9})foreach(int offset in op==5?new[]{-1,0,1,3,4}:new[]{0}){
   var native=new Dictionary<string,int>();if(size!=0){native.Add("a",1);native.Add("b",2);}IDictionary<string,int> d=kind==0?(IDictionary<string,int>)native:new TraceDictionary(size);if(nil!=0)d=null;string key=keyKind==0?"a":keyKind==1?"x":null;var pair=new KeyValuePair<string,int>(key,value);
   var output=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(output);string error="ok";try{switch(op){
    case 0:Console.WriteLine(d.Count);break;case 1:Console.WriteLine(d.IsReadOnly);break;case 2:d.Add(pair);Items(d);break;case 3:d.Clear();Items(d);break;case 4:Console.WriteLine(d.Contains(pair));break;
    case 5:var a=new KeyValuePair<string,int>[3];d.CopyTo(a,offset);ArrayText(a);break;case 6:Console.WriteLine(d.Remove(pair));Items(d);break;case 7:Items(d);break;
    case 8:Console.WriteLine(d.Keys.Count+" "+d.Keys.IsReadOnly);break;case 9:Console.WriteLine(d.Values.Count+" "+d.Values.Contains(1));break;
    case 10:Console.WriteLine(d[key]);break;case 11:d[key]=value;Items(d);break;case 12:Console.WriteLine(d.ContainsKey(key));break;case 13:d.Add(key,value);Items(d);break;case 14:Console.WriteLine(d.Remove(key));Items(d);break;
    case 15:int result=99;Console.WriteLine(d.TryGetValue(key,out result)+" "+result);break;
    case 16:ICollection<KeyValuePair<string,int>> c=d;Console.WriteLine(c.Count+" "+c.Contains(pair));break;case 17:Console.WriteLine(d.AsEnumerable().Count());break;case 18:ArrayText(d.AsEnumerable().ToArray());break;
    case 19:d[key]=d[key]+value;Items(d);break;case 20:Console.WriteLine(d.Keys.Remove(key));break;
   }}catch(Exception e){error=Error(e,op);}Console.SetOut(original);file.WriteLine(kind+"\t"+op+"\t"+size+"\t"+nil+"\t"+keyKind+"\t"+value+"\t"+offset+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
  }
 }Console.WriteLine(rows+" independent IDictionary outcomes");}
}
