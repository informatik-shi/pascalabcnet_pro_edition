// Development-only Framework reference; never invokes the Pascal compiler.
using System;using System.IO;using System.Text;using System.Collections.Generic;using System.Reflection;
class Gb18030Reference{
 static void Need(bool ok,string text){if(!ok)throw new Exception(text);}
 static void Bit(byte[] data,int n){data[n>>3]|=(byte)(1<<(n&7));}
 static uint Packed(byte[] data,int count){uint value=0;for(int n=0;n<count;++n)value=(value<<8)|data[n];return value;}
 static uint Scalar(char[] data,int count){if(count==1)return data[0];Need(count==2&&char.IsSurrogatePair(data[0],data[1]),"Invalid decoded scalar");return (uint)char.ConvertToUtf32(data[0],data[1]);}
 static byte[] Four(int offset){var result=new byte[4];result[3]=(byte)(offset%10+0x30);offset/=10;result[2]=(byte)(offset%126+0x81);offset/=126;result[1]=(byte)(offset%10+0x30);result[0]=(byte)(offset/10+0x81);return result;}
 static int Split(Decoder decoder,byte[] input,int cut,char[] output){decoder.Reset();int count=decoder.GetChars(input,0,cut,output,0,false);return count+decoder.GetChars(input,cut,input.Length-cut,output,count,true);}
 static void Main(string[] args){
  if(args.Length>1){var provider=Type.GetType("System.Text.CodePagesEncodingProvider, System.Text.Encoding.CodePages");typeof(Encoding).GetMethod("RegisterProvider").Invoke(null,new[]{provider.GetProperty("Instance").GetValue(null,null)});}
  var encoding=Encoding.GetEncoding(54936);var strict=(Encoding)encoding.Clone();strict.EncoderFallback=EncoderFallback.ExceptionFallback;strict.DecoderFallback=DecoderFallback.ExceptionFallback;
  var decoder=encoding.GetDecoder();var strictDecoder=strict.GetDecoder();var chars=new char[16];var other=new char[16];var bytes=new byte[16];var one=new char[1];
  Need(encoding.GetType().FullName=="System.Text.GB18030Encoding","Runtime class");
  var encoded=new uint[65536];var encodedExact=new byte[8192];
  for(int n=0;n<65536;++n){one[0]=(char)n;int count=encoding.GetBytes(one,0,1,bytes,0);Need(count==1||count==2||count==4,"BMP encoded width");encoded[n]=Packed(bytes,count);
   try{int exact=strict.GetBytes(one,0,1,bytes,0);Need(exact==count&&Packed(bytes,exact)==encoded[n],"Strict BMP mapping");Bit(encodedExact,n);}catch(EncoderFallbackException){Need(char.IsSurrogate(one[0]),"Unexpected BMP fallback");}
  }
  var singles=new ushort[256];var singlesExact=new byte[32];var raw=new byte[1];
  for(int n=0;n<256;++n){raw[0]=(byte)n;int count=encoding.GetChars(raw,0,1,chars,0);Need(count==1,"Single byte width");singles[n]=chars[0];try{int exact=strict.GetChars(raw,0,1,chars,0);Need(exact==1&&chars[0]==singles[n],"Strict single value");Bit(singlesExact,n);}catch(DecoderFallbackException){}}
  var pairs=new uint[65536];var pairFlags=new byte[65536];raw=new byte[2];
  for(int n=0;n<65536;++n){raw[0]=(byte)(n>>8);raw[1]=(byte)n;int count=encoding.GetChars(raw,0,2,chars,0);Need(count==1||count==2,"Pair output width");pairs[n]=(uint)chars[0]|(count==2?(uint)chars[1]<<16:0);pairFlags[n]=(byte)count;
   try{int exact=strict.GetChars(raw,0,2,chars,0);Need(exact==count&&((uint)chars[0]|(exact==2?(uint)chars[1]<<16:0))==pairs[n],"Strict pair value");pairFlags[n]|=4;}catch(DecoderFallbackException){}
  }
  const int fourCount=126*10*126*10;var four=new uint[fourCount];
  for(int n=0;n<fourCount;++n){raw=Four(n);int count=encoding.GetChars(raw,0,4,chars,0);uint scalar=Scalar(chars,count);bool exact=true;
   try{int length=strict.GetChars(raw,0,4,other,0);Need(Scalar(other,length)==scalar,"Strict four-byte value");}catch(DecoderFallbackException){exact=false;Need(count==1&&scalar==0x3f,"Four-byte fallback");}
   four[n]=exact?scalar:uint.MaxValue;
   for(int cut=1;cut<=4;++cut){int split=Split(decoder,raw,cut,other);Need(split==count&&Scalar(other,split)==scalar,"Four-byte split output");}
  }
  var scalars=new uint[1048576];var pair=new char[2];
  for(int n=0;n<scalars.Length;++n){pair[0]=(char)(0xd800+(n>>10));pair[1]=(char)(0xdc00+(n&1023));int count=encoding.GetBytes(pair,0,2,bytes,0);Need(count==4,"Supplementary width");scalars[n]=Packed(bytes,count);int exact=strict.GetBytes(pair,0,2,bytes,0);Need(exact==4&&Packed(bytes,exact)==scalars[n],"Strict supplementary value");
   raw=new[]{bytes[0],bytes[1],bytes[2],bytes[3]};int back=encoding.GetChars(raw,0,4,chars,0);Need(back==2&&chars[0]==pair[0]&&chars[1]==pair[1],"Supplementary round trip");
  }
  var cases=new List<byte[]>();for(int n=0;n<256;++n)cases.Add(new[]{(byte)n});for(int n=0;n<65536;++n)cases.Add(new[]{(byte)(n>>8),(byte)n});
  // Every malformed third/fourth byte after a representative four-byte prefix.
  for(int n=0;n<65536;++n)cases.Add(new[]{(byte)0x81,(byte)0x30,(byte)(n>>8),(byte)n});
  foreach(byte lead in new byte[]{0x81,0x84,0x8f,0x90,0xe3,0xfe})foreach(byte digit in new byte[]{0x30,0x39})for(int n=0;n<256;++n)cases.Add(new[]{lead,digit,(byte)n});
  using(var output=new BinaryWriter(File.Create(args[0]))){
   output.Write(Encoding.ASCII.GetBytes("ABCGB181"));output.Write(54936);output.Write(encoding.GetType().FullName);
   foreach(uint value in encoded)output.Write(value);output.Write(encodedExact);foreach(ushort value in singles)output.Write(value);output.Write(singlesExact);
   for(int n=0;n<65536;++n){output.Write(pairs[n]);output.Write(pairFlags[n]);}
   output.Write(four.Length);foreach(uint value in four)output.Write(value);output.Write(scalars.Length);foreach(uint value in scalars)output.Write(value);
   output.Write(cases.Count);
   foreach(var input in cases){output.Write((byte)input.Length);output.Write(input);
    for(int cut=0;cut<=input.Length;++cut){int count=Split(decoder,input,cut,chars);Need(count<=16,"Stream case width");bool exact=true;
     try{int length=Split(strictDecoder,input,cut,other);Need(length==count,"Strict stream length");for(int n=0;n<count;++n)Need(chars[n]==other[n],"Strict stream value");}catch(DecoderFallbackException){exact=false;}
     output.Write((byte)(count|(exact?128:0)));for(int n=0;n<count;++n)output.Write((ushort)chars[n]);
    }
   }
  }
  Console.WriteLine("GB18030: 65536 BMP encodings, 256 single bytes, 65536 pairs, "+fourCount+" four-byte inputs and splits, 1048576 supplementary scalars, "+cases.Count+" variable streaming inputs verified");
 }
}
