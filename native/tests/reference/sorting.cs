// Development-only independent Framework oracle; never installed.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.Versioning;
using System.Text;
[assembly:TargetFramework(".NETFramework,Version=v4.7.2")]
public static class SortingReference {
 static Item[] items;static StringBuilder log;static int mode,calls,scenarioId;static bool detectHeap,heapSeen;
 sealed class Item:IComparable<Item> {
  public int id,score;public Item(int i,int value){id=i;score=value;}
  public int CompareTo(Item other){Touch("C("+Tag(this)+","+Tag(other)+");");if(mode==4)return -1;if(other==null)return 1;return mode==1?other.score-score:score-other.score;}
 }
 sealed class Key:IComparable<Key> {
  public int id,score;public Key(int i,int value){id=i;score=value;}
  public int CompareTo(Key other){Touch("Q("+KeyTag(this)+","+KeyTag(other)+");");if(mode==4)return -1;if(other==null)return 1;return mode==1?other.score-score:score-other.score;}
 }
 static string Tag(Item x){return x==null?"nil":x.id+":"+x.score;}
 static string KeyTag(Key x){return x==null?"nil":x.id+":"+x.score;}
 static string Dump(){var b=new StringBuilder("[");for(int i=0;i<items.Length;i++){if(i!=0)b.Append(",");b.Append(Tag(items[i]));}return b.Append("]").ToString();}
 static void Touch(string text){
  if(detectHeap && !heapSeen)foreach(var frame in new System.Diagnostics.StackTrace().GetFrames()){var name=frame.GetMethod().Name;if(name=="DownHeap" || name=="Heapsort")heapSeen=true;}
  log.Append(text);calls++;if(mode==2 || mode==3)log.Append(Dump()).Append(";");
  if(calls!=3)return;
  if(mode==2)throw new DivideByZeroException();if(mode==3)throw new IndexOutOfRangeException();
  if(mode==5 && items.Length>1){var x=items[0];items[0]=items[items.Length-1];items[items.Length-1]=x;log.Append("S;");}
  if(mode==6){foreach(var x in items)if(x!=null){x.score+=7;break;}log.Append("V;");}
  if(mode==7 && items.Length>0){items[0]=new Item(999,-77);log.Append("R;");}
 }
 static int Compare(Item a,Item b,string label){Touch(label+"("+Tag(a)+","+Tag(b)+");");if(mode==4)return -1;if(a==null)return b==null?0:-1;if(b==null)return 1;return mode==1?b.score-a.score:a.score-b.score;}
 static bool Less(Item a,Item b){return Compare(a,b,"L")<0;}
 static int IntKey(Item x){Touch("K("+Tag(x)+");");return x==null?-99:x.score%3;}
 static Key ClassKey(Item x){Touch("K("+Tag(x)+");");return x==null?null:new Key(x.id,scenarioId>=12?x.score:x.score%3);}
 static string StringKey(Item x){Touch("K("+Tag(x)+");");return x==null?null:x.score<0?"z":x.score==0?"a":"A";}
 static void Run(int scenario){
  switch(scenario){
   case 0:case 1:case 12:Array.Sort(items);break;
   case 2:Array.Sort(items);Array.Reverse(items);break;
   case 3:Array.Sort(items,(a,b)=>Compare(a,b,"F"));break;
   case 4:Array.Sort(items,(a,b)=>Less(a,b)?-1:Less(b,a)?1:0);break;
   case 5:case 8:var ik=Array.ConvertAll(items,IntKey);Array.Sort(ik,items);if(scenario==8)Array.Reverse(items);break;
   case 6:case 9:case 10:case 13:var ck=Array.ConvertAll(items,ClassKey);Array.Sort(ck,items);break;
   case 7:var sk=Array.ConvertAll(items,StringKey);Array.Sort(sk,items,StringComparer.Ordinal);break;
   case 11:var gk=Array.ConvertAll(items,StringKey);Array.Sort(gk,items);break;
  }
 }
 static int Killer(int i,int n){return i<n/2?2*i:2*(n-i)-1;}
 static void Emit(StreamWriter output,int scenario,int size,int seed,int selectedMode,int nil){
    mode=selectedMode;scenarioId=scenario;detectHeap=scenario>=12;heapSeen=false;
    calls=0;log=new StringBuilder();items=new Item[size];for(int i=0;i<size;i++)if(nil==0 || (i+seed)%5!=0)items[i]=new Item(i,detectHeap?Killer(i,size):(i*17+seed*13)%7-3);
    var code="ok";try{Run(scenario);log.Append("END").Append(Dump());}catch(InvalidOperationException){code="E_SORT";}catch(ArgumentException){code="E_COMPARER";}catch(IndexOutOfRangeException){code="E_BOUNDS";}catch(DivideByZeroException){code="E_DIV_ZERO";}
    if(detectHeap && !heapSeen)throw new Exception("Heap fallback was not reached: "+scenario+" seed "+seed+" calls "+calls);
    output.WriteLine(scenario+"\t"+size+"\t"+seed+"\t"+mode+"\t"+nil+"\t"+log+"\t"+code);
 }
 public static void Main(string[] args){
  var property=typeof(object).Assembly.GetType("System.Runtime.Versioning.BinaryCompatibility").GetProperty("TargetsAtLeast_Desktop_V4_5",BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic);
  if(property==null || !(bool)property.GetValue(null,null))throw new Exception("Reference process must select Framework 4.5+ introsort");
  System.Threading.Thread.CurrentThread.CurrentCulture=CultureInfo.GetCultureInfo("en-US");
  using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){
   output.WriteLine("# Framework "+Environment.Version+"; target .NETFramework,Version=v4.7.2; introsort=true");
   for(int scenario=0;scenario<12;scenario++)foreach(int size in new[]{0,1,2,3,4,8,16,17,31,64})foreach(int seed in new[]{1,17})for(mode=0;mode<8;mode++)for(int nil=0;nil<2;nil++){
    Emit(output,scenario,size,seed,mode,nil);
   }
   for(int scenario=12;scenario<=13;scenario++)foreach(int seed in new[]{17})Emit(output,scenario,1024,seed,0,0);
  }
 }
}
