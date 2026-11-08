// Independent development-only stream recipes; never installed.
// Does not load Pascal assemblies or invoke a Pascal compiler.
using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
[assembly:System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.7.2")]
class Reference {
 static string Hex(byte[] bytes){var s=new StringBuilder();foreach(byte b in bytes)s.Append(b.ToString("x2"));return s.Length==0?"-":s.ToString();}
 static string Text(string text){if(text==null)return "nil";var s=new StringBuilder();foreach(char c in text)s.Append(((int)c).ToString("x4"));return s.Length==0?"-":s.ToString();}
 static byte[] Bytes(params int[] bytes){return Array.ConvertAll(bytes,x=>(byte)x);}
 static byte[] Join(byte[] a,byte[] b){var r=new byte[a.Length+b.Length];a.CopyTo(r,0);b.CopyTo(r,a.Length);return r;}
 static string Read(byte[] bytes,Encoding encoding){
  // FilesOperations: EOF, Readln(Text,var string), StringBuilder.AppendLine.
  using(var input=new MemoryStream(bytes))using(var reader=new StreamReader(input,encoding,true,1024)){
   var result=new StringBuilder();while(!reader.EndOfStream)result.AppendLine(reader.ReadLine());
   return result.ToString().Replace("\r\n","\n");
  }
 }
 static byte[] Write(string text,Encoding encoding){
  using(var output=new MemoryStream()){
   using(var writer=new StreamWriter(output,encoding,1024,true))writer.Write(text??"nil");
   return output.ToArray();
  }
 }
 static void Sharing(string outputName,string scratch){
  if(Environment.OSVersion.Platform!=PlatformID.Win32NT)return;
  scratch=Path.GetFullPath(scratch);
  string directory=Path.Combine(scratch,"file-sharing-reference-"+Guid.NewGuid().ToString("N"));
  string name=Path.Combine(directory,"held.txt");Directory.CreateDirectory(directory);
  try{
   using(var output=new StreamWriter(outputName,false,new UTF8Encoding(false))){
    output.WriteLine("# Independent Windows .NET "+Environment.Version+" FileStream sharing: access, share, operation, outcome, result text, final bytes");
    foreach(int access in new[]{1,2,3})foreach(int share in new[]{0,1,2,3,4,5,7})foreach(string operation in new[]{"R","W"}){
     File.WriteAllBytes(name,Encoding.ASCII.GetBytes("old"));string outcome="ok",result="-";
     using(var held=new FileStream(name,FileMode.Open,(FileAccess)access,(FileShare)share)){
      try{
       if(operation=="R")using(var reader=new StreamReader(name,Encoding.GetEncoding(1251))){var value=new StringBuilder();while(!reader.EndOfStream)value.AppendLine(reader.ReadLine());result=Text(value.ToString().Replace("\r\n","\n"));}
       else using(var writer=new StreamWriter(name,false,Encoding.GetEncoding(1251)))writer.Write("changed");
      }catch(IOException){outcome="E_IO";}
     }
     output.WriteLine(access+"\t"+share+"\t"+operation+"\t"+outcome+"\t"+result+"\t"+Hex(File.ReadAllBytes(name)));
    }
   }
  }finally{
   if(Path.GetDirectoryName(Path.GetFullPath(directory))!=scratch||!Path.GetFileName(directory).StartsWith("file-sharing-reference-"))throw new Exception("Unsafe reference cleanup");
   File.Delete(name);Directory.Delete(directory);
  }
 }
 public static void Main(string[] args){
  // Framework already has code pages; modern runtimes provide them separately.
  var provider=Type.GetType("System.Text.CodePagesEncodingProvider, System.Text.Encoding.CodePages",false);
  var register=typeof(Encoding).GetMethod("RegisterProvider");
  if(provider!=null&&register!=null)register.Invoke(null,new[]{provider.GetProperty("Instance").GetValue(null,null)});
  var raw=new List<byte[]>();raw.Add(new byte[0]);
  foreach(int b in new[]{0,9,10,13,32,65,127,128,152,191,192,193,194,223,224,237,239,240,244,245,254,255})raw.Add(Bytes(b));
  var edges=new[]{0,10,13,65,127,128,159,160,175,176,191,192,194,223,224,237,239,240,244,254,255};
  foreach(int lead in new[]{194,223,224,237,239,240,244,245,254,255})foreach(int tail in edges)raw.Add(Bytes(lead,tail));
  foreach(var b in new[]{Bytes(0xe0,0x80,0x80),Bytes(0xe0,0xa0,0x80),Bytes(0xed,0x9f,0xbf),Bytes(0xed,0xa0,0x80),Bytes(0xed,0xbf,0xbf),Bytes(0xf0,0x80,0x80,0x80),Bytes(0xf0,0x90,0x80,0x80),Bytes(0xf4,0x8f,0xbf,0xbf),Bytes(0xf4,0x90,0x80,0x80),Bytes(0xf0,0x9f,0x8c,0x8d),Bytes(0xff,0xfe),Bytes(0xff,0xfe,0,0),Bytes(0xfe,0xff),Bytes(0xef,0xbb,0xbf),Bytes(0,0,0xfe,0xff),Bytes(0xff,0xfe,0,0xd8,65),Bytes(0,0xd8,0,0xdc),Bytes(0,0xd8,65,0)})raw.Add(b);
  foreach(string text in new[]{"", "one", "\n", "\r", "\r\n", "a\r\nb\rc\nlast", "\n\r\n\r", "\0x\0", "\u0085\u2028\u2029", "Я🌍"})raw.Add(Encoding.UTF8.GetBytes(text));
  foreach(int n in new[]{1020,1021,1022,1023,1024,1025,2047})foreach(var ending in new[]{Bytes(13,10,65),Bytes(0xf0,0x9f,0x8c,0x8d,10),Bytes(0xed,0xa0,0x80),Bytes(0xff,0xfe,65,0)}){
   var prefix=new byte[n];for(int i=0;i<n;++i)prefix[i]=65;raw.Add(Join(prefix,ending));
  }
  var texts=new List<string>{null,"","x","one\r\ntwo\rthree\n","Я🌍","\0\uffff\ufeff","\ud800","\udfff","\ud800A","A\udfff","\ud800\ud800\udc00","\udfff\ud800","\ud800\udc00","\udbff\udfff"};
  foreach(int n in new[]{1022,1023,1024,1025,2047})foreach(string end in new[]{"\ud800","\ud800\udc00","\ud800A","\udfff"})texts.Add(new string('x',n)+end);
  int count=0;
  using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){
   output.WriteLine("# Independent CLR "+Environment.Version+" FilesOperations stream recipes; UTF-16 text hex, byte hex; read result newline canonicalized to LF");
   foreach(int code in new[]{1251,20127,65001,1200,1201,12000,12001}){
    var encoding=Encoding.GetEncoding(code);
    foreach(var bytes in raw){output.WriteLine("R\t"+code+"\t"+Hex(bytes)+"\t"+Text(Read(bytes,encoding)));++count;output.WriteLine("D\t"+code+"\t"+Hex(bytes)+"\t"+Text(encoding.GetString(bytes)));++count;}
    foreach(var mark in new[]{Bytes(0xef,0xbb,0xbf),Bytes(0xff,0xfe),Bytes(0xfe,0xff),Bytes(0xff,0xfe,0,0),Bytes(0,0,0xfe,0xff)})foreach(var bytes in new[]{new byte[0],Bytes(65),Bytes(65,0,13,0,10,0),Bytes(0xff),Bytes(0xf0,0x9f,0x8c,0x8d)}){
     var data=Join(mark,bytes);output.WriteLine("R\t"+code+"\t"+Hex(data)+"\t"+Text(Read(data,encoding)));++count;
    }
    foreach(string text in texts){output.WriteLine("W\t"+code+"\t"+Text(text)+"\t"+Hex(Write(text,encoding)));++count;}
   }
  }
  Console.WriteLine(count+" independent file stream outcomes");
  Sharing(Path.Combine(Path.GetDirectoryName(args[0]),"file-operations-sharing-windows.tsv"),args[1]);
 }
}
