// Development-only independent Framework oracle; never installed.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.Versioning;
using System.Text;
[assembly:TargetFramework(".NETFramework,Version=v4.7.2")]
public static class PairedSortingReference {
 static Item[] keys,items,oldKeys,oldItems;static StringBuilder log;static int mode,calls,index,count;static bool detectHeap,heapSeen;
 sealed class Item:IComparable<Item>{
  public int id,score;public Item(int i,int value){id=i;score=value;}
  public int CompareTo(Item other){Touch("C("+Tag(this)+","+Tag(other)+");");if(mode==4)return -1;if(other==null)return 1;return mode==1?other.score-score:score-other.score;}
 }
 interface IOrder:IComparer<Item>{}
 sealed class Order:IComparer<Item>{public int Compare(Item a,Item b){return PairedSortingReference.Compare(a,b,"I");}}
 sealed class NamedOrder:IOrder{public int Compare(Item a,Item b){return PairedSortingReference.Compare(a,b,"N");}}
 static string Tag(Item x){return x==null?"nil":x.id+":"+x.score;}
 static string Dump(Item[] a){if(a==null)return "nil";var b=new StringBuilder("[");for(int i=0;i<a.Length;i++){if(i!=0)b.Append(",");b.Append(Tag(a[i]));}return b.Append("]").ToString();}
 static void Touch(string text){
  if(detectHeap && !heapSeen)foreach(var frame in new System.Diagnostics.StackTrace().GetFrames()){var name=frame.GetMethod().Name;if(name=="DownHeap" || name=="Heapsort")heapSeen=true;}
  log.Append(text);calls++;if(mode==2 || mode==3)log.Append(Dump(keys)).Append(" ").Append(Dump(items)).Append(";");
  if(calls!=3)return;
  if(mode==2)throw new DivideByZeroException();if(mode==3)throw new IndexOutOfRangeException();
  if(mode==5 && keys.Length>1){var x=keys[0];keys[0]=keys[keys.Length-1];keys[keys.Length-1]=x;log.Append("S;");}
  if(mode==6){foreach(var x in keys)if(x!=null){x.score+=7;break;}log.Append("V;");}
  if(mode==7 && keys.Length>0){keys[0]=new Item(999,-77);log.Append("R;");}
  if(mode==8){keys=new[]{new Item(777,42)};log.Append("A;");}
  if(mode==9 && items!=null && items.Length>1){var x=items[0];items[0]=items[items.Length-1];items[items.Length-1]=x;log.Append("T;");}
  if(mode==10 && items!=null && items.Length>0){items[0]=new Item(888,-66);log.Append("B;");}
  if(mode==11){items=new[]{new Item(666,24)};log.Append("D;");}
  if(mode==12){items=null;log.Append("Z;");}
  if(mode==13 && items!=null){foreach(var x in items)if(x!=null){x.score+=11;break;}log.Append("W;");}
  if(mode==14){keys=null;log.Append("H;");}
 }
 static int Compare(Item a,Item b,string label){Touch(label+"("+Tag(a)+","+Tag(b)+");");if(mode==4)return -1;if(a==null)return b==null?0:-1;if(b==null)return 1;return mode==1?b.score-a.score:a.score-b.score;}
 static void Run(int scenario){switch(scenario){
  case 0:Array.Sort(keys,items);break;
  case 1:Array.Sort(keys,items,Comparer<Item>.Default);break;
  case 2:Array.Sort(keys,items,Comparer<Item>.Create((a,b)=>Compare(a,b,"F")));break;
  case 3:Array.Sort(keys,items,new Order());break;
  case 4:Array.Sort(keys,items,new NamedOrder());break;
  case 5:case 8:Array.Sort(keys,items,index,count);break;
  case 6:Array.Sort(keys,items,index,count,(IComparer<Item>)null);break;
  case 7:case 9:Array.Sort(keys,items,index,count,new Order());break;
 }}
 static int Killer(int i,int n){return i<n/2?2*i:2*(n-i)-1;}
 static bool BadRange(int scenario){int start=scenario>=5?index:0,length=scenario>=5?count:oldKeys.Length;return start<0 || length<0 || start>oldKeys.Length || length>oldKeys.Length-start || (oldItems!=null && (start>oldItems.Length || length>oldItems.Length-start));}
 static void Emit(StreamWriter output,int scenario,int size,int kind,int selectedMode,int nil,int start,int length){
  mode=selectedMode;index=start;count=length;detectHeap=scenario>=8 && kind!=1;heapSeen=false;calls=0;log=new StringBuilder();
  keys=new Item[size];for(int i=0;i<size;i++)if(nil==0 || (i+17)%5!=0)keys[i]=new Item(i,scenario>=8?Killer(i,size):(i*17+17*13)%7-3);
  int itemCount=kind==3?size/2:kind==4?size+3:kind==6?0:size;items=new Item[itemCount];for(int i=0;i<itemCount;i++)if(nil==0 || i%3!=0)items[i]=kind==7?keys[i]:new Item(100+i,i%5);
  if(kind==1)items=keys;if(kind==2)items=null;if(kind==5)keys=null;oldKeys=keys;oldItems=items;
  var code="ok";try{Run(scenario);}catch(ArgumentNullException){code="E_NIL";}catch(InvalidOperationException){code="E_SORT";}catch(ArgumentOutOfRangeException){code="E_RANGE";}catch(ArgumentException){code=BadRange(scenario)?"E_RANGE":"E_COMPARER";}catch(IndexOutOfRangeException){code="E_BOUNDS";}catch(DivideByZeroException){code="E_DIV_ZERO";}
  finally{log.Append("|").Append(Dump(keys)).Append(" ").Append(Dump(items)).Append(" ").Append(Dump(oldKeys)).Append(" ").Append(Dump(oldItems)).Append(" ").Append(keys==oldKeys?"True":"False").Append(" ").Append(items==oldItems?"True":"False");}
  if(detectHeap && !heapSeen)throw new Exception("Heap fallback was not reached: "+scenario+" kind "+kind);
  output.WriteLine(scenario+"\t"+size+"\t"+kind+"\t"+mode+"\t"+nil+"\t"+index+"\t"+count+"\t"+log+"\t"+code);
 }
 static StringBuilder intLog;
 sealed class IntOrder:IComparer<int>{public int Compare(int a,int b){return IntCompare(a,b);}}
 static int IntCompare(int a,int b){intLog.Append("I(").Append(a).Append(",").Append(b).Append(");");return a-b;}
 static void IntSort<T>(int[] k,T[] v,int scenario,int start,int length){switch(scenario){
  case 0:Array.Sort(k,v);break;case 1:Array.Sort(k,v,Comparer<int>.Default);break;case 2:Array.Sort(k,v,Comparer<int>.Create(IntCompare));break;case 3:Array.Sort(k,v,new IntOrder());break;
  case 4:Array.Sort(k,v,start,length);break;case 5:Array.Sort(k,v,start,length,(IComparer<int>)null);break;case 6:Array.Sort(k,v,start,length,new IntOrder());break;
 }}
 static string IntDump<T>(T[] a){return a==null?"nil":"["+String.Join(",",a)+"]";}
 static void Primitive(StreamWriter output,int scenario,int size,int kind,int seed){
  intLog=new StringBuilder();var k=new int[size];for(int i=0;i<size;i++)k[i]=size==1024?Killer(i,size):(i*17+seed*13)%7-3;
  var v=new int[kind==5?size+3:size];for(int i=0;i<v.Length;i++)v[i]=100+i;if(kind==1)v=k;if(kind==2)v=null;
  var s=new string[size];for(int i=0;i<size;i++)s[i]="v"+i;if(kind==4)s=null;
  var code="ok";try{int start=size==1024?0:size>1?1:0,length=size==1024?size:Math.Max(0,size-2);if(kind==3 || kind==4)IntSort(k,s,scenario,start,length);else IntSort(k,v,scenario,start,length);}
  catch(ArgumentException){code="E_COMPARER";}catch(InvalidOperationException){code="E_SORT";}
  intLog.Append("|").Append(IntDump(k)).Append(" ").Append(kind==3 || kind==4?IntDump(s):IntDump(v));
  output.WriteLine(scenario+"\t"+size+"\t"+kind+"\t"+seed+"\t"+intLog+"\t"+code);
 }
 static string DoubleDump(double[] a){if(a==null)return "nil";var s=new string[a.Length];for(int i=0;i<a.Length;i++)s[i]=unchecked((ulong)BitConverter.DoubleToInt64Bits(a[i])).ToString("X16");return String.Join(",",s);}
 static string SingleDump(float[] a){if(a==null)return "nil";var s=new string[a.Length];for(int i=0;i<a.Length;i++)s[i]=BitConverter.ToUInt32(BitConverter.GetBytes(a[i]),0).ToString("X8");return String.Join(",",s);}
 static void Floating(StreamWriter output,int precision,int scenario,int size,int seed){
  var bits=new ulong[]{0x7ff8000000000001UL,0xfff8000000000002UL,0UL,0x8000000000000000UL,0x7ff0000000000000UL,0xfff0000000000000UL,0x3ff0000000000000UL,0xbff0000000000000UL,0x401c000000000000UL,0x7ff0000000000001UL,0xfff0000000000002UL};
  var fb=new uint[]{0x7fc00001U,0xffc00002U,0U,0x80000000U,0x7f800000U,0xff800000U,0x3f800000U,0xbf800000U,0x40e00000U,0x7f800001U,0xff800002U};
  var d=new double[size];var f=new float[size];var dv=new double[size];var fv=new float[size];var iv=new int[size];
  for(int i=0;i<size;i++){int at=(i*17+seed)%bits.Length;d[i]=BitConverter.Int64BitsToDouble(unchecked((long)bits[at]));f[i]=BitConverter.ToSingle(BitConverter.GetBytes(fb[at]),0);dv[i]=BitConverter.Int64BitsToDouble(unchecked((long)bits[(at+3)%bits.Length]));fv[i]=BitConverter.ToSingle(BitConverter.GetBytes(fb[(at+3)%fb.Length]),0);iv[i]=100+i;}
  if(scenario==2){dv=null;fv=null;}if(scenario==3){dv=d;fv=f;}
  int start=size>1?1:0,length=Math.Max(0,size-2);var code="ok";
  try{
   if(precision==0){switch(scenario){case 0:case 2:case 3:Array.Sort(d,dv);break;case 1:Array.Sort(d,iv);break;case 4:Array.Sort(d,dv,start,length);break;case 5:Array.Sort(d,iv,start,length);break;case 6:Array.Sort(d);break;case 7:var l=new List<double>(size+7);l.AddRange(d);l.Sort();d=l.ToArray();break;}}
   else{switch(scenario){case 0:case 2:case 3:Array.Sort(f,fv);break;case 1:Array.Sort(f,iv);break;case 4:Array.Sort(f,fv,start,length);break;case 5:Array.Sort(f,iv,start,length);break;case 6:Array.Sort(f);break;case 7:var l=new List<float>(size+7);l.AddRange(f);l.Sort();f=l.ToArray();break;}}
  }catch(ArgumentException){code="E_COMPARER";}catch(InvalidOperationException){code="E_SORT";}
  string k=precision==0?DoubleDump(d):SingleDump(f),v=scenario==1 || scenario==5?String.Join(",",iv):scenario>=6?"nil":precision==0?DoubleDump(dv):SingleDump(fv);
  output.WriteLine(precision+"\t"+scenario+"\t"+size+"\t"+seed+"\t"+k+"\t"+v+"\t"+code);
 }
 enum Small{N3,N2,N1,Z,P1,P2,P3}
 static void EnumCase(StreamWriter output,int scenario,int size,int seed){
  var k=new Small[size];var v=new Small[size];var ids=new int[size];for(int i=0;i<size;i++){k[i]=(Small)((i*17+seed*13)%7);v[i]=(Small)((i*3+seed)%7);ids[i]=100+i;}
  if(scenario==1)v=k;if(scenario==3)v=null;var code="ok";try{switch(scenario){case 0:case 1:case 3:Array.Sort(k,v);break;case 2:Array.Sort(k,ids);break;case 4:Array.Sort(k);break;case 5:var list=new List<Small>(size+7);list.AddRange(k);list.Sort();k=list.ToArray();break;case 6:Array.Sort(k,v,size>1?1:0,Math.Max(0,size-2));break;}}catch(ArgumentException){code="E_COMPARER";}catch(InvalidOperationException){code="E_SORT";}
  var a=Array.ConvertAll(k,x=>(int)x);var b=v==null?null:Array.ConvertAll(v,x=>(int)x);string payload=scenario==2?IntDump(ids):scenario==4 || scenario==5?"nil":IntDump(b);
  output.WriteLine(scenario+"\t"+size+"\t"+seed+"\t"+IntDump(a)+" "+payload+"\t"+code);
 }
 public static void Main(string[] args){
  var property=typeof(object).Assembly.GetType("System.Runtime.Versioning.BinaryCompatibility").GetProperty("TargetsAtLeast_Desktop_V4_5",BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic);
  if(property==null || !(bool)property.GetValue(null,null))throw new Exception("Framework 4.5+ introsort required");
  System.Threading.Thread.CurrentThread.CurrentCulture=CultureInfo.GetCultureInfo("en-US");System.Threading.Thread.CurrentThread.CurrentUICulture=CultureInfo.GetCultureInfo("en-US");
  using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){
   output.WriteLine("# Framework "+Environment.Version+"; target .NETFramework,Version=v4.7.2; introsort=true");
   for(int scenario=0;scenario<8;scenario++)foreach(int size in new[]{0,1,2,3,4,16,17,32})foreach(int kind in new[]{0,1,2,3,4,7})for(int mode=0;mode<15;mode++)for(int nil=0;nil<2;nil++)Emit(output,scenario,size,kind,mode,nil,size>1?1:0,Math.Max(0,size-2));
   foreach(int scenario in new[]{5,6,7})foreach(int size in new[]{0,1,3})for(int kind=0;kind<=6;kind++)foreach(int start in new[]{-1,0,1,3,int.MaxValue})foreach(int length in new[]{-1,0,1,3,int.MaxValue})Emit(output,scenario,size,kind,0,0,start,length);
   foreach(int scenario in new[]{8,9})foreach(int kind in new[]{0,1,2})Emit(output,scenario,1024,kind,0,0,0,1024);
  }
  using(var output=new StreamWriter(Path.Combine(Path.GetDirectoryName(args[0]),"paired-primitive-reference.tsv"),false,new UTF8Encoding(false))){
   output.WriteLine("# Framework primitive paired sorting");
   for(int scenario=0;scenario<7;scenario++)foreach(int size in new[]{0,1,2,3,4,8,16,17,32,64})for(int kind=0;kind<6;kind++)foreach(int seed in new[]{1,17})Primitive(output,scenario,size,kind,seed);
   foreach(int scenario in new[]{4,6})foreach(int kind in new[]{0,1,3})Primitive(output,scenario,1024,kind,17);
  }
  using(var output=new StreamWriter(Path.Combine(Path.GetDirectoryName(args[0]),"paired-floating-reference.tsv"),false,new UTF8Encoding(false))){
   output.WriteLine("# Framework binary64/binary32 sorting with exact NaN/zero/payload bits");
   for(int precision=0;precision<2;precision++)for(int scenario=0;scenario<8;scenario++)foreach(int size in new[]{0,1,2,3,4,8,16,17,32,64,1024})for(int seed=0;seed<4;seed++)Floating(output,precision,scenario,size,seed);
  }
  using(var output=new StreamWriter(Path.Combine(Path.GetDirectoryName(args[0]),"paired-enum-reference.tsv"),false,new UTF8Encoding(false))){
   output.WriteLine("# Framework nominal enum sorting and native primitive dispatch");
   for(int scenario=0;scenario<7;scenario++)foreach(int size in new[]{0,1,2,3,4,8,16,17,32,64,1024})foreach(int seed in new[]{1,17})EnumCase(output,scenario,size,seed);
  }
 }
}
