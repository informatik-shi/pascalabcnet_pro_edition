// Independent development-only Framework oracle; never installed.
using System;using System.Collections.Generic;using System.Globalization;using System.IO;using System.Numerics;using System.Runtime.Versioning;using System.Text;
[assembly:TargetFramework(".NETFramework,Version=v4.7.2")]
class EqualityReference {
 enum Number{Zero,One,Two}
 static string Label(Type t){string n=t.Name;return n.StartsWith("ByteEqualityComparer")?"B":n.StartsWith("EnumEqualityComparer")?"E":n.StartsWith("GenericEqualityComparer")?"G":"O";}
 static void Values<T>(StreamWriter w,string kind,T[] values,Func<T,string> encode){var c=EqualityComparer<T>.Default;foreach(var a in values)foreach(var b in values)w.WriteLine(kind+"\t"+encode(a)+"\t"+encode(b)+"\t"+c.Equals(a,b)+"\t"+c.GetHashCode(a)+"\t"+c.GetHashCode(b)+"\t"+Label(c.GetType()));}
 static string Text(string s){if(s==null)return "nil";if(s.Length==0)return "empty";var a=new string[s.Length];for(int i=0;i<s.Length;i++)a[i]=((int)s[i]).ToString("X4");return String.Join(",",a);}
 static int mode,calls;static StringBuilder log;
 class C:IEquatable<C>{public int n;public C(int v){n=v;}public bool Equals(C other){Touch("E("+Tag(this)+","+Tag(other)+");",this);return mode!=1 && other!=null && n==other.n;}}
 class N{public int n;public N(int v){n=v;}}
 class W:IEquatable<C>{public int n;public W(int v){n=v;}public bool Equals(C other){Touch("W;",null);return true;}}
 struct R{public int n;public C c;public R(int v,C x){n=v;c=x;}}
 interface INamed:IEqualityComparer<C>{}
 class Order:IEqualityComparer<C>{public bool Equals(C a,C b){Touch("Q("+Tag(a)+","+Tag(b)+");",a);return mode!=1 && (a==null?b==null:b!=null && a.n==b.n);}public int GetHashCode(C a){Touch("H("+Tag(a)+");",a);return a==null?-77:a.n;}}
 class NamedOrder:INamed{public bool Equals(C a,C b){Touch("N("+Tag(a)+","+Tag(b)+");",a);return mode!=1 && (a==null?b==null:b!=null && a.n==b.n);}public int GetHashCode(C a){Touch("J("+Tag(a)+");",a);return a==null?-77:a.n;}}
 static string Tag(C a){return a==null?"nil":a.n.ToString();}
 static void Touch(string text,C a){log.Append(text);calls++;if(mode==2 && calls==3)throw new DivideByZeroException();if(mode==4 && calls==1 && a!=null)a.n++;}
 static void Call<T>(IEqualityComparer<T> c,T a,T b,Func<T,string> tag){var x=c.Equals(a,b);var h=c.GetHashCode(a);var h2=c.GetHashCode(a);var y=c.Equals(b,a);var z=c.Equals(a,a);log.Append("R").Append(x).Append(",").Append(y).Append(",").Append(z).Append(";H").Append(h==h2).Append(";Z").Append(tag(a)=="nil"?h==0:true).Append(";").Append(tag(a)).Append(",").Append(tag(b)).Append(";").Append(Label(c.GetType()));}
 static void ClassRow(StreamWriter w,int scenario,int ai,int bi,int selectedMode){mode=selectedMode;calls=0;log=new StringBuilder();var c0=new C(0);var cp=new[]{null,c0,new C(1),c0,new C(0)};var a=cp[ai];var b=cp[bi];string code="ok";
 try{switch(scenario){
  case 0:case 1:case 2:IEqualityComparer<C> cmp=scenario==0?EqualityComparer<C>.Default:scenario==1?(IEqualityComparer<C>)new Order():new NamedOrder();if(mode==3)cmp=null;Call(cmp,a,b,Tag);break;
  case 3:var n0=new N(0);var ns=new[]{null,n0,new N(1),n0,new N(0)};Call(mode==3?null:EqualityComparer<N>.Default,ns[ai],ns[bi],x=>x==null?"nil":x.n.ToString());break;
  case 4:var w0=new W(0);var ws=new[]{null,w0,new W(1),w0,new W(0)};Call(mode==3?null:EqualityComparer<W>.Default,ws[ai],ws[bi],x=>x==null?"nil":x.n.ToString());break;
  case 5:Call(mode==3?null:EqualityComparer<IEquatable<C>>.Default,(IEquatable<C>)a,(IEquatable<C>)b,x=>Tag((C)x));break;
  case 6:IEquatable<C> receiver=mode==3?null:a;var x0=receiver.Equals(b);var h=EqualityComparer<C>.Default.GetHashCode(a);var h2=EqualityComparer<C>.Default.GetHashCode(a);var y0=receiver.Equals(a);var z0=receiver.Equals(b);log.Append("R").Append(x0).Append(",").Append(y0).Append(",").Append(z0).Append(";H").Append(h==h2).Append(";Z").Append(a==null?h==0:true).Append(";").Append(Tag(a)).Append(",").Append(Tag(b)).Append(";D");break;
  case 7:var ar0=new[]{0};var arrays=new[]{null,ar0,new[]{1},ar0,new[]{0}};Call(mode==3?null:EqualityComparer<int[]>.Default,arrays[ai],arrays[bi],v=>v==null?"nil":v[0].ToString());break;
  case 8:var ra=new R(ai%2,a);var rb=new R(bi%2,b);Call(mode==3?null:EqualityComparer<R>.Default,ra,rb,v=>v.n+":"+Tag(v.c));break;
 }}catch(DivideByZeroException){code="E_DIV_ZERO";}catch(NullReferenceException){code="E_NIL";}
 w.WriteLine(scenario+"\t"+ai+"\t"+bi+"\t"+mode+"\t"+log+"\t"+code);
 }
 public static void Main(string[] args){System.Threading.Thread.CurrentThread.CurrentCulture=CultureInfo.GetCultureInfo("en-US");System.Threading.Thread.CurrentThread.CurrentUICulture=CultureInfo.GetCultureInfo("en-US");
 using(var w=new StreamWriter(args[0],false,new UTF8Encoding(false))){w.WriteLine("# Framework 4.7.2 typed equality/hash/default implementation");
 Values(w,"integer",new[]{int.MinValue,-65536,-1,0,1,65535,int.MaxValue},x=>x.ToString());
 Values(w,"int64",new[]{long.MinValue,-4294967296L,-1L,0L,1L,4294967296L,long.MaxValue},x=>x.ToString());
 Values(w,"longword",new uint[]{0,1,65535,2147483648U,uint.MaxValue},x=>x.ToString());
 Values(w,"uint64",new ulong[]{0,1,4294967296UL,9223372036854775808UL,ulong.MaxValue},x=>x.ToString());
 Values(w,"word",new ushort[]{0,1,255,32768,65535},x=>x.ToString());Values(w,"smallint",new short[]{-32768,-1,0,1,5,32767},x=>x.ToString());Values(w,"byte",new byte[]{0,1,127,128,255},x=>x.ToString());Values(w,"shortint",new sbyte[]{-128,-1,0,1,127},x=>x.ToString());
 Values(w,"char",new char[]{'\0','A','я','\ud800','\udfff','\uffff'},x=>((int)x).ToString());Values(w,"boolean",new[]{false,true},x=>x?"1":"0");
 var f=new List<float>();foreach(uint n in new uint[]{0,0x80000000,1,0x3f800000,0xbf800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc00001,0xffc00002,0x7f800001})f.Add(BitConverter.ToSingle(BitConverter.GetBytes(n),0));Values(w,"single",f.ToArray(),x=>BitConverter.ToUInt32(BitConverter.GetBytes(x),0).ToString("X8"));
 var d=new List<double>();foreach(ulong n in new ulong[]{0,0x8000000000000000,1,0x3ff0000000000000,0xbff0000000000000,0x7fefffffffffffff,0x7ff0000000000000,0xfff0000000000000,0x7ff8000000000001,0xfff8000000000002,0x7ff0000000000001})d.Add(BitConverter.Int64BitsToDouble(unchecked((long)n)));Values(w,"real",d.ToArray(),x=>unchecked((ulong)BitConverter.DoubleToInt64Bits(x)).ToString("X16"));
 Values(w,"string",new[]{null,"","a","A","aa","Aa","abc","Я","я","🐱","Aa\0B","Aa\0C",new String(new[]{'\ud800'}),new String(new[]{'\udfff'})},Text);
 var b=new List<BigInteger>();foreach(string s in new[]{"-18446744073709551616","-4294967296","-2147483649","-2147483648","-2147483647","-1","0","1","2147483647","2147483648","4294967296","18446744073709551616","340282366920938463463374607431768211457"})b.Add(BigInteger.Parse(s));Values(w,"BigInteger",b.ToArray(),x=>x.ToString());
 Values(w,"enum",new[]{(Number)int.MinValue,(Number)(-1),Number.Zero,Number.One,Number.Two,(Number)int.MaxValue},x=>((int)x).ToString());
 }
 using(var w=new StreamWriter(Path.Combine(Path.GetDirectoryName(args[0]),"equality-classes-reference.tsv"),false,new UTF8Encoding(false))){w.WriteLine("# Framework typed IEquatable/IEqualityComparer exact call traces");for(int scenario=0;scenario<9;scenario++)for(int ai=0;ai<5;ai++)for(int bi=0;bi<5;bi++)for(int mode=0;mode<5;mode++)ClassRow(w,scenario,ai,bi,mode);}
 }
}
