// Development-only actual StreamReader/StreamWriter oracle; no Pascal compiler.
using System;using System.IO;using System.Text;using System.Collections.Generic;
class Utf7Files{
 static string Hex(byte[] b){return b.Length==0?"-":BitConverter.ToString(b).Replace("-","").ToLowerInvariant();}
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("x4"));return b.ToString();}
 static byte[] Cat(byte[] a,byte[] b){var r=new byte[a.Length+b.Length];Array.Copy(a,r,a.Length);Array.Copy(b,0,r,a.Length,b.Length);return r;}
 static void Main(string[] args){var e=new UTF7Encoding(false);int rows=0;using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){output.WriteLine("# code_page\toperation\tinput_bytes_hex\toutput_hex");var inputs=new List<byte[]>();for(int n=0;n<256;++n)inputs.Add(new[]{(byte)n});var all=new byte[256];for(int n=0;n<256;++n)all[n]=(byte)n;inputs.Add(all);
  var prefixes=new List<byte[]>{new byte[0],new byte[]{43}};for(int length=1;length<=8;++length)foreach(byte value in new byte[]{65,66,47,122}){var p=new byte[length+1];p[0]=43;for(int n=1;n<p.Length;++n)p[n]=value;prefixes.Add(p);}
  foreach(var prefix in prefixes){inputs.Add(prefix);foreach(byte tail in new byte[]{0,13,10,33,43,45,65,128,255})inputs.Add(Cat(prefix,new[]{tail}));}
  foreach(string text in new[]{"+2AA-","+3gA-","+2D3eAA-","+-","+!","+AA","+AAAA","+ZeU-+ZeU-","A\0B"})inputs.Add(Encoding.ASCII.GetBytes(text));
  for(int cut=0;cut<8;++cut)foreach(string tail in new[]{"+","+A","+AA","+AAA","+AAAA","+AAAAA","+AAAAAA","+AAAAAAA","+AAAAAAAA","+2AA-","+3gA-","+ZeU-\r\n+"})inputs.Add(Cat(Encoding.ASCII.GetBytes(new string('A',1024-cut)),Encoding.ASCII.GetBytes(tail)));
  foreach(var u in new[]{Encoding.UTF8,Encoding.Unicode,new UTF32Encoding(false,true)})inputs.Add(Cat(u.GetPreamble(),u.GetBytes("日本語 한국 中文 🌍")));
  foreach(var input in inputs)for(int op=1;op<=2;++op){string text;using(var reader=new StreamReader(new MemoryStream(input),e,true,1024)){if(op==1)text=reader.ReadToEnd();else{var b=new StringBuilder();string line;while((line=reader.ReadLine())!=null)b.Append(line).Append('\n');text=b.ToString();}}output.WriteLine("65000\t"+op+"\t"+Hex(input)+"\t"+Hex(text));++rows;}
  foreach(int op in new[]{0,3,4,5,6,7,8,9,10,11,12})using(var stream=new MemoryStream()){
   using(var writer=new StreamWriter(stream,e,1024,true)){
    if(op==0)writer.Write("A\r\n日本語 + 中文 한국어 éＡ🌍\0Z\ud83d");
    else if(op==3)writer.Write(new string('A',1023)+"日日"+new string('B',1024)+"+\ud83d");
    else if(op==4){writer.Write(new string('A',1023)+"日\ud83d");writer.Write("\ude00日"+new string('B',1024)+"+\ud83d");}
    else if(op==5){writer.Write("日");writer.Flush();writer.Write("日");}
    else if(op==6){writer.Write("日+");writer.Flush();writer.Write("日+");}
    else if(op==7)writer.Write("日");
    else if(op==8){writer.Write("日");writer.Write('A');writer.Write("日");}
    else if(op==9||op==10){writer.Write(new string('日',1025));if(op==9){writer.BaseStream.Seek(0,SeekOrigin.Begin);writer.Write("日");}}
    else if(op==11){writer.Write("日日日");writer.Flush();writer.Flush();writer.Write('+');}
    else{writer.Write("日");writer.Flush();writer.Write('\ud83d');writer.Flush();writer.Write('\ude00');}
   }if(op==7)using(var writer=new StreamWriter(stream,e,1024,true))writer.Write("日");output.WriteLine("65000\t"+op+"\t-\t"+Hex(stream.ToArray()));++rows;
  }
 }Console.WriteLine(rows+" UTF7 file recipes");}
}