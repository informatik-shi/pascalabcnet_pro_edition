// Development-only independent Framework oracle; never installed.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.Versioning;
using System.Text;
[assembly:TargetFramework(".NETFramework,Version=v4.7.2")]
public static class ArraySortReference {
 static Item[] items;static StringBuilder log;static int mode,calls;static bool detectHeap,heapSeen;
 sealed class Item:IComparable<Item> {
  public int id,score;public Item(int i,int value){id=i;score=value;}
  public int CompareTo(Item other){Touch("C("+Tag(this)+","+Tag(other)+");");if(mode==4)return -1;if(other==null)return 1;return mode==1?other.score-score:score-other.score;}
 }
 static string Tag(Item x){return x==null?"nil":x.id+":"+x.score;}
 static string Dump(){var b=new StringBuilder("[");for(int i=0;i<items.Length;i++){if(i!=0)b.Append(",");b.Append(Tag(items[i]));}return b.Append("]").ToString();}
 static void Touch(string text){
  if(detectHeap && !heapSeen)foreach(var frame in new System.Diagnostics.StackTrace().GetFrames()){var name=frame.GetMethod().Name;if(name=="DownHeap" || name=="Heapsort")heapSeen=true;}
  log.Append(text);calls++;if(mode==2 || mode==3)log.Append(Dump()).Append(";");
  if(calls!=3)return;
  if(mode==2)throw new DivideByZeroException();if(mode==3)throw new IndexOutOfRangeException();
  if(mode==5 && items.Length>1){var x=items[0];items[0]=items[items.Length-1];items[items.Length-1]=x;log.Append("S;");}
  if(mode==6){foreach(var x in items)if(x!=null){x.score+=7;break;}log.Append("V;");}
  if(mode==7 && items.Length>0){items[0]=new Item(999,-77);log.Append("R;");}
  if(mode==8){items=new[]{new Item(777,42)};log.Append("A;");}
 }
 static int Compare(Item a,Item b,string label){Touch(label+"("+Tag(a)+","+Tag(b)+");");if(mode==4)return -1;if(a==null)return b==null?0:-1;if(b==null)return 1;return mode==1?b.score-a.score:a.score-b.score;}
 interface IOrder:IComparer<Item>{}
 sealed class Order:IComparer<Item>{public int Compare(Item a,Item b){return ArraySortReference.Compare(a,b,"I");}}
 sealed class NamedOrder:IOrder{public int Compare(Item a,Item b){return ArraySortReference.Compare(a,b,"N");}}
 static int index,count;
 static void Run(int scenario){
  switch(scenario){
   case 0:Array.Sort(items);break;
   case 1:Array.Sort(items,(Comparison<Item>)((a,b)=>Compare(a,b,"F")));break;
   case 2:Array.Sort(items,Comparer<Item>.Default);break;
   case 3:Array.Sort(items,Comparer<Item>.Create((a,b)=>Compare(a,b,"F")));break;
   case 4:Array.Sort(items,new Order());break;
   case 5:Array.Sort(items,new NamedOrder());break;
   case 6:case 11:Array.Sort(items,index,count);break;
   case 7:Array.Sort(items,index,count,(IComparer<Item>)null);break;
   case 8:case 12:Array.Sort(items,index,count,new Order());break;
   case 9:Array.Sort(items,(IComparer<Item>)null);break;
   case 10:Array.Sort(items,(Comparison<Item>)null);break;
  }
 }
 static int Killer(int i,int n){return i<n/2?2*i:2*(n-i)-1;}
 static void Emit(StreamWriter output,int scenario,int size,int seed,int selectedMode,int nil,int start,int length){
  mode=selectedMode;index=start;count=length;detectHeap=scenario>=11;heapSeen=false;
  calls=0;log=new StringBuilder();items=new Item[size];for(int i=0;i<size;i++)if(nil==0 || (i+seed)%5!=0)items[i]=new Item(i,detectHeap?Killer(i,size):(i*17+seed*13)%7-3);
  var code="ok";try{Run(scenario);log.Append("END").Append(Dump());}
  catch(ArgumentNullException){code="E_NIL";}catch(InvalidOperationException){code="E_SORT";}
  catch(ArgumentOutOfRangeException){code="E_RANGE";}
  catch(ArgumentException){code=(index<0 || count<0 || index>size || count>size-index)?"E_RANGE":"E_COMPARER";}
  catch(IndexOutOfRangeException){code="E_BOUNDS";}catch(DivideByZeroException){code="E_DIV_ZERO";}
  if(detectHeap && !heapSeen)throw new Exception("Heap fallback was not reached: "+scenario+" calls "+calls);
  output.WriteLine(scenario+"\t"+size+"\t"+seed+"\t"+mode+"\t"+nil+"\t"+index+"\t"+count+"\t"+log+"\t"+code);
 }
 public static void Main(string[] args){
  var property=typeof(object).Assembly.GetType("System.Runtime.Versioning.BinaryCompatibility").GetProperty("TargetsAtLeast_Desktop_V4_5",BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic);
  if(property==null || !(bool)property.GetValue(null,null))throw new Exception("Framework 4.5+ introsort required");
  System.Threading.Thread.CurrentThread.CurrentCulture=CultureInfo.GetCultureInfo("en-US");
  System.Threading.Thread.CurrentThread.CurrentUICulture=CultureInfo.GetCultureInfo("en-US");
  using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){
   output.WriteLine("# Framework "+Environment.Version+"; target .NETFramework,Version=v4.7.2; introsort=true");
   for(int scenario=0;scenario<=10;scenario++)foreach(int size in new[]{0,1,2,3,4,8,16,17,32})foreach(int seed in new[]{1,17})for(int mode=0;mode<9;mode++)for(int nil=0;nil<2;nil++)Emit(output,scenario,size,seed,mode,nil,size>1?1:0,Math.Max(0,size-2));
   foreach(int scenario in new[]{6,7,8})foreach(int size in new[]{0,1,3})foreach(int start in new[]{-1,0,1,3,int.MaxValue})foreach(int length in new[]{-1,0,1,3,int.MaxValue})Emit(output,scenario,size,1,0,0,start,length);
   for(int scenario=11;scenario<=12;scenario++)Emit(output,scenario,1024,17,0,0,0,1024);
  }
 }
}
