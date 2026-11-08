// Independent development-only IEEE search oracle; never installed.
using System;using System.Collections.Generic;using System.Linq;using System.IO;using System.Text;
[assembly:System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.7.2")]
public static class ArraySearchScalarsReference {
 static float F(uint bits){return BitConverter.ToSingle(BitConverter.GetBytes(bits),0);}static double D(ulong bits){return BitConverter.ToDouble(BitConverter.GetBytes(bits),0);}
 static string Hex(uint x){return x.ToString("x8");}static string Hex(ulong x){return x.ToString("x16");}
 public static void Main(string[] args){using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){output.WriteLine("# Independent Framework CLR "+Environment.Version+" generic Array search, legacy array Contains and explicit LINQ default comparer; exact IEEE inputs");var fb=new uint[]{0,0x80000000U,0x7fc00000U,0x7f800001U,0x7f800000U,0xff800000U,0x3f800000U,0xbf800000U};var db=new ulong[]{0,0x8000000000000000UL,0x7ff8000000000000UL,0x7ff0000000000001UL,0x7ff0000000000000UL,0xfff0000000000000UL,0x3ff0000000000000UL,0xbff0000000000000UL};
 for(int shape=0;shape<4;shape++){var f=shape==0?new uint[0]:(uint[])fb.Clone();var d=shape==0?new ulong[0]:(ulong[])db.Clone();if(shape==2){Array.Reverse(f);Array.Reverse(d);}if(shape==3){f=f.Concat(f).ToArray();d=d.Concat(d).ToArray();}var fa=Array.ConvertAll(f,F);var da=Array.ConvertAll(d,D);
 foreach(uint q in fb.Concat(new uint[]{0xffc12345U,0x3f000000U})){float value=F(q);output.WriteLine("single\t"+(f.Length==0?"-":string.Join(",",Array.ConvertAll(f,Hex)))+"\t"+Hex(q)+"\t"+Array.IndexOf(fa,value)+"\t"+Array.LastIndexOf(fa,value)+"\t"+((ICollection<float>)fa).Contains(value)+"\t"+Enumerable.Contains(fa,value,null));}
 foreach(ulong q in db.Concat(new ulong[]{0xfff8123456789abcUL,0x3fe0000000000000UL})){double value=D(q);output.WriteLine("real\t"+(d.Length==0?"-":string.Join(",",Array.ConvertAll(d,Hex)))+"\t"+Hex(q)+"\t"+Array.IndexOf(da,value)+"\t"+Array.LastIndexOf(da,value)+"\t"+((ICollection<double>)da).Contains(value)+"\t"+Enumerable.Contains(da,value,null));}
 }}}
}
