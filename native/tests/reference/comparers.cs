// Independent development oracle. No managed runtime is shipped.
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
public static class ComparerReference {
 static StringBuilder log;static int failMode;
 sealed class Item:IComparable<Item> {
  public int n;public Item(int value){n=value;}
  public int CompareTo(Item other){log.Append("C(").Append(n).Append(",").Append(other==null?"nil":other.n.ToString()).Append(");");if(n==1 && failMode==1)throw new DivideByZeroException();return other==null?7:failMode==2?other.n-n:n-other.n;}
 }
 sealed class ItemComparer:IComparer<Item>{public int Compare(Item a,Item b){log.Append("U;");return a==null?(b==null?0:-1):b==null?1:a.CompareTo(b);}}
 static Item Make(int n){return n==-99?null:new Item(n);}
 static int CompareItems(Item a,Item b){log.Append("F;");return a==null?(b==null?0:-8):b==null?8:a.CompareTo(b);}
 static bool InRange<T>(T x,T low,T high) where T:IComparable<T>{return x.CompareTo(low)>=0 && x.CompareTo(high)<=0;}
 static bool Between<T>(T x,T low,T high,bool inclusive) where T:IComparable<T>{int a=x.CompareTo(low),b=x.CompareTo(high);bool reversed=low.CompareTo(high)>0;int first=reversed?b:a,last=reversed?a:b;return inclusive?first>=0 && last<=0:first>0 && last<0;}
 static T Clamp<T>(T x,T low,T high) where T:IComparable<T>{if(low.CompareTo(high)>0)throw new ArgumentException();if(x.CompareTo(low)<0)return low;if(x.CompareTo(high)>0)return high;return x;}
 static void Scalar<T>(StreamWriter output,string type,T[] values,Func<T,string> literal) where T:IComparable<T>{foreach(var a in values)foreach(var b in values){var d=a.CompareTo(b);var c=Comparer<T>.Default.Compare(a,b);if(d!=c)throw new Exception("Primitive comparer differs");output.WriteLine(type+"\t"+literal(a)+"\t"+literal(b)+"\t"+c);}}
 public static void Scalars(string file){using(var output=new StreamWriter(file,false,new UTF8Encoding(false))){
  output.WriteLine("# Independent Framework primitive CompareTo/Comparer<T>.Default");
  Scalar(output,"char",new char[]{(char)0,'a','z',(char)65535},x=>((int)x).ToString());
  Scalar(output,"byte",new byte[]{0,1,127,255},x=>x.ToString());
  Scalar(output,"shortint",new sbyte[]{-128,-1,0,127},x=>x.ToString());
  Scalar(output,"smallint",new short[]{-32768,-1,0,32767},x=>x.ToString());
  Scalar(output,"word",new ushort[]{0,1,32767,65535},x=>x.ToString());
  Scalar(output,"integer",new int[]{int.MinValue,-1,0,int.MaxValue},x=>x.ToString());
  Scalar(output,"longword",new uint[]{0,1,2147483647,uint.MaxValue},x=>x.ToString());
  Scalar(output,"int64",new long[]{long.MinValue,-1,0,long.MaxValue},x=>x.ToString());
  Scalar(output,"uint64",new ulong[]{0,1,9223372036854775807,ulong.MaxValue},x=>x.ToString());
  Scalar(output,"boolean",new bool[]{false,true},x=>x?"True":"False");
 }}
 public static void Generate(string file){
  using(var output=new StreamWriter(file,false,new UTF8Encoding(false))){output.WriteLine("# scenario a b c mode trace error; Framework "+Environment.Version);
   foreach(int scenario in new[]{0,1,2,3,4,5,6,7,8,9,10,11})foreach(int a in new[]{-99,-2,1,3})foreach(int b in new[]{-99,-2,1,3})foreach(int c in new[]{-99,-2,1,3})foreach(int mode in new[]{0,1,2}){
    failMode=mode;log=new StringBuilder();string code="ok";var x=Make(a);var y=Make(b);var z=Make(c);
    try{object value=null;switch(scenario){
     case 0:value=Comparer<Item>.Default.Compare(x,y);break;
     case 1:value=Comparer<Item>.Create(CompareItems).Compare(x,y);break;
     case 2:value=((IComparer<Item>)new ItemComparer()).Compare(x,y);break;
     case 3:value=InRange(x,y,z);break;case 4:value=Between(x,y,z,true);break;case 5:value=Between(x,y,z,false);break;
     case 6:var q=Clamp(x,y,z);value=q==null?"nil":q.n.ToString();break;
     case 7:value=x.CompareTo(y);break;case 8:value=((IComparable<Item>)x).CompareTo(y);break;
     case 9:value=((IComparer<Item>)Comparer<Item>.Default).Compare(x,x);break;
     case 10:value=Comparer<Item>.Create(CompareItems).Compare(x,x);break;
     case 11:value=Comparer<Item>.Default.Compare(x,x);break;
    }log.Append(value);}
    catch(NullReferenceException){code="E_NIL";}catch(DivideByZeroException){code="E_DIV_ZERO";}catch(ArgumentException){code="E_RANGE";}
    output.WriteLine(scenario+"\t"+a+"\t"+b+"\t"+c+"\t"+mode+"\t"+log+"\t"+code);
   }
  }
 }
}
