// Development-only actual StreamReader/StreamWriter reference.
using System;using System.IO;using System.Text;using System.Collections.Generic;using System.Reflection;
class GbFiles{
 static string Hex(byte[] data){return BitConverter.ToString(data).Replace("-","").ToLowerInvariant();}
 static string Hex(string text){if(text.Length==0)return "-";var b=new StringBuilder();foreach(char c in text)b.Append(((int)c).ToString("x4"));return b.ToString();}
 static void Main(string[] args){
  if(args.Length>1){var provider=Type.GetType("System.Text.CodePagesEncodingProvider, System.Text.Encoding.CodePages");typeof(Encoding).GetMethod("RegisterProvider").Invoke(null,new[]{provider.GetProperty("Instance").GetValue(null,null)});}
  var encoding=Encoding.GetEncoding(54936);var cases=new List<byte[]>();for(int n=0;n<256;++n)cases.Add(new[]{(byte)n});
  var everyByte=new byte[256];for(int n=0;n<256;++n)everyByte[n]=(byte)n;cases.Add(everyByte);
  foreach(byte lead in new byte[]{0x81,0x84,0x8f,0x90,0xe3,0xfe})foreach(byte digit in new byte[]{0x30,0x39}){
   cases.Add(new[]{lead,digit});foreach(byte third in new byte[]{0,0x30,0x39,0x40,0x7f,0x81,0xfe,0xff})cases.Add(new[]{lead,digit,third});
  }
  cases.AddRange(new[]{new byte[]{0xef,0xbb},new byte[]{0xff,0xfe},new byte[]{0xff,0xfe,0},new byte[]{0,0,0xfe},new byte[]{0xfe,0xff},new byte[]{0x81,0x30,0x81,0x30},new byte[]{0x84,0x31,0xa5,0x30},new byte[]{0x90,0x30,0x81,0x30},new byte[]{0xe3,0x32,0x9a,0x36}});
  foreach(var unicode in new[]{Encoding.UTF8,Encoding.Unicode,Encoding.BigEndianUnicode,new UTF32Encoding(false,true),new UTF32Encoding(true,true)}){var preamble=unicode.GetPreamble();var text=unicode.GetBytes("日本語 🌍");var bytes=new byte[preamble.Length+text.Length];Array.Copy(preamble,bytes,preamble.Length);Array.Copy(text,0,bytes,preamble.Length,text.Length);cases.Add(bytes);}
  var wide=encoding.GetBytes("🌍");for(int cut=1;cut<=3;++cut)for(int pending=0;pending<=3;++pending){
   var bytes=new byte[1024-cut+wide.Length+2+pending];for(int n=0;n<1024-cut;++n)bytes[n]=(byte)'A';Array.Copy(wide,0,bytes,1024-cut,wide.Length);bytes[1024-cut+4]=13;bytes[1024-cut+5]=10;Array.Copy(wide,0,bytes,bytes.Length-pending,pending);cases.Add(bytes);
  }
  using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){output.WriteLine("# operation\tinput_bytes_hex\toutput_hex");
   foreach(var input in cases)for(int op=1;op<=2;++op){string text;using(var reader=new StreamReader(new MemoryStream(input),encoding,true,1024)){
    if(op==1)text=reader.ReadToEnd();else{var b=new StringBuilder();string line;while((line=reader.ReadLine())!=null)b.Append(line).Append('\n');text=b.ToString();}
   }output.WriteLine(op+"\t"+Hex(input)+"\t"+Hex(text));}
   foreach(int op in new[]{0,3,4,5})using(var stream=new MemoryStream()){
    using(var writer=new StreamWriter(stream,encoding,1024,true)){
     if(op==0)writer.Write("A\r\nПривет 日本語 中文 한국어 éＡ🌍\0Z");
     else if(op==3)writer.Write(new string('A',1023)+"\ud83d\ude00"+new string('B',1024)+"\ud83d");
     else if(op==4){writer.Write(new string('A',1023)+"\ud83d");writer.Write("\ude00"+new string('B',1024)+"\ud83d");}
     else {writer.Write("\ud83d");writer.Flush();writer.Write("\ude00");}
    }output.WriteLine(op+"\t-\t"+Hex(stream.ToArray()));
   }
  }
  Console.WriteLine((cases.Count*2+4)+" GB18030 file recipes");
 }
}
