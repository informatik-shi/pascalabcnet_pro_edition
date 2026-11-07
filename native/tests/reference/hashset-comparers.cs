// Independent development oracle; never installed in the native distribution.
using System;using System.Collections;using System.Collections.Generic;using System.IO;using System.Text;
public static class HashSetComparersReference {
 static HashSet<int> set,alias,result,otherSet;static IEnumerator<int> oldCursor;static IEqualityComparer<int> policy,alternate;static IEnumerable<int> other;static StringBuilder log;static int mutation,trigger,phase,hashCalls,equalsCalls;static bool active,busy;
 sealed class Order:IEqualityComparer<int>{public string label;public int modulus;public Order(string l,int m){label=l;modulus=m;}public bool Equals(int a,int b){bool changed=Touch(label+"E("+a+","+b+");",false);return changed && (mutation==3 || mutation==9) || a%modulus==b%modulus;}public int GetHashCode(int a){Touch(label+"H("+a+");",true);return -2147483647;}}
 sealed class Guard:IEnumerable<int>,IEnumerator<int>{int n;public IEnumerator<int> GetEnumerator(){return this;}IEnumerator IEnumerable.GetEnumerator(){return this;}public bool MoveNext(){return ++n==1;}public int Current{get{return 0;}}object IEnumerator.Current{get{return 0;}}public void Reset(){n=0;}public void Dispose(){Finish();}}
 static bool Touch(string text,bool hashing){if(!active || busy)return false;log.Append(text);if(hashing)hashCalls++;else equalsCalls++;if(mutation==0 || (hashing?0:1)!=phase || (hashing?hashCalls:equalsCalls)!=trigger)return false;busy=true;log.Append("M").Append(mutation).Append(';');
 if(mutation==1)throw new DivideByZeroException();if(mutation==2)throw new IndexOutOfRangeException();if(mutation==3)set.Clear();if(mutation==4)for(int i=20;i<40;i++)set.Add(i);if(mutation==5)set.TrimExcess();if(mutation==6)set.Remove(2);if(mutation==7)set=new HashSet<int>(set,policy);if(mutation==8)set=null;if(mutation==9){set.Clear();set.Add(99);}if(mutation==10)set.Remove(4);busy=false;return true;}
 static string Dump(HashSet<int> s){if(s==null)return "nil";var b=new StringBuilder("[");bool first=true;foreach(int x in s){if(!first)b.Append(',');first=false;b.Append(x);}return b.Append(']').ToString();}
 static string Label(HashSet<int> s){return s==null?"nil":Object.ReferenceEquals(s.Comparer,policy)?"P":Object.ReferenceEquals(s.Comparer,alternate)?"A":Object.ReferenceEquals(s.Comparer,EqualityComparer<int>.Default)?"D":"?";}
 static void Finish(){active=false;log.Append("Z").Append(Dump(set)).Append(':').Append(set==null?-1:set.Count).Append(':').Append(Label(set)).Append('|').Append(Dump(alias)).Append(':').Append(alias.Count).Append('|').Append(Dump(result)).Append(':').Append(Label(result)).Append("|E");bool n=oldCursor.MoveNext();log.Append(n);if(n)log.Append(':').Append(oldCursor.Current);log.Append(';');}
 static HashSet<int> Setup(int shape,IEqualityComparer<int> cmp){var s=shape==1?new HashSet<int>(new int[0],cmp):new HashSet<int>(cmp);if(shape==2){s.Add(9);s.Clear();}if(shape==3)foreach(int n in new[]{0,1,2,4,6,8})s.Add(n);if(shape==4){for(int i=0;i<80;i++)s.Add(i);for(int i=0;i<75;i++)s.Remove(i);}if(shape==5){for(int i=0;i<17;i++)s.Add(i);foreach(int i in new[]{1,3,5,7})s.Remove(i);}return s;}
 static bool Predicate(int n){Touch("P("+n+");",false);return n%2==0;}
 static HashSet<int> DefaultCopy(){return new HashSet<int>(set);}
 static void Run(int op){int value;switch(op){
 case 0:log.Append(set.Contains(4));break;case 1:log.Append(set.Add(4));break;case 2:log.Append(set.Remove(4));break;case 3:bool found=set.TryGetValue(4,out value);log.Append(found).Append(':').Append(value);break;
 case 4:set.Clear();break;case 5:set.TrimExcess();break;case 6:result=new HashSet<int>(set);break;case 7:result=new HashSet<int>(set,policy);break;case 8:result=new HashSet<int>(set,alternate);break;
 case 9:set.UnionWith(other);break;case 10:set.ExceptWith(other);break;case 11:set.IntersectWith(other);break;case 12:set.SymmetricExceptWith(other);break;case 13:log.Append(set.SetEquals(other));break;case 14:log.Append(set.IsSubsetOf(other));break;case 15:log.Append(set.IsProperSubsetOf(other));break;case 16:log.Append(set.IsSupersetOf(other));break;case 17:log.Append(set.IsProperSupersetOf(other));break;case 18:log.Append(set.Overlaps(other));break;
 case 19:log.Append(set.RemoveWhere(Predicate));break;case 20:result=new HashSet<int>(set,set.Comparer);break;
 case 21:result=DefaultCopy();result.UnionWith(otherSet);break;case 22:result=DefaultCopy();result.ExceptWith(otherSet);break;case 23:result=DefaultCopy();result.IntersectWith(otherSet);break;
 case 24:result=new HashSet<int>(other,policy);break;case 25:log.Append(set.Contains(44));break;
 case 26:var a=new int[alias.Count+2];set.CopyTo(a,1);log.Append('[').Append(string.Join(",",a)).Append(']');break;
 case 27:set.Remove(4);set.Add(44);set.Add(4);break;
 }log.Append("DONE;");}
 static void Emit(StreamWriter output,int op,int shape,int p,int kind,int m,int ph,int nth){active=false;busy=false;hashCalls=equalsCalls=0;mutation=m;phase=ph;trigger=nth;policy=p==0?EqualityComparer<int>.Default:(IEqualityComparer<int>)new Order("U",p==1?3:p==2?1000:2);alternate=new Order("V",2);set=Setup(shape,policy);alias=set;result=null;
 var data=new[]{1,4,4,7,2,8};otherSet=kind==1?new HashSet<int>(data,policy):kind==2?new HashSet<int>(data,p==0?EqualityComparer<int>.Default:(IEqualityComparer<int>)new Order("U",p==1?3:p==2?1000:2)):new HashSet<int>(data,alternate);if(kind==4)otherSet=set;if(kind==7){otherSet=new HashSet<int>(policy);for(int i=0;i<21;i++)otherSet.Add(i);}
 other=kind==0?data:kind==5?null:kind==6?new int[0]:(IEnumerable<int>)otherSet;oldCursor=set.GetEnumerator();log=new StringBuilder();active=true;string code="ok";
 try{foreach(var ignored in new Guard())Run(op);}catch(DivideByZeroException){code="E_DIV_ZERO";}catch(IndexOutOfRangeException){code="E_BOUNDS";}catch(NullReferenceException){code="E_NIL";}catch(ArgumentNullException){code="E_NIL";}catch(InvalidOperationException){code="E_MODIFIED";}
 output.WriteLine(op+"	"+shape+"	"+p+"	"+kind+"	"+m+"	"+ph+"	"+nth+"	"+log+"	"+code);
 }
 public static void Main(string[] args){using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){output.AutoFlush=true;output.WriteLine("# Independent Framework CLR "+Environment.Version+" HashSet<int>: collision chains, policy constructors, algebra, live callback mutations, fault state, cursor versions");
 for(int op=0;op<28;op++)for(int shape=0;shape<6;shape++)for(int p=0;p<4;p++)for(int kind=0;kind<8;kind++)Emit(output,op,shape,p,kind,0,0,1);
 foreach(int op in new[]{0,1,2,3,7,8,9,10,11,12,13,14,15,16,17,18,19,20,24,25,27})foreach(int shape in new[]{3,5})foreach(int kind in new[]{1,7})for(int m=1;m<=10;m++)for(int ph=0;ph<2;ph++)foreach(int nth in new[]{1,3}){if(ph==1 && (m==3 || m==5 || m==6 || m==9 || m==10) && op!=0 && op!=3)continue;Emit(output,op,shape,2,kind,m,ph,nth);}
 }}
}
