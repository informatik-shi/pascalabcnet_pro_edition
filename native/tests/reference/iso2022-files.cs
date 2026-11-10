// Development-only StreamReader/StreamWriter reference. No Pascal compiler.
using System;using System.IO;using System.Text;using System.Reflection;using System.Collections.Generic;
class IsoFiles{
 static string Hex(byte[] b){return b.Length==0?"-":BitConverter.ToString(b).Replace("-","").ToLowerInvariant();}
 static string Hex(string s){if(s.Length==0)return "-";var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("x4"));return b.ToString();}
 static byte[] Cat(byte[] a,byte[] b){var r=new byte[a.Length+b.Length];Array.Copy(a,r,a.Length);Array.Copy(b,0,r,a.Length,b.Length);return r;}
 static IEnumerable<byte[]> Prefixes(int cp){var modes=cp==52936?new[]{new byte[0],new byte[]{126,123}}:cp==50225?new[]{new byte[0],new byte[]{14},new byte[]{27,36,41,67},new byte[]{27,36,41,67,14}}:new[]{new byte[0],new byte[]{27,40,73},new byte[]{27,36,66},new byte[]{14},new byte[]{27,36,66,14},new byte[]{27,38,64},new byte[]{27,36,40,68}};
  var pending=cp==52936?new[]{new byte[0],new byte[]{126}}:cp==50225?new[]{new byte[0],new byte[]{27},new byte[]{27,36},new byte[]{27,36,41},new byte[]{27,27}}:new[]{new byte[0],new byte[]{27},new byte[]{27,36},new byte[]{27,40},new byte[]{27,38},new byte[]{27,36,40},new byte[]{27,27}};
  foreach(var m in modes)foreach(var p in pending)yield return Cat(m,p);
 }
 static void Main(string[] args){if(args.Length>1){var t=Type.GetType("System.Text.CodePagesEncodingProvider, System.Text.Encoding.CodePages");typeof(Encoding).GetMethod("RegisterProvider").Invoke(null,new[]{t.GetProperty("Instance").GetValue(null,null)});}int rows=0;
  using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){output.WriteLine("# code_page\toperation\tinput_bytes_hex\toutput_hex");
   foreach(int cp in new[]{50220,50221,50222,50225,52936}){var e=Encoding.GetEncoding(cp);string glyph=cp==50225?"한":cp==52936?"中":"日";var inputs=new List<byte[]>();for(int n=0;n<256;++n)inputs.Add(new[]{(byte)n});var all=new byte[256];for(int n=0;n<256;++n)all[n]=(byte)n;inputs.Add(all);
    foreach(var p in Prefixes(cp)){inputs.Add(p);foreach(byte b in new byte[]{0,14,15,27,32,36,40,41,66,67,68,73,126,123,125,129,161,223,255})inputs.Add(Cat(p,new[]{b}));}
    var encoded=e.GetBytes(glyph+"ｶ"+glyph);for(int cut=1;cut<=4;++cut)foreach(var tail in new[]{new byte[]{27},new byte[]{27,36},new byte[]{27,36,40},new byte[]{126},new byte[]{14},new byte[]{27,36,66,49}}){var b=new List<byte>();for(int n=0;n<1024-cut;++n)b.Add((byte)'A');b.AddRange(encoded);b.Add(13);b.Add(10);b.AddRange(tail);inputs.Add(b.ToArray());}
    foreach(var u in new[]{Encoding.UTF8,Encoding.Unicode,new UTF32Encoding(false,true)})inputs.Add(Cat(u.GetPreamble(),u.GetBytes("日本語 한국 中文 🌍")));
    foreach(var input in inputs)for(int op=1;op<=2;++op){string text;using(var reader=new StreamReader(new MemoryStream(input),e,true,1024)){if(op==1)text=reader.ReadToEnd();else{var b=new StringBuilder();string line;while((line=reader.ReadLine())!=null)b.Append(line).Append('\n');text=b.ToString();}}output.WriteLine(cp+"\t"+op+"\t"+Hex(input)+"\t"+Hex(text));++rows;}
    foreach(int op in new[]{0,3,4,5,6,7,8,9,10})using(var stream=new MemoryStream()){
     using(var writer=new StreamWriter(stream,e,1024,true)){
      if(op==0)writer.Write("A\r\n日本語 ｶﾀｶﾅ 中文 한국어 éＡ🌍\0Z");
      else if(op==3)writer.Write(new string('A',1023)+glyph+glyph+new string('B',1024)+"ｶ\ud83d");
      else if(op==4){writer.Write(new string('A',1023)+glyph+"\ud83d");writer.Write("\ude00"+glyph+new string('B',1024)+"ｶ\ud83d");}
      else if(op==5){writer.Write(glyph);writer.Flush();writer.Write(glyph);}
      else if(op==6){writer.Write(glyph+"ｶ");writer.Flush();writer.Write(glyph+"ｶ");}
      else if(op==7)writer.Write(glyph);
      else if(op==8){writer.Write(glyph);writer.Write('A');writer.Write(glyph);}
      else{writer.Write(new string(glyph[0],1025));if(op==9){writer.BaseStream.Seek(0,SeekOrigin.Begin);writer.Write(glyph);}}
     }
     if(op==7)using(var writer=new StreamWriter(stream,e,1024,true))writer.Write(glyph);
     output.WriteLine(cp+"\t"+op+"\t-\t"+Hex(stream.ToArray()));++rows;
    }
   }
  }Console.WriteLine(rows+" ISO-2022/HZ file recipes");
 }
}
