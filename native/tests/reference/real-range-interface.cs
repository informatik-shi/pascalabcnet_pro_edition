// Independent source RealRange recipe and genuine Framework interfaces; development only.
using System;using System.IO;using System.Text;using System.Collections.Generic;
struct R:IEquatable<R>{
 public readonly double Low,High;public R(double l,double h){Low=l;High=h;}
 public static bool operator==(R a,R b){return a.Low==b.Low&&a.High==b.High;}public static bool operator!=(R a,R b){return !(a==b);}
 public bool Equals(R other){return this==other;}public override bool Equals(object other){return other is R&&this==(R)other;}
 public override int GetHashCode(){return unchecked(Low.GetHashCode()*668265263^High.GetHashCode());}
}
class Reference{
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("X4"));return b.ToString();}
 static string Bits(double n){return unchecked((ulong)BitConverter.DoubleToInt64Bits(n)).ToString("X16");}
 static void Run(R ra,R rb,int nil){
  IEquatable<R> a=ra,b=rb;if((nil&1)!=0)a=null;if((nil&2)!=0)b=null;var same=a;
  Console.WriteLine(ra==rb);Console.WriteLine(EqualityComparer<R>.Default.Equals(ra,rb));
  Console.WriteLine(object.ReferenceEquals(a,same)+" "+object.ReferenceEquals(a,b));
  var comparer=EqualityComparer<IEquatable<R>>.Default;Console.WriteLine(comparer.Equals(a,b)+" "+(comparer.GetHashCode(a)==comparer.GetHashCode(b)));Console.WriteLine(comparer.Equals(a,a));Console.WriteLine(Tuple.Create(ra,1).Equals(Tuple.Create(rb,1)));
  Console.WriteLine(new List<R>{ra}.Contains(rb)+" "+new HashSet<R>{ra,rb}.Count);
  Console.WriteLine(new List<IEquatable<R>>{a}.Contains(b)+" "+new HashSet<IEquatable<R>>{a,b}.Count);
  Console.WriteLine(a.Equals(rb));Console.WriteLine(((R)(object)a).Equals(ra));
 }
 static void Main(string[] args){
  var ranges=new[]{new R(0,1),new R(1,0),new R(-0.0,0.0),new R(0.0,-0.0),new R(-1,1),new R(1,1),new R(double.Epsilon,1),new R(double.MaxValue,double.MaxValue),new R(double.NaN,1),new R(1,double.NaN),new R(double.NaN,double.NaN),new R(double.PositiveInfinity,double.PositiveInfinity),new R(double.NegativeInfinity,double.PositiveInfinity),new R(BitConverter.Int64BitsToDouble(0x7ff8000000000123L),1),new R(BitConverter.Int64BitsToDouble(0x7ff8000000000000L),1)};
  var original=Console.Out;int rows=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# a b nil low_a_bits high_a_bits low_b_bits high_b_bits output_utf16 error; Framework "+Environment.Version);
   for(int ai=0;ai<ranges.Length;ai++)for(int bi=0;bi<ranges.Length;bi++)for(int nil=0;nil<4;nil++){
    var output=new StringWriter();Console.SetOut(output);string error="ok";try{Run(ranges[ai],ranges[bi],nil);}catch(NullReferenceException){error="E_NIL";}finally{Console.SetOut(original);}
    file.WriteLine(ai+"\t"+bi+"\t"+nil+"\t"+Bits(ranges[ai].Low)+"\t"+Bits(ranges[ai].High)+"\t"+Bits(ranges[bi].Low)+"\t"+Bits(ranges[bi].High)+"\t"+Hex(output.ToString().Replace("\r\n","\n"))+"\t"+error);rows++;
   }
  }Console.WriteLine(rows+" independent RealRange interface outcomes");
 }
}
