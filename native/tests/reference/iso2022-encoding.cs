// Development-only actual public-API oracle. Does not invoke Pascal.
using System;using System.IO;using System.Text;using System.Reflection;using System.Collections.Generic;
class IsoReference{
 static void Need(bool ok,string message){if(!ok)throw new Exception(message);}
 static byte[] Cat(byte[] a,byte[] b){var r=new byte[a.Length+b.Length];Array.Copy(a,r,a.Length);Array.Copy(b,0,r,a.Length,b.Length);return r;}
 static bool Equal(byte[] a,byte[] b){if(a.Length!=b.Length)return false;for(int n=0;n<a.Length;++n)if(a[n]!=b[n])return false;return true;}
 static byte[] Slice(byte[] a,int n){var b=new byte[n];Array.Copy(a,b,n);return b;}
 static void Bytes(BinaryWriter w,byte[] data){Need(data.Length<128,"Byte result width");w.Write((byte)data.Length);w.Write(data);}
 static void Text(BinaryWriter w,string s,bool exact){Need(s.Length<128,"Text result width");w.Write((byte)(s.Length|(exact?128:0)));foreach(char c in s)w.Write((ushort)c);}
 class ByteRuns{
  List<int> counts=new List<int>();List<byte[]> data=new List<byte[]>();List<bool> strict=new List<bool>();
  public void Add(byte[] value,bool exact){int n=data.Count-1;if(n>=0&&strict[n]==exact&&Equal(value,data[n])){counts[n]++;return;}counts.Add(1);data.Add(value);strict.Add(exact);}
  public void Write(BinaryWriter w){w.Write(data.Count);for(int n=0;n<data.Count;++n){w.Write(counts[n]);w.Write(strict[n]);Bytes(w,data[n]);}}
 }
 static byte[][] Modes(int cp){if(cp==52936)return new[]{new byte[0],new byte[]{0x7e,0x7b}};if(cp==50225)return new[]{new byte[0],new byte[]{0x0e},new byte[]{0x1b,0x24,0x29,0x43},new byte[]{0x1b,0x24,0x29,0x43,0x0e}};return new[]{new byte[0],new byte[]{0x1b,0x28,0x49},new byte[]{0x1b,0x24,0x42},new byte[]{0x0e},new byte[]{0x1b,0x24,0x42,0x0e},new byte[]{0x1b,0x26,0x40},new byte[]{0x1b,0x24,0x28,0x44}};}
 static byte[][] Pending(int cp){if(cp==52936)return new[]{new byte[0],new byte[]{0x7e}};if(cp==50225)return new[]{new byte[0],new byte[]{0x1b},new byte[]{0x1b,0x24},new byte[]{0x1b,0x24,0x29},new byte[]{0x1b,0x1b}};return new[]{new byte[0],new byte[]{0x1b},new byte[]{0x1b,0x24},new byte[]{0x1b,0x28},new byte[]{0x1b,0x26},new byte[]{0x1b,0x24,0x28},new byte[]{0x1b,0x1b}};}
 static void Main(string[] args){
  if(args.Length>1){var t=Type.GetType("System.Text.CodePagesEncodingProvider, System.Text.Encoding.CodePages");typeof(Encoding).GetMethod("RegisterProvider").Invoke(null,new[]{t.GetProperty("Instance").GetValue(null,null)});}
  long decoderInputs=0,encoderInputs=0;var units=new char[1];var bytes=new byte[64];var chars=new char[32];
  using(var w=new BinaryWriter(File.Create(args[0]))){w.Write(Encoding.ASCII.GetBytes("ABCISO01"));w.Write(5);
   foreach(int cp in new[]{50220,50221,50222,50225,52936}){var e=Encoding.GetEncoding(cp);var strict=(Encoding)e.Clone();strict.EncoderFallback=EncoderFallback.ExceptionFallback;strict.DecoderFallback=DecoderFallback.ExceptionFallback;
    w.Write(cp);w.Write(e.GetType().FullName);var exacts=new bool[65536];var encoded=new ByteRuns();
    for(int n=0;n<65536;++n){units[0]=(char)n;var value=e.GetBytes(units);bool exact=true;try{Need(Equal(strict.GetBytes(units),value),"Strict BMP mapping");}catch(EncoderFallbackException){exact=false;}exacts[n]=exact;encoded.Add(value,exact);}encoded.Write(w);
    // Raw single/halfwidth/DBCS lookup probes have modes selected in a previous call.
    var maps=cp==52936?new[]{new byte[0],new byte[]{0x7e,0x7b}}:cp==50225?new[]{new byte[0],new byte[]{0x0e}}:new[]{new byte[0],new byte[]{0x1b,0x28,0x49},new byte[]{0x1b,0x24,0x42}};
    w.Write(maps.Length);foreach(var prefix in maps){Bytes(w,prefix);var decoder=e.GetDecoder();var strictDecoder=strict.GetDecoder();
     for(int n=0;n<65536;++n){var raw=new[]{(byte)(n>>8),(byte)n};decoder.Reset();Need(decoder.GetChars(prefix,0,prefix.Length,chars,0,false)==0,"Lookup prefix emits no character");int count=decoder.GetChars(raw,0,2,chars,0,true);string value=new string(chars,0,count);bool exact=true;
      try{strictDecoder.Reset();strictDecoder.GetChars(prefix,0,prefix.Length,chars,0,false);count=strictDecoder.GetChars(raw,0,2,chars,0,true);Need(new string(chars,0,count)==value,"Strict lookup mapping");}catch(DecoderFallbackException){exact=false;}Text(w,value,exact);
     }
    }
    var contexts=new List<string>{"","\ud83d"};if(cp<50225)contexts.AddRange(new[]{"日","ｶ","日ｶ","ｶ日","日A"});else if(cp==50225)contexts.AddRange(new[]{"한","한A","한\n"});else contexts.AddRange(new[]{"中","中A","~"});
    w.Write(contexts.Count*2-1);foreach(string prefix in contexts)for(int flush=0;flush<=1;++flush){if(flush==1&&prefix=="\ud83d")continue;w.Write((byte)prefix.Length);foreach(char c in prefix)w.Write((ushort)c);w.Write(flush!=0);var p=prefix.ToCharArray();var encoder=e.GetEncoder();Bytes(w,Slice(bytes,encoder.GetBytes(p,0,p.Length,bytes,0,flush!=0)));var runs=new ByteRuns();
     for(int n=0;n<65536;++n){units[0]=(char)n;encoder.Reset();encoder.GetBytes(p,0,p.Length,bytes,0,flush!=0);int count=encoder.GetBytes(units,0,1,bytes,0,true);var result=Slice(bytes,count);bool exact=prefix!="\ud83d"&&exacts[n];
      if(exacts[n]||n>=0xd800&&n<=0xdfff||n%257==0){bool ok=true;try{var s=strict.GetEncoder();s.GetBytes(p,0,p.Length,bytes,0,flush!=0);count=s.GetBytes(units,0,1,bytes,0,true);Need(Equal(Slice(bytes,count),result),"Strict encoder context bytes");}catch(EncoderFallbackException){ok=false;}Need(ok==exact,"Strict encoder context membership");}runs.Add(result,exact);++encoderInputs;
     }runs.Write(w);
    }
    var prefixes=new List<byte[]>();foreach(var mode in Modes(cp))foreach(var pending in Pending(cp))prefixes.Add(Cat(mode,pending));w.Write(prefixes.Count);
    foreach(var prefix in prefixes){Bytes(w,prefix);var decoder=e.GetDecoder();var strictDecoder=strict.GetDecoder();decoder.Reset();int prefixCount=decoder.GetChars(prefix,0,prefix.Length,chars,0,false);string prefixValue=new string(chars,0,prefixCount);Text(w,prefixValue,true);
     for(int n=0;n<65536;++n){var raw=new[]{(byte)(n>>8),(byte)n};decoder.Reset();int first=decoder.GetChars(prefix,0,prefix.Length,chars,0,false);int count=decoder.GetChars(raw,0,2,chars,first,true);string value=new string(chars,0,first+count);bool exact=true;
      try{strictDecoder.Reset();first=strictDecoder.GetChars(prefix,0,prefix.Length,chars,0,false);count=strictDecoder.GetChars(raw,0,2,chars,first,true);Need(new string(chars,0,first+count)==value,"Strict context decode bytes");}catch(DecoderFallbackException){exact=false;}Text(w,value,exact);
      // All two-byte suffixes in each state, split at every boundary.
      for(int cut=1;cut<=2;++cut){decoder.Reset();first=decoder.GetChars(prefix,0,prefix.Length,chars,0,false);first+=decoder.GetChars(raw,0,cut,chars,first,false);count=decoder.GetChars(raw,cut,2-cut,chars,first,true);Need(new string(chars,0,first+count)==value,"Stateful suffix split invariance");}
      ++decoderInputs;
     }
    }
    Console.WriteLine(cp+" ISO-2022/HZ BMP maps, all encoder suffix contexts and all decoder suffix states verified");
   }
  }Console.WriteLine(encoderInputs+" contextual BMP inputs; "+decoderInputs+" contextual byte-pair inputs");
 }
}
