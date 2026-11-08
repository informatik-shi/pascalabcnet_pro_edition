// Independent development-only Framework oracle; never installed.
using System;using System.Collections;using System.Collections.Generic;using System.Linq;using System.IO;using System.Text;
[assembly:System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.7.2")]
public static class CollectionSearchReference {
 sealed class Item:IEquatable<Item>{public int id,n;public Item(int i,int v){id=i;n=v;}public bool Equals(Item b){Touch("E",this,b);return mode!=1 && b!=null && n==b.n;}}
 sealed class Order:IEqualityComparer<Item>{public bool Equals(Item a,Item b){Touch("C",a,b);return mode!=1 && (a==null?b==null:b!=null && a.n==b.n);}public int GetHashCode(Item a){throw new Exception("Contains must not hash");}}
 sealed class Guard:IEnumerable<int>,IEnumerator<int>{int n;public IEnumerator<int> GetEnumerator(){return this;}IEnumerator IEnumerable.GetEnumerator(){return this;}public bool MoveNext(){return ++n==1;}public int Current{get{return 0;}}object IEnumerator.Current{get{return 0;}}public void Reset(){n=0;}public void Dispose(){Finish();}}
 static List<Item> list,aliasList;static Item[] array,aliasArray;static IEnumerator<Item> old;static bool primed,active,busy;static int kind,mode,trigger,calls,observe;static StringBuilder log;
 static string Tag(Item x){return x==null?"nil":x.id+":"+x.n;}
 static void Touch(string label,Item a,Item b){if(!active || busy)return;log.Append(label).Append('(').Append(Tag(a)).Append(',').Append(Tag(b)).Append(");");calls++;if(mode==0 || mode==1 || calls!=trigger)return;busy=true;log.Append('M').Append(mode).Append(';');
 if(mode==2)throw new DivideByZeroException();if(mode==3)throw new IndexOutOfRangeException();
 if(mode==4){if(kind==0)for(int i=0;i<20;i++)list.Add(new Item(100+i,i%3));else{int n=array.Length;Array.Resize(ref array,n+7);for(int i=n;i<array.Length;i++)array[i]=new Item(100+i,i%3);}}
 if(mode==5){if(kind==0){list.Capacity=list.Count+9;if(list.Count>1)list[1]=new Item(999,9);}else{array=(Item[])array.Clone();if(array.Length>1)array[1]=new Item(999,9);}}
 if(mode==6){if(kind==0)list.Clear();else Array.Clear(array,0,array.Length);}
 if(mode==7){if(kind==0){if(list.Count>0)list.RemoveAt(0);}else if(array.Length>0){Array.Copy(array,1,array,0,array.Length-1);array[array.Length-1]=null;}}
 if(mode==8){if(kind==0)list.Insert(0,new Item(998,8));else{Array.Resize(ref array,array.Length+1);Array.Copy(array,0,array,1,array.Length-1);array[0]=new Item(998,8);}}
 if(mode==9){if(kind==0)list.Reverse();else Array.Reverse(array);}
 if(mode==10){if(kind==0)list=new List<Item>(list);else array=(Item[])array.Clone();}
 if(mode==11){if(kind==0)list=null;else array=null;}
 if(mode==12 && a!=null)a.n+=3;if(mode==13 && b!=null)b.n+=3;
 if(mode==14){if(kind==0){if(list.Count>0)list[0]=new Item(997,9);}else if(array.Length>0)array[0]=new Item(997,9);}
 if(mode==15){if(kind==0){list.Clear();list.Add(new Item(996,8));}else{Array.Clear(array,0,array.Length);array=new[]{new Item(996,8)};}}
 if(mode==16){if(kind==0){list.TrimExcess();if(list.Count>1)list[1]=new Item(995,9);}else{array=(Item[])array.Clone();if(array.Length>1)array[1]=new Item(995,9);}}
 if(mode==17){if(kind==0)list.RemoveRange(0,Math.Min(2,list.Count));else{int n=Math.Min(2,array.Length);Array.Copy(array,n,array,0,array.Length-n);Array.Clear(array,array.Length-n,n);}}
 if(mode==18 && kind==0)list.AddRange(new Item[0]);if(mode==19){if(kind==0)list.Capacity=-1;else throw new IndexOutOfRangeException();}busy=false;
 }
 static string Dump(IEnumerable<Item> data){if(data==null)return "nil";var s=new StringBuilder("[");bool first=true;foreach(var x in data){if(!first)s.Append(',');first=false;s.Append(Tag(x));}return s.Append(']').ToString();}
 static void Finish(){active=false;log.Append('Z').Append(Dump(kind==0?(IEnumerable<Item>)list:array)).Append('|').Append(Dump(kind==0?(IEnumerable<Item>)aliasList:aliasArray)).Append("|S").Append(kind==0?(list==null?-1:list.Count):(array==null?-1:array.Length)).Append("|C").Append(kind==0?(list==null?-1:list.Capacity):(array==null?-1:array.Length)).Append("|O").Append(primed?Tag(old.Current):"-");if(observe!=0){log.Append("|E");if(old==null)log.Append('-');else{bool n=old.MoveNext();log.Append(n);if(n)log.Append(':').Append(Tag(old.Current));}}log.Append(';');}
 static void Run(int op,Item item,int index,int count){IEnumerable<Item> source=kind==0?(IEnumerable<Item>)list:array;switch(op){
 case 0:log.Append(kind==0?list.Contains(item):((ICollection<Item>)array).Contains(item));break;
 case 1:log.Append(kind==0?list.IndexOf(item):Array.IndexOf(array,item));break;case 2:log.Append(kind==0?list.IndexOf(item,index):Array.IndexOf(array,item,index));break;case 3:log.Append(kind==0?list.IndexOf(item,index,count):Array.IndexOf(array,item,index,count));break;
 case 4:log.Append(kind==0?list.LastIndexOf(item):Array.LastIndexOf(array,item));break;case 5:log.Append(kind==0?list.LastIndexOf(item,index):Array.LastIndexOf(array,item,index));break;case 6:log.Append(kind==0?list.LastIndexOf(item,index,count):Array.LastIndexOf(array,item,index,count));break;case 7:log.Append(list.Remove(item));break;
 case 8:log.Append(Enumerable.Contains(source,item));break;case 9:log.Append(Enumerable.Contains(source,item,null));break;case 10:log.Append(Enumerable.Contains(source,item,new Order()));break;case 11:log.Append(Enumerable.Contains(source.Where(x=>true),item));break;case 12:log.Append(Enumerable.Contains(source.Where(x=>true),item,new Order()));break;case 13:log.Append(kind==0?list.Contains(item):Enumerable.Contains(source,item));break;
 }log.Append("DONE;");}
 static void Emit(StreamWriter output,int op,int selectedKind,int size,int pad,int probe,int selectedMode,int index,int count,int selectedObserve,int selectedTrigger){kind=selectedKind;mode=selectedMode;observe=selectedObserve;trigger=selectedTrigger;calls=0;busy=false;active=false;
 array=size<0?null:new Item[size];list=size<0?null:new List<Item>(size+pad);for(int i=0;i<size;i++){var item=(i+1)%4==0?null:new Item(i,(i*7+1)%3);array[i]=item;list.Add(item);}aliasList=list;aliasArray=array;old=size<0?null:kind==0?(IEnumerator<Item>)list.GetEnumerator():((IEnumerable<Item>)array).GetEnumerator();primed=old!=null && old.MoveNext();
 var q=probe==0?null:probe==1 && size>0?array[0]:probe==2 && size>1?array[1]:new Item(990+probe,probe==4?99:1);log=new StringBuilder();active=true;string code="ok",reason="other";
 try{foreach(var ignored in new Guard())Run(op,q,index,count);}catch(DivideByZeroException){code="E_DIV_ZERO";}catch(IndexOutOfRangeException){code="E_BOUNDS";}catch(ArgumentNullException){code="E_NIL";}catch(NullReferenceException){code="E_NIL";}catch(ArgumentOutOfRangeException e){code="E_RANGE";reason=e.ParamName=="count"?"count":e.ParamName=="value"?"capacity":"index";}catch(InvalidOperationException){code="E_MODIFIED";}
 output.WriteLine(op+"	"+kind+"	"+size+"	"+pad+"	"+probe+"	"+mode+"	"+index+"	"+count+"	"+observe+"	"+trigger+"	"+log+"	"+code+"	"+reason);
 }
 static void Normal(StreamWriter output,int op,int k,int size,int pad,int probe,int mode,int observe,int trigger){int index=(op==5 || op==6)?size-1:0,count=Math.Max(size,0);Emit(output,op,k,size,pad,probe,mode,index,count,observe,trigger);}
 public static void Main(string[] args){using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){output.WriteLine("# Independent Framework CLR "+Environment.Version+" List/Array default equality, physical search, Contains overloads, callback mutation, validation and preserved fault state");
 for(int op=0;op<14;op++)for(int k=0;k<2;k++)if(k==0 || op!=7)foreach(int size in new[]{-1,0,1,2,5,17})foreach(int pad in new[]{0,9})for(int probe=0;probe<5;probe++)for(int observe=0;observe<2;observe++)Normal(output,op,k,size,pad,probe,0,observe,1);
 foreach(int op in new[]{0,1,4,7,8,9,10,11,12,13})for(int k=0;k<2;k++)if(k==0 || op!=7)foreach(int size in new[]{5,17})foreach(int pad in new[]{0,9})for(int probe=0;probe<5;probe++)for(int mode=1;mode<=19;mode++)foreach(int trigger in new[]{1,3})for(int observe=0;observe<2;observe++)Normal(output,op,k,size,pad,probe,mode,observe,trigger);
 foreach(int op in new[]{2,3,5,6})for(int k=0;k<2;k++)foreach(int size in new[]{0,1,3,17})foreach(int index in new[]{int.MinValue,-3,-1,0,1,3,17,18,int.MaxValue}){if(op==2 || op==5)Emit(output,op,k,size,9,4,0,index,0,0,1);else foreach(int count in new[]{int.MinValue,-3,-1,0,1,3,17,int.MaxValue})Emit(output,op,k,size,9,4,0,index,count,0,1);}
 }}
}
