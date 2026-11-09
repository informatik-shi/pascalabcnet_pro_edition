// Development-only exhaustive Framework metadata/mapping reference.
using System;using System.IO;using System.Text;using System.Reflection;
using System.Runtime.InteropServices;using System.Collections.Generic;
class SbcsReference{
 unsafe static IntPtr PointerValue(object value){return new IntPtr(Pointer.Unbox(value));}
 static void Bit(byte[] data,int index){data[index/8]|=(byte)(1<<(index%8));}
 static void Need(bool condition,string message){if(!condition)throw new Exception(message);}
 static string Hex(byte[] data){return BitConverter.ToString(data).Replace("-","").ToLowerInvariant();}
 static string Hex(string text){if(text.Length==0)return "-";var result=new StringBuilder();foreach(char c in text)result.Append(((int)c).ToString("x4"));return result.ToString();}
 static void Files(string path,List<Encoding> codecs){using(var output=new StreamWriter(path,false,new UTF8Encoding(false))){
  output.WriteLine("# code_page\toperation\toutput_hex");var raw=new byte[256];for(int i=0;i<256;++i)raw[i]=(byte)i;
  foreach(var e in codecs)for(int op=0;op<6;++op){
   string result;
   if(op==0||op==3||op==4){using(var stream=new MemoryStream()){
    using(var writer=new StreamWriter(stream,e,1024,true)){
     if(op==0)writer.Write("A\r\nПривет éＡ界🌍\0Z");
     else if(op==3)writer.Write(new string('A',1023)+"\ud83d\ude00"+new string('B',1024)+"\ud83d");
     else{writer.Write(new string('A',1023)+"\ud83d");writer.Write("\ude00"+new string('B',1024)+"\ud83d");}
    }result=Hex(stream.ToArray());
   }}else{
    var bytes=op==5 ? Encoding.UTF8.GetPreamble():raw;
    if(op==5){var payload=Encoding.UTF8.GetBytes("Привет");var joined=new byte[bytes.Length+payload.Length];Array.Copy(bytes,joined,bytes.Length);Array.Copy(payload,0,joined,bytes.Length,payload.Length);bytes=joined;}
    using(var reader=new StreamReader(new MemoryStream(bytes),e,true)){
     if(op==2){var text=new StringBuilder();string line;while((line=reader.ReadLine())!=null)text.Append(line).Append('\n');result=Hex(text.ToString());}
     else result=Hex(reader.ReadToEnd());
    }
   }
   output.WriteLine(e.CodePage+"\t"+op+"\t"+result);
  }
 }}
 unsafe static void Main(string[] args){
  var codecs=new List<Encoding>();foreach(var info in Encoding.GetEncodings()){var e=info.GetEncoding();if(e.IsSingleByte)codecs.Add(e);}codecs.Sort((a,b)=>a.CodePage.CompareTo(b.CodePage));
  using(var output=new BinaryWriter(File.Create(args[0]))){
   output.Write(Encoding.ASCII.GetBytes("ABCSBCS2"));output.Write(codecs.Count);
   foreach(var e in codecs){
    var exact=(Encoding)e.Clone();exact.EncoderFallback=new EncoderReplacementFallback("??");exact.DecoderFallback=DecoderFallback.ExceptionFallback;
    var strict=(Encoding)e.Clone();strict.EncoderFallback=EncoderFallback.ExceptionFallback;strict.DecoderFallback=DecoderFallback.ExceptionFallback;
    var input=new char[1];var encoded=new byte[8];var decoded=new char[8];var one=new byte[1];
    var decode=new ushort[256];var decodeStrict=new byte[32];
    for(int n=0;n<256;++n){one[0]=(byte)n;int length=e.GetChars(one,0,1,decoded,0);Need(length==1,"Non-BMP/non-single decode "+e.CodePage);decode[n]=decoded[0];
     try{length=strict.GetChars(one,0,1,decoded,0);Need(length==1&&decoded[0]==decode[n],"Strict decode mismatch");Bit(decodeStrict,n);}catch(DecoderFallbackException){}
    }
    var encode=new byte[65536];var encodeStrict=new byte[8192];int checkedStrict=0;
    input[0]='\ud800';Need(e.GetBytes(input,0,1,encoded,0)==1,"Non-single default fallback");byte fallback=encoded[0];
    for(int n=0;n<65536;++n){input[0]=(char)n;int length=e.GetBytes(input,0,1,encoded,0);Need(length==1,"Non-single encode "+e.CodePage+" "+n);encode[n]=encoded[0];
     length=exact.GetBytes(input,0,1,encoded,0);Need(length==1||length==2,"Replacement classification mismatch");bool mapped=length==1;if(mapped)Bit(encodeStrict,n);
     // Check every exact mapping and every nontrivial best-fit mapping against
     // actual throwing fallback; also sample every remaining BMP block.
     if(mapped||encode[n]!=fallback||n%257==0||n>=0xd800&&n<=0xdfff){
      bool ok=true;try{length=strict.GetBytes(input,0,1,encoded,0);Need(length==1&&encoded[0]==encode[n],"Strict encode mismatch");}catch(EncoderFallbackException){ok=false;}
      Need(ok==mapped,"Strict classification mismatch "+e.CodePage+" "+n);++checkedStrict;
     }
    }
    var encoder=e.GetEncoder();var pairBytes=new byte[8];int first=encoder.GetBytes(new[]{'\ud83d'},0,1,pairBytes,0,false);int second=encoder.GetBytes(new[]{'\ude00'},0,1,pairBytes,first,true);
    Need((first==0||first==1)&&first+second==2&&pairBytes[0]==fallback&&pairBytes[1]==fallback,"Streaming surrogate fallback");
    output.Write(e.CodePage);output.Write(e.WebName);output.Write(e.GetType().FullName);output.Write(encode[0xd800]);output.Write((byte)(first==0?1:0));
    foreach(var value in decode)output.Write(value);output.Write(decodeStrict);output.Write(encode);output.Write(encodeStrict);
    Console.WriteLine(e.CodePage+" "+e.WebName+" verified "+checkedStrict+" throwing-fallback cases");
   }
  }
  var table=typeof(Encoding).Assembly.GetType("System.Globalization.EncodingTable");var field=table.GetField("encodingDataPtr",BindingFlags.NonPublic|BindingFlags.Static);var element=field.FieldType.GetElementType();
  int size=Marshal.SizeOf(element),count=(int)table.GetField("lastEncodingItem",BindingFlags.NonPublic|BindingFlags.Static).GetValue(null)+1;
  var address=PointerValue(field.GetValue(null));var aliases=new SortedDictionary<string,int>(StringComparer.Ordinal);
  for(int n=0;n<count;++n){var entry=Marshal.PtrToStructure(new IntPtr(address.ToInt64()+n*size),element);var name=Marshal.PtrToStringAnsi(PointerValue(element.GetField("webName",BindingFlags.Instance|BindingFlags.Public|BindingFlags.NonPublic).GetValue(entry)));int code=(ushort)element.GetField("codePage",BindingFlags.Instance|BindingFlags.Public|BindingFlags.NonPublic).GetValue(entry);
   Need(Encoding.GetEncoding(name).CodePage==code,"Alias mismatch "+name);aliases.Add(name.ToLowerInvariant(),code);
  }
  using(var output=new StreamWriter(args[1],false,new UTF8Encoding(false))){output.WriteLine("# alias\tcode_page");foreach(var pair in aliases)output.WriteLine(pair.Key+"\t"+pair.Value);}
  Files(args[2],codecs);
  using(var output=new StreamWriter(args[3],false,new UTF8Encoding(false))){
   output.WriteLine("# name_UTF16_hex\tcode_page\terror");
   foreach(string name in new[]{"utf8","cp1251","utf-8 "," utf-8","Utf-8","UTF-16LE","utf_8","","x-unknown","utf--8","cp28591","latin1",null}){
    int code=0;string error="ok";try{code=Encoding.GetEncoding(name).CodePage;}catch(ArgumentNullException){error="E_NIL";}catch(ArgumentException){error="E_ENCODING";}
    output.WriteLine((name==null?"nil":Hex(name))+"\t"+code+"\t"+error);
   }
  }
  Console.WriteLine(codecs.Count+" codecs; "+aliases.Count+" aliases");
 }
}
