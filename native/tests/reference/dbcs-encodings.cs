// Development-only exhaustive Framework reference. Does not invoke Pascal.
using System;using System.IO;using System.Text;using System.Collections.Generic;using System.Reflection;
class DbcsReference{
 static void Need(bool ok,string text){if(!ok)throw new Exception(text);}
 static void Bit(byte[] bits,int n){bits[n>>3]|=(byte)(1<<(n&7));}
 static string Hex(byte[] data){return BitConverter.ToString(data).Replace("-","").ToLowerInvariant();}
 static string Hex(string text){if(text.Length==0)return "-";var result=new StringBuilder();foreach(char c in text)result.Append(((int)c).ToString("x4"));return result.ToString();}
 static void Files(string path,List<Encoding> codecs){using(var output=new StreamWriter(path,false,new UTF8Encoding(false))){
  output.WriteLine("# code_page\toperation\tinput_bytes_hex\toutput_hex");var raw=new byte[256];for(int i=0;i<256;++i)raw[i]=(byte)i;
  foreach(var e in codecs)for(int op=0;op<7;++op){string result,input="-";
   if(op==0||op==3||op==4){using(var stream=new MemoryStream()){
    using(var writer=new StreamWriter(stream,e,1024,true)){
     if(op==0)writer.Write("A\r\nПривет 日本語 中文 한국어 éＡ🌍\0Z");
     else if(op==3)writer.Write(new string('A',1023)+"\ud83d\ude00"+new string('B',1024)+"\ud83d");
     else{writer.Write(new string('A',1023)+"\ud83d");writer.Write("\ude00"+new string('B',1024)+"\ud83d");}
    }result=Hex(stream.ToArray());
   }}else{byte[] bytes=raw;
    if(op==5){var a=Encoding.UTF8.GetPreamble();var b=Encoding.UTF8.GetBytes("日本語");bytes=new byte[a.Length+b.Length];Array.Copy(a,bytes,a.Length);Array.Copy(b,0,bytes,a.Length,b.Length);}
    if(op==6){byte[] wide=null;for(int n=128;n<65536;++n){var candidate=e.GetBytes(new[]{(char)n});if(candidate.Length==2){wide=candidate;break;}}Need(wide!=null,"Missing wide character");var a=e.GetBytes("A");Need(a.Length==1,"Non-single ASCII");bytes=new byte[1028];for(int n=0;n<1023;++n)bytes[n]=a[0];bytes[1023]=wide[0];bytes[1024]=wide[1];bytes[1025]=13;bytes[1026]=10;bytes[1027]=wide[0];}
    input=Hex(bytes);using(var reader=new StreamReader(new MemoryStream(bytes),e,true,1024)){
     if(op==2){var text=new StringBuilder();string line;while((line=reader.ReadLine())!=null)text.Append(line).Append('\n');result=Hex(text.ToString());}else result=Hex(reader.ReadToEnd());
    }
   }
   output.WriteLine(e.CodePage+"\t"+op+"\t"+input+"\t"+result);
  }
 }}
 static void Main(string[] args){
  if(args.Length>2&&args[2]=="files-only"){
   var provider=Type.GetType("System.Text.CodePagesEncodingProvider, System.Text.Encoding.CodePages");
   if(provider!=null)typeof(Encoding).GetMethod("RegisterProvider",BindingFlags.Public|BindingFlags.Static).Invoke(null,new[]{provider.GetProperty("Instance").GetValue(null,null)});
  }
  var codecs=new List<Encoding>();foreach(var info in Encoding.GetEncodings()){var e=info.GetEncoding();if(e.GetType().Name=="DBCSCodePageEncoding"||e.GetType().Name=="EUCJPEncoding")codecs.Add(e);}codecs.Sort((a,b)=>a.CodePage.CompareTo(b.CodePage));
  if(args.Length>2&&args[2]=="files-only"){codecs.Clear();foreach(int cp in new[]{932,936,949,950,1361,10001,10002,10003,10008,20000,20001,20002,20003,20004,20005,20261,20932,20936,20949,50227,51932,51936,51949})codecs.Add(Encoding.GetEncoding(cp));Need(codecs.Count==23,"File codec count");Files(args[1],codecs);return;}
  Need(codecs.Count==23,"Framework DBCS codec count");
  using(var output=new BinaryWriter(File.Create(args[0]))){
   output.Write(Encoding.ASCII.GetBytes("ABCDBCS1"));output.Write(codecs.Count);
   foreach(var e in codecs){
    var strict=(Encoding)e.Clone();strict.EncoderFallback=EncoderFallback.ExceptionFallback;strict.DecoderFallback=DecoderFallback.ExceptionFallback;
    var classified=(Encoding)e.Clone();classified.EncoderFallback=new EncoderReplacementFallback("???");
    var decoder=e.GetDecoder();var one=new byte[1];var pair=new byte[2];var chars=new char[8];var bytes=new byte[16];
    var leads=new byte[32];var singles=new ushort[256];var singleExact=new byte[32];
    ushort fallbackChar=0;bool haveFallback=false;
    for(int n=0;n<256;++n){one[0]=(byte)n;decoder.Reset();int prefix=decoder.GetChars(one,0,1,chars,0,false);Need(prefix==0||prefix==1,"Invalid prefix length");bool lead=prefix==0;if(lead)Bit(leads,n);
     int length=e.GetChars(one,0,1,chars,0);Need(length==1,"Single-byte final decode length");singles[n]=chars[0];
     if(lead){if(!haveFallback){fallbackChar=chars[0];haveFallback=true;}else Need(fallbackChar==chars[0],"Per-lead fallback mismatch");}
     try{length=strict.GetChars(one,0,1,chars,0);Need(length==1&&chars[0]==singles[n],"Strict single mismatch");Bit(singleExact,n);}catch(DecoderFallbackException){}
    }
    Need(haveFallback,"Missing lead bytes");var pairs=new uint[65536];var pairExact=new byte[8192];
    for(int n=0;n<65536;++n){pair[0]=(byte)(n>>8);pair[1]=(byte)n;int length=e.GetChars(pair,0,2,chars,0);bool lead=(leads[pair[0]>>3]&(1<<(pair[0]&7)))!=0;
     Need(length==(lead?1:2),"DBCS pair consumption mismatch "+e.CodePage+" "+n);pairs[n]=(uint)chars[0]|(length==2?(uint)chars[1]<<16:0);
     try{int exact=strict.GetChars(pair,0,2,chars,0);Need(exact==length&&((uint)chars[0]|(length==2?(uint)chars[1]<<16:0))==pairs[n],"Strict pair mismatch");Bit(pairExact,n);}catch(DecoderFallbackException){}
     // Compare split decoding with the public stateful decoder for every pair.
     decoder.Reset();int first=decoder.GetChars(pair,0,1,chars,0,false);int second=decoder.GetChars(pair,1,1,chars,first,true);
     Need(first+second==length&&((uint)chars[0]|(length==2?(uint)chars[1]<<16:0))==pairs[n],"Split pair mismatch");
    }
    if(e.CodePage==51932){
     var triple=new byte[3];triple[0]=0x8f;
     for(int n=0;n<65536;++n){triple[1]=(byte)(n>>8);triple[2]=(byte)n;var expected=new StringBuilder();
      for(int cursor=0;cursor<3;){int b=triple[cursor++];if((leads[b>>3]&(1<<(b&7)))!=0&&cursor<3){expected.Append((char)pairs[(b<<8)|triple[cursor++]]);}else expected.Append((char)singles[b]);}
      string value=e.GetString(triple);Need(value==expected.ToString(),"EUC-JP JIS0212-prefix consumption");
      for(int cut=1;cut<=2;++cut){decoder.Reset();int a=decoder.GetChars(triple,0,cut,chars,0,false);int b=decoder.GetChars(triple,cut,3-cut,chars,a,true);Need(new string(chars,0,a+b)==value,"EUC-JP triple split");}
     }
    }
    var input=new char[1];var encoded=new ushort[65536];var encodeExact=new byte[8192];input[0]='\ud800';Need(e.GetBytes(input,0,1,bytes,0)==1,"Default encoder fallback width");byte fallback=bytes[0];
    for(int n=0;n<65536;++n){input[0]=(char)n;int length=e.GetBytes(input,0,1,bytes,0);Need(length==1||length==2,"DBCS encoder width");encoded[n]=(ushort)(length==1?bytes[0]:(bytes[0]<<8)|bytes[1]);
     int replaced=classified.GetBytes(input,0,1,bytes,0);Need(replaced==1||replaced==2||replaced==3,"Exact-map classification width");bool exact=replaced!=3;if(exact)Bit(encodeExact,n);
     if(exact||encoded[n]!=fallback||n%257==0||n>=0xd800&&n<=0xdfff){bool ok=true;try{int l=strict.GetBytes(input,0,1,bytes,0);Need(l==length&&(l==1?bytes[0]:(bytes[0]<<8)|bytes[1])==encoded[n],"Strict encode value mismatch");}catch(EncoderFallbackException){ok=false;}Need(ok==exact,"Strict encode membership mismatch");}
    }
    // All supplementary scalar values are unrepresentable and use two fallbacks.
    var scalar=new char[2];for(int high=0xd800;high<=0xdbff;++high)for(int low=0xdc00;low<=0xdfff;++low){scalar[0]=(char)high;scalar[1]=(char)low;int length=e.GetBytes(scalar,0,2,bytes,0);Need(length==2&&bytes[0]==fallback&&bytes[1]==fallback,"Supplementary fallback mismatch");}
    var encoder=e.GetEncoder();scalar[0]='\ud83d';scalar[1]='\ude00';int p=encoder.GetBytes(scalar,0,1,bytes,0,false);int q=encoder.GetBytes(scalar,1,1,bytes,p,true);Need(p==0&&q==2&&bytes[0]==fallback&&bytes[1]==fallback,"Encoder carry mismatch");
    output.Write(e.CodePage);output.Write(e.WebName);output.Write(e.GetType().FullName);output.Write(fallback);output.Write(fallbackChar);output.Write(leads);
    foreach(var c in singles)output.Write(c);output.Write(singleExact);foreach(var c in pairs)output.Write(c);output.Write(pairExact);foreach(var c in encoded)output.Write(c);output.Write(encodeExact);
    Console.WriteLine(e.CodePage+" "+e.WebName+" all BMP, byte pairs, splits and supplementary scalars verified");
   }
  }
  Console.WriteLine(codecs.Count+" DBCS codecs");
  Files(args[1],codecs);
 }
}
