// Independent development oracle; managed binaries are never installed.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.Versioning;
using System.Text;
[assembly:TargetFramework(".NETFramework,Version=v4.7.2")]
public static class ListSortingReference {
 static List<Item> list,alias;static IEnumerator<Item> oldCursor;static StringBuilder log;static int mode,calls,scenarioId;
 static bool detectHeap,heapSeen;
 sealed class Item:IComparable<Item>{
  public int id,score;public Item(int i,int n){id=i;score=n;}
  public int CompareTo(Item other){Touch("C("+Tag(this)+","+Tag(other)+");");if(mode==4)return -1;if(other==null)return 1;return mode==1?other.score-score:score-other.score;}
 }
 sealed class Key:IComparable<Key>{
  public int id,score;public Key(int i,int n){id=i;score=n;}
  public int CompareTo(Key other){Touch("Q("+KeyTag(this)+","+KeyTag(other)+");");if(mode==4)return -1;if(other==null)return 1;return mode==1?other.score-score:score-other.score;}
 }
 interface NamedOrder:IComparer<Item>{}
 sealed class Order:NamedOrder{public int Compare(Item a,Item b){return CompareItems(a,b,"U");}}
 sealed class Guard:IEnumerable<int>,IEnumerator<int>{
  int n;public IEnumerator<int> GetEnumerator(){return this;}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
  public bool MoveNext(){return ++n==1;}public int Current{get{return 0;}}object IEnumerator.Current{get{return Current;}}
  public void Reset(){n=0;}public void Dispose(){Finish();}
 }
 sealed class Empty:IEnumerable<Item>,IEnumerator<Item>{
  public IEnumerator<Item> GetEnumerator(){return this;}IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
  public bool MoveNext(){return false;}public Item Current{get{return null;}}object IEnumerator.Current{get{return Current;}}
  public void Reset(){}public void Dispose(){var next=oldCursor.MoveNext();log.Append("Y").Append(next).Append(";");}
 }
 static string Tag(Item x){return x==null?"nil":x.id+":"+x.score;}
 static string KeyTag(Key x){return x==null?"nil":x.id+":"+x.score;}
 static string Dump(List<Item> data){if(data==null)return "nil";var b=new StringBuilder("[");for(int i=0;i<data.Count;i++){if(i>0)b.Append(",");b.Append(Tag(data[i]));}return b.Append("]").ToString();}
 static void Finish(){var l=Dump(list);var a=Dump(alias);log.Append("Z").Append(l).Append("|").Append(a).Append("|").Append(Object.ReferenceEquals(list,alias)).Append("|E");bool next=oldCursor.MoveNext();log.Append(next);if(next)log.Append(":").Append(Tag(oldCursor.Current));log.Append(";");}
 static int Plain(Item a,Item b){if(a==null)return b==null?0:-1;if(b==null)return 1;return a.score.CompareTo(b.score);}
 static void Touch(string text){
  if(detectHeap && !heapSeen)foreach(var frame in new System.Diagnostics.StackTrace().GetFrames()){var name=frame.GetMethod().Name;if(name=="DownHeap" || name=="Heapsort")heapSeen=true;}
  log.Append(text);calls++;if(mode==2 || mode==3)log.Append(Dump(list)).Append(";");
  if(calls!=3)return;
  if(mode==2)throw new DivideByZeroException();
  if(mode==3){var missing=list[999999];}
  if(mode==5 && list.Count>0){list[0]=new Item(999,-77);log.Append("R;");}
  if(mode==6){foreach(var x in list)if(x!=null){x.score+=7;break;}log.Append("V;");}
  if(mode==7 && list.Count>1){var x=list[0];list[0]=list[list.Count-1];list[list.Count-1]=x;log.Append("S;");}
  if(mode==8){list.Add(new Item(998,-66));log.Append("A;");}
  if(mode==9){list.Capacity=list.Count+9;log.Append("B;");}
  if(mode==10){list.Clear();log.Append("C;");}
  if(mode==11 && list.Count>0){list.RemoveAt(0);log.Append("D;");}
  if(mode==12){list.Insert(0,new Item(997,-55));log.Append("I;");}
  if(mode==13){list.RemoveRange(0,Math.Min(2,list.Count));log.Append("M;");}
  if(mode==14){list.InsertRange(Math.Min(1,list.Count),new[]{new Item(996,-44),new Item(995,44)});log.Append("J;");}
  if(mode==15){list.AddRange(new Item[0]);log.Append("N;");}
  if(mode==16){list.TrimExcess();log.Append("T;");}
  if(mode==17){list=new List<Item>(list);log.Append("G;");}
  if(mode==18){list.Sort((Comparison<Item>)Plain);log.Append("H;");}
  if(mode==19 && list.Count>0){list.RemoveAt(0);list.Add(new Item(994,-33));log.Append("P;");}
  if(mode==20){list.Clear();list.Add(new Item(993,-22));list.Add(new Item(992,22));log.Append("W;");}
  if(mode==21){list.AddRange(new Empty());log.Append("O;");}
  if(mode==22){list=null;log.Append("X;");}
 }
 static int CompareItems(Item a,Item b,string label){Touch(label+"("+Tag(a)+","+Tag(b)+");");if(mode==4)return -1;if(a==null)return b==null?0:-1;if(b==null)return 1;return mode==1?b.score-a.score:a.score-b.score;}
 static int CompareFunction(Item a,Item b){return CompareItems(a,b,"F");}
 static bool Less(Item a,Item b){return CompareItems(a,b,"L")<0;}
 static int IntKey(Item x){Touch("K("+Tag(x)+");");return x==null?-99:x.score%3;}
 static Key ClassKey(Item x){Touch("K("+Tag(x)+");");return x==null?null:new Key(x.id,scenarioId>=17?x.score:x.score%3);}
 static string StringKey(Item x){Touch("K("+Tag(x)+");");return x==null?null:x.score<0?"z":x.score==0?"a":"A";}
 static void KeySort<T>(Func<Item,T> selector,IComparer<T> order){var array=list.ToArray();var keys=Array.ConvertAll(array,x=>selector(x));Array.Sort(keys,array,order);list=new List<Item>(array);}
 static void Run(int scenario,int size){
  switch(scenario){
   case 0:case 1:case 17:list.Sort();break;
   case 2:var target=list;target.Sort();target.Reverse();break;
   case 3:case 4:list.Sort((Comparison<Item>)CompareFunction);break;
   case 5:list.Sort((a,b)=>Less(a,b)?-1:Less(b,a)?1:0);break;
   case 6:case 9:KeySort(IntKey,null);if(scenario==9)list.Reverse();break;
   case 7:case 18:KeySort(ClassKey,null);break;
   case 8:KeySort(StringKey,StringComparer.Ordinal);break;
   case 10:list.Sort(new Order());break;
   case 11:list.Sort(Comparer<Item>.Default);break;
   case 12:list.Sort(Comparer<Item>.Create(CompareFunction));break;
   case 13:list.Sort(size>1?1:0,size>2?size-2:0,new Order());break;
   case 14:list.Sort(size>1?1:0,size>2?size-2:0,null);break;
   case 15:KeySort(StringKey,null);break;
   case 16:NamedOrder order=new Order();list.Sort(order);break;
  }
 }
 static void Emit(StreamWriter output,int scenario,int size,int pad,int selectedMode,int nil){
  scenarioId=scenario;mode=selectedMode;calls=0;detectHeap=scenario>=17;heapSeen=false;log=new StringBuilder();list=new List<Item>(size+pad);
  for(int i=0;i<size;i++)list.Add(nil!=0 && (i+1)%5==0?null:new Item(i,detectHeap?(i<size/2?2*i:2*(size-i)-1):(i*17+13)%7-3));
  alias=list;oldCursor=list.GetEnumerator();string code="ok";
  try{foreach(var ignored in new Guard()){Run(scenario,size);log.Append("DONE;");}}
  catch(InvalidOperationException e){code=e.Message.StartsWith("Collection was modified",StringComparison.Ordinal)?"E_MODIFIED":"E_SORT";}
  catch(ArgumentException){code="E_RANGE";}catch(NullReferenceException){code="E_NIL";}catch(DivideByZeroException){code="E_DIV_ZERO";}catch(IndexOutOfRangeException){code="E_BOUNDS";}
  // Bogus comparison is an ArgumentException; key extraction index errors retain ArgumentOutOfRange.
  if(code=="E_RANGE" && mode==4)code="E_COMPARER";
  if(detectHeap && !heapSeen)throw new Exception("Heap fallback not reached: "+scenario+" pad "+pad);
  output.WriteLine(scenario+"\t"+size+"\t"+pad+"\t"+mode+"\t"+nil+"\t"+log+"\t"+code);
 }
 struct Record{public int n;}static List<Record> records;
 static void Borrow(ref int n){records.Capacity=20;records.TrimExcess();n=7;log.Append("B").Append(n).Append(";");}
 static void EmitBorrow(StreamWriter output){
  records=new List<Record>{default(Record)};var cursor=records.GetEnumerator();var field=typeof(List<Record>).GetField("_items",BindingFlags.Instance|BindingFlags.NonPublic);var old=(Record[])field.GetValue(records);log=new StringBuilder();Borrow(ref old[0].n);var next=cursor.MoveNext();
  log.Append(records[0].n).Append(" ").Append(records.Capacity).Append(" ").Append(next).Append(" ").Append(cursor.Current.n);
  output.WriteLine("21\t1\t0\t0\t0\t"+log+"\tok");
 }
 static void EmitRange(StreamWriter output,int scenario,int size,int index,int count){
  scenarioId=scenario;mode=0;calls=0;detectHeap=false;heapSeen=false;log=new StringBuilder();list=new List<Item>(size);
  for(int i=0;i<size;i++)list.Add(new Item(i,(i*17+13)%7-3));alias=list;oldCursor=list.GetEnumerator();var code="ok";
  try{foreach(var ignored in new Guard()){list.Sort(index,count,scenario==20?(IComparer<Item>)new Order():null);log.Append("DONE;");}}
  catch(InvalidOperationException e){code=e.Message.StartsWith("Collection was modified",StringComparison.Ordinal)?"E_MODIFIED":"E_SORT";}catch(ArgumentException){code="E_RANGE";}
  output.WriteLine(scenario+"\t"+size+"\t"+index+"\t"+count+"\t0\t"+log+"\t"+code);
 }
 public static void Main(string[] args){
  var p=typeof(object).Assembly.GetType("System.Runtime.Versioning.BinaryCompatibility").GetProperty("TargetsAtLeast_Desktop_V4_5",BindingFlags.Static|BindingFlags.Public|BindingFlags.NonPublic);
  if(p==null || !(bool)p.GetValue(null,null))throw new Exception("Framework 4.5+ sort compatibility required");
  System.Threading.Thread.CurrentThread.CurrentCulture=CultureInfo.GetCultureInfo("en-US");
  System.Threading.Thread.CurrentThread.CurrentUICulture=CultureInfo.GetCultureInfo("en-US");
  using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){
   output.WriteLine("# Independent Framework List sorting; target .NETFramework,Version=v4.7.2; introsort=true");
   for(int scenario=0;scenario<17;scenario++)foreach(int size in new[]{0,1,2,3,4,8,16,17,32})foreach(int pad in new[]{0,7})for(int m=0;m<23;m++)for(int nil=0;nil<2;nil++)Emit(output,scenario,size,pad,m,nil);
   for(int scenario=17;scenario<=18;scenario++)foreach(int pad in new[]{0,3000})Emit(output,scenario,1024,pad,0,0);
   for(int scenario=19;scenario<=20;scenario++)foreach(int size in new[]{0,1,3})foreach(int index in new[]{-1,0,1,3,int.MaxValue})foreach(int count in new[]{-1,0,1,3,int.MaxValue})EmitRange(output,scenario,size,index,count);
   EmitBorrow(output);
  }
 }
}
