// Development-only file/encoder-state oracle. No Pascal compiler.
using System;using System.IO;using System.Text;using System.Collections.Generic;using System.Reflection;
class IsciiFiles{
 static string Hex(byte[] data){return data.Length==0?"-":BitConverter.ToString(data).Replace("-","").ToLowerInvariant();}
 static string Hex(string text){if(text.Length==0)return "-";var b=new StringBuilder();foreach(char c in text)b.Append(((int)c).ToString("x4"));return b.ToString();}
 static readonly string Sample="कকகకಕകકକਕ ्\u200c्\u200d ॒॰ 🌍\r\n";
 static void Main(string[] args){
  if(args.Length>1){var provider=Type.GetType("System.Text.CodePagesEncodingProvider, System.Text.Encoding.CodePages");typeof(Encoding).GetMethod("RegisterProvider").Invoke(null,new[]{provider.GetProperty("Instance").GetValue(null,null)});}
  int rows=0;using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){output.WriteLine("# code_page\toperation\tinput_bytes_hex\toutput_hex");
   for(int cp=57002;cp<=57011;++cp){var e=Encoding.GetEncoding(cp);var inputs=new List<byte[]>();for(int n=0;n<256;++n)inputs.Add(new[]{(byte)n});var all=new byte[256];for(int n=0;n<256;++n)all[n]=(byte)n;inputs.Add(all);
    var pending=new List<byte>{0xe8,0xe9};for(int n=160;n<256;++n){var decoder=e.GetDecoder();var sink=new char[8];if(decoder.GetChars(new[]{(byte)n},0,1,sink,0,false)==0)pending.Add((byte)n);}
    foreach(byte a in pending)foreach(byte b in new byte[]{0,0x40,0x41,0x42,0xe8,0xe9,0xef,0xf0,0xb8,0xbf,0xff})inputs.Add(new[]{a,b});
    inputs.Add(e.GetBytes(Sample));
    for(int font=2;font<=11;++font){inputs.Add(new[]{(byte)0xef,(byte)(0x40+font),(byte)0xb3,(byte)0xe9,(byte)0xef,(byte)0x40,(byte)0xb3});
     for(int cut=1;cut<=2;++cut)foreach(byte tail in new byte[]{0xef,0xe8,0xe9,0xf0,0xb3}){var b=new List<byte>();for(int n=0;n<1024-cut;++n)b.Add((byte)'A');b.AddRange(new[]{(byte)0xef,(byte)(0x40+font),(byte)0xb3,(byte)0xe9,(byte)13,(byte)10,tail});inputs.Add(b.ToArray());}
    }
    foreach(var u in new[]{Encoding.UTF8,Encoding.Unicode,new UTF32Encoding(false,true)}){var b=new List<byte>(u.GetPreamble());b.AddRange(u.GetBytes(Sample));inputs.Add(b.ToArray());}
    foreach(var input in inputs)for(int op=1;op<=2;++op){string result;using(var reader=new StreamReader(new MemoryStream(input),e,true,1024)){if(op==1)result=reader.ReadToEnd();else{var b=new StringBuilder();string line;while((line=reader.ReadLine())!=null)b.Append(line).Append('\n');result=b.ToString();}}output.WriteLine(cp+"\t"+op+"\t"+Hex(input)+"\t"+Hex(result));++rows;}
    foreach(int op in new[]{0,3,4,5,6,7,8,9,10})using(var stream=new MemoryStream()){
     using(var writer=new StreamWriter(stream,e,1024,true)){
      if(op==0)writer.Write(Sample);
      else if(op==3)writer.Write(new string('A',1023)+"कকக"+new string('B',1024)+"्\u200d\ud83d");
      else if(op==4){writer.Write(new string('A',1023)+"कক\ud83d");writer.Write("\ude00க"+new string('B',1024)+"्\u200d\ud83d");}
      else if(op==5||op==6){writer.Write(op==5?e.GetString(new byte[]{0xe8}):cp==57002?"\u09cd":"\u094d");writer.Flush();writer.Write('\u200d');}
      else if(op==7)writer.Write('ক');
      else if(op==8){writer.Write('ক');writer.Write('க');}
      else {var glyph=cp==57003?'க':'ক';writer.Write(new string(glyph,1025));if(op==9){writer.BaseStream.Seek(0,SeekOrigin.Begin);writer.Write('क');}}
     }
     if(op==7)using(var writer=new StreamWriter(stream,e,1024,true))writer.Write('क');
     output.WriteLine(cp+"\t"+op+"\t-\t"+Hex(stream.ToArray()));++rows;
    }
   }
  }Console.WriteLine(rows+" ISCII file recipes");
 }
}
