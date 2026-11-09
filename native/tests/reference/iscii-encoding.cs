// Development-only actual Framework/.NET reference. No Pascal compiler.
using System;using System.IO;using System.Text;using System.Reflection;using System.Collections.Generic;
class IsciiReference{
 static void Need(bool ok,string message){if(!ok)throw new Exception(message);}
 static void SetBit(byte[] data,int n){data[n>>3]|=(byte)(1<<(n&7));}
 static bool GetBit(byte[] data,int n){return (data[n>>3]&(1<<(n&7)))!=0;}
 static readonly byte[] ResetFont={0xef,0x40};
 static void Reset(Decoder decoder,char[] output){decoder.Reset();Need(decoder.GetChars(ResetFont,0,2,output,0,false)==0,"Decoder default-font selection");}
 static byte[] Encode(Encoding e,char[] input){return e.GetBytes(input);}
 static byte[] Slice(byte[] input,int n){var result=new byte[n];Array.Copy(input,result,n);return result;}
 static bool Equal(byte[] a,byte[] b){if(a.Length!=b.Length)return false;for(int n=0;n<a.Length;++n)if(a[n]!=b[n])return false;return true;}
 static void Bytes(BinaryWriter output,byte[] value){Need(value.Length<128,"Encoded result width");output.Write((byte)value.Length);output.Write(value);}
 static void Text(BinaryWriter output,char[] value,int count,bool exact){Need(count<128,"Decoded result width");output.Write((byte)(count|(exact?128:0)));for(int n=0;n<count;++n)output.Write((ushort)value[n]);}
 // Consecutive identical results are losslessly grouped, keeping exhaustive BMP contexts compact.
 class Runs{
  public List<int> count=new List<int>();public List<byte[]> data=new List<byte[]>();public List<bool> exact=new List<bool>();
  public void Add(byte[] value,bool strict){int n=data.Count-1;if(n>=0&&exact[n]==strict&&Equal(data[n],value)){count[n]++;return;}count.Add(1);data.Add(value);exact.Add(strict);}
  public void Write(BinaryWriter output){output.Write(data.Count);for(int n=0;n<data.Count;++n){output.Write(count[n]);output.Write(exact[n]);Bytes(output,data[n]);}}
 }
 class Codec{public Encoding normal,strict;public byte[] exact=new byte[8192];public string[] pairs=new string[65536];public bool[] pairExact=new bool[65536];}
 static void Main(string[] args){
  if(args.Length>1){var provider=Type.GetType("System.Text.CodePagesEncodingProvider, System.Text.Encoding.CodePages");typeof(Encoding).GetMethod("RegisterProvider").Invoke(null,new[]{provider.GetProperty("Instance").GetValue(null,null)});}
  var codecs=new Codec[10];var outputChars=new char[16];var strictChars=new char[16];var inputChars=new char[1];var bytes=new byte[32];
  using(var output=new BinaryWriter(File.Create(args[0]))){output.Write(Encoding.ASCII.GetBytes("ABCISC01"));output.Write(10);
   for(int cp=57002;cp<=57011;++cp){
    var c=new Codec();c.normal=Encoding.GetEncoding(cp);c.strict=(Encoding)c.normal.Clone();c.strict.EncoderFallback=EncoderFallback.ExceptionFallback;c.strict.DecoderFallback=DecoderFallback.ExceptionFallback;codecs[cp-57002]=c;
    output.Write(cp);output.Write(c.normal.GetType().FullName);var runs=new Runs();
    for(int n=0;n<65536;++n){inputChars[0]=(char)n;var value=Encode(c.normal,inputChars);bool exact=true;try{Need(Equal(Encode(c.strict,inputChars),value),"Strict BMP result");SetBit(c.exact,n);}catch(EncoderFallbackException){exact=false;}runs.Add(value,exact);}runs.Write(output);
    var singles=new ushort[256];var singleExact=new byte[32];var modes=new byte[256];var alternate=new ushort[256];
    for(int n=0;n<256;++n){var input=new[]{(byte)n};int count=c.normal.GetChars(input,0,1,outputChars,0);Need(count==1,"ISCII final single width");singles[n]=outputChars[0];try{int k=c.strict.GetChars(input,0,1,strictChars,0);Need(k==1&&strictChars[0]==singles[n],"Strict single mapping");SetBit(singleExact,n);}catch(DecoderFallbackException){}
     var decoder=c.normal.GetDecoder();int prefix=decoder.GetChars(input,0,1,outputChars,0,false);Need(prefix<=1,"Single-byte prefix width");
     if(n==0xef)modes[n]=1;
     else if(n==0xe8){var joined=c.normal.GetString(new[]{(byte)n,(byte)0xe9});if(joined.Length==2&&joined[1]=='\u200d')modes[n]=2;}
     if(modes[n]==0&&prefix==0){if(n==0xf0)modes[n]=3;else{modes[n]=4;var joined=c.normal.GetString(new[]{(byte)n,(byte)0xe9});Need(joined.Length==1,"Nukta composed width");alternate[n]=joined[0];}}
     Need(prefix==(modes[n]==1||modes[n]==3||modes[n]==4?0:1),"Pending classification");
    }
    foreach(ushort n in singles)output.Write(n);output.Write(singleExact);output.Write(modes);foreach(ushort n in alternate)output.Write(n);
    var split=c.normal.GetDecoder();var strictSplit=c.strict.GetDecoder();var raw=new byte[2];
    for(int n=0;n<65536;++n){raw[0]=(byte)(n>>8);raw[1]=(byte)n;int count=c.normal.GetChars(raw,0,2,outputChars,0);c.pairs[n]=new string(outputChars,0,count);bool exact=true;
     try{int k=c.strict.GetChars(raw,0,2,strictChars,0);Need(k==count&&new string(strictChars,0,k)==c.pairs[n],"Strict pair mapping");}catch(DecoderFallbackException){exact=false;}c.pairExact[n]=exact;Text(output,outputChars,count,exact);
     for(int cut=1;cut<=2;++cut){Reset(split,strictChars);int a=split.GetChars(raw,0,cut,strictChars,0,false);int b=split.GetChars(raw,cut,2-cut,strictChars,a,true);Need(new string(strictChars,0,a+b)==c.pairs[n],"Every pair split");bool ok=true;try{Reset(strictSplit,strictChars);a=strictSplit.GetChars(raw,0,cut,strictChars,0,false);b=strictSplit.GetChars(raw,cut,2-cut,strictChars,a,true);Need(new string(strictChars,0,a+b)==c.pairs[n],"Strict split output");}catch(DecoderFallbackException){ok=false;}Need(ok==exact,"Strict split membership");}
    }
    var contexts=new List<string>{"","\ud83d"};foreach(int n in new[]{0x0915,0x0995,0x0b95,0x0c15,0x0c95,0x0d15,0x0a95,0x0b15,0x0a15,0x094d,0x09cd,0x0bcd,0x0c4d,0x0ccd,0x0d4d,0x0acd,0x0b4d,0x0a4d})contexts.Add(new string((char)n,1));
    output.Write(contexts.Count);
    foreach(string prefix in contexts){output.Write((byte)prefix.Length);foreach(char unit in prefix)output.Write((ushort)unit);var prefixChars=prefix.ToCharArray();var referenceEncoder=c.normal.GetEncoder();Bytes(output,Slice(bytes,referenceEncoder.GetBytes(prefixChars,0,prefixChars.Length,bytes,0,false)));var suffixRuns=new Runs();
     for(int n=0;n<65536;++n){inputChars[0]=(char)n;var encoder=c.normal.GetEncoder();encoder.GetBytes(prefixChars,0,prefixChars.Length,bytes,0,false);int count=encoder.GetBytes(inputChars,0,1,bytes,0,true);var value=Slice(bytes,count);
      bool exact=prefix!="\ud83d"&&(GetBit(c.exact,n)||((prefix.Length==1&&((int)prefix[0]&127)==0x4d)&&(n==0x200c||n==0x200d)));
      if(GetBit(c.exact,n)||n==0x200c||n==0x200d||n>=0xd800&&n<=0xdfff||n%257==0){bool actual=true;try{var strictEncoder=c.strict.GetEncoder();strictEncoder.GetBytes(prefixChars,0,prefixChars.Length,bytes,0,false);int k=strictEncoder.GetBytes(inputChars,0,1,bytes,0,true);Need(Equal(value,Slice(bytes,k)),"Context strict bytes");}catch(EncoderFallbackException){actual=false;}Need(actual==exact,"Context strict classification");}
      suffixRuns.Add(value,exact);
     }suffixRuns.Write(output);
    }
    Console.WriteLine(cp+" all BMP, bytes, pairs/splits and 20 exhaustive encoder contexts verified");
   }
   // Confirm that selecting each script changes the mapping but retains the initial default for ATR 40/41.
   for(int cp=0;cp<10;++cp){var c=codecs[cp];var decoder=c.normal.GetDecoder();var selected=new byte[2];selected[0]=0xef;var raw=new byte[2];
    for(int font=0;font<10;++font){selected[1]=(byte)(font+0x42);for(int n=0;n<65536;++n){Reset(decoder,outputChars);Need(decoder.GetChars(selected,0,2,outputChars,0,false)==0,"Script selector emits no character");raw[0]=(byte)(n>>8);raw[1]=(byte)n;int count=decoder.GetChars(raw,0,2,outputChars,0,true);Need(new string(outputChars,0,count)==codecs[font].pairs[n],"Every selected-script pair");}} for(int font=0;font<10;++font)for(int reset=0x40;reset<=0x41;++reset)for(int n=0;n<256;++n){Reset(decoder,outputChars);selected[1]=(byte)(font+0x42);decoder.GetChars(selected,0,2,outputChars,0,false);var tail=new[]{(byte)0xef,(byte)reset,(byte)n};int count=decoder.GetChars(tail,0,3,outputChars,0,true);Need(new string(outputChars,0,count)==c.normal.GetString(new[]{(byte)n}),"ATR restores initial default");}
   }
  }
  Console.WriteLine("ISCII: ten complete BMP/default/exact maps; 655360 pair inputs and splits; 13107200 contextual BMP encodings; 6553600 selected-script pair decodings verified");
 }
}
