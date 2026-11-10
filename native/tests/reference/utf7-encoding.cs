// Development-only public API oracle. No Pascal compiler or private runtime state.
using System;using System.IO;using System.Text;using System.Collections.Generic;
class Utf7Reference{
 static void Need(bool ok,string s){if(!ok)throw new Exception(s);}
 static bool Equal(byte[] a,byte[] b){if(a.Length!=b.Length)return false;for(int n=0;n<a.Length;++n)if(a[n]!=b[n])return false;return true;}
 static byte[] Take(byte[] a,int n){var r=new byte[n];Array.Copy(a,r,n);return r;}
 static void Bytes(BinaryWriter w,byte[] b){Need(b.Length<128,"Byte width");w.Write((byte)b.Length);w.Write(b);}
 static void Text(BinaryWriter w,string s,bool exact){Need(s.Length<128,"Text width");w.Write((byte)(s.Length|(exact?128:0)));foreach(char c in s)w.Write((ushort)c);}
 static List<string> EncoderPrefixes(){var r=new List<string>{""};for(int n=0;n<16;++n)r.Add(new string((char)(256+n),1));for(int n=0;n<4;++n)r.Add("\u0100"+(char)(256+n));r.Add("\u0100\u0100\u0100");return r;}
 static List<byte[]> DecoderPrefixes(){const string digits="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";var r=new List<byte[]>{new byte[0],new byte[]{43}};foreach(int count in new[]{0,2,4,6,8,10,12,14}){int length=count==0?8:1;while(length*6%16!=count)++length;for(int bits=0;bits<(1<<count);++bits){var p=new byte[length+1];p[0]=43;for(int n=0;n<length;++n){int shift=(length-n-1)*6;p[n+1]=(byte)digits[shift>=32?0:(bits>>shift)&63];}r.Add(p);}}return r;}
 static void Main(string[] args){var e=new UTF7Encoding(false);var strict=(Encoding)e.Clone();strict.EncoderFallback=EncoderFallback.ExceptionFallback;strict.DecoderFallback=DecoderFallback.ExceptionFallback;var bytes=new byte[64];var chars=new char[32];var unit=new char[1];long ec=0,dc=0;
 using(var w=new BinaryWriter(File.Create(args[0]))){w.Write(Encoding.ASCII.GetBytes("ABCUTF71"));w.Write(e.GetType().FullName);
  var contexts=EncoderPrefixes();w.Write(contexts.Count);foreach(string prefix in contexts){Text(w,prefix,true);var p=prefix.ToCharArray();var encoder=e.GetEncoder();Bytes(w,Take(bytes,encoder.GetBytes(p,0,p.Length,bytes,0,false)));
   for(int n=0;n<65536;++n){unit[0]=(char)n;encoder.Reset();encoder.GetBytes(p,0,p.Length,bytes,0,false);var expected=Take(bytes,encoder.GetBytes(unit,0,1,bytes,0,true));Bytes(w,expected);
    encoder.Reset();encoder.GetBytes(p,0,p.Length,bytes,0,false);int first=encoder.GetBytes(unit,0,1,bytes,0,false);int end=encoder.GetBytes(unit,1,0,bytes,first,true);Need(Equal(expected,Take(bytes,first+end)),"Encoder empty final split");
    var se=strict.GetEncoder();se.GetBytes(p,0,p.Length,bytes,0,false);Need(Equal(expected,Take(bytes,se.GetBytes(unit,0,1,bytes,0,true))),"Strict encoder preserves every UTF16 unit");++ec;
   }
  }Console.WriteLine(ec+" contextual BMP encodings, all residual encoder states");
  w.Write(1048576);for(int scalar=65536;scalar<=0x10ffff;++scalar){string s=char.ConvertFromUtf32(scalar);var input=s.ToCharArray();var expected=e.GetBytes(s);Bytes(w,expected);Need(Equal(expected,strict.GetBytes(s)),"Strict scalar");var enc=e.GetEncoder();int a=enc.GetBytes(input,0,1,bytes,0,false);int b=enc.GetBytes(input,1,1,bytes,a,true);Need(Equal(expected,Take(bytes,a+b)),"Raw surrogate stream");}
  var prefixes=DecoderPrefixes();w.Write(prefixes.Count);foreach(var prefix in prefixes){Bytes(w,prefix);var decoder=e.GetDecoder();int pc=decoder.GetChars(prefix,0,prefix.Length,chars,0,false);Text(w,new string(chars,0,pc),true);
   for(int n=0;n<256;++n){var raw=new[]{(byte)n};decoder.Reset();decoder.GetChars(prefix,0,prefix.Length,chars,0,false);int count=decoder.GetChars(raw,0,1,chars,0,true);string expected=new string(chars,0,count);bool exact=true;try{var sd=strict.GetDecoder();sd.GetChars(prefix,0,prefix.Length,chars,0,false);count=sd.GetChars(raw,0,1,chars,0,true);Need(expected==new string(chars,0,count),"Strict decoder output");}catch(DecoderFallbackException){exact=false;}Text(w,expected,exact);
    decoder.Reset();decoder.GetChars(prefix,0,prefix.Length,chars,0,false);int a=decoder.GetChars(raw,0,1,chars,0,false);int b=decoder.GetChars(raw,1,0,chars,a,true);Need(expected==new string(chars,0,a+b),"Decoder empty final split");Need(decoder.GetChars(new byte[]{65},0,1,chars,0,true)==1&&chars[0]=='A',"Decoder final clears state");++dc;
   }
  }Console.WriteLine(dc+" contextual decoder bytes, every residual value and phase");
  w.Write(65536);for(int n=0;n<65536;++n){var raw=new[]{(byte)(n>>8),(byte)n};string expected=e.GetString(raw);bool exact=true;try{Need(strict.GetString(raw)==expected,"Strict pair decode");}catch(DecoderFallbackException){exact=false;}Text(w,expected,exact);for(int cut=0;cut<=2;++cut){var d=e.GetDecoder();int a=d.GetChars(raw,0,cut,chars,0,false);int b=d.GetChars(raw,cut,2-cut,chars,a,true);Need(new string(chars,0,a+b)==expected,"Pair split decode");}}
 }Console.WriteLine("UTF7 reference complete");}
}