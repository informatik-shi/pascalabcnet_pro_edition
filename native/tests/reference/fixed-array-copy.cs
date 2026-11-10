// Development-only source-derived wrapper/record methods and public binary APIs.
using System;using System.IO;using System.Text;using System.Globalization;
class FixedCopyReference{
 static int calls;static StringBuilder output;
 static int Bump(){++calls;output.Append('i');return calls;}
 sealed class Inner{public byte[] data=new byte[2];public Inner Clone(){var r=new Inner();Array.Copy(data,r.data,2);return r;}}
 struct Item{public int n;public Inner inner;public static Item Init(){return new Item{n=Bump(),inner=new Inner()};}public Item Clone(){var r=Init();r.n=n;r.inner=inner.Clone();return r;}}
 sealed class Items{public Item[] data;public Items(int count){data=new Item[count];for(int i=0;i<count;++i)data[i]=Item.Init();}public Items Clone(){var r=new Items(data.Length);for(int i=0;i<data.Length;++i)r.data[i]=data[i].Clone();return r;}}
 static void ByValue(Items a){output.Append('V').Append(calls).Append('\n');}static void ByConst(Items a){output.Append('C').Append(calls).Append('\n');}

 static Items Read(BinaryReader reader,int count){var r=new Items(count);for(int i=0;i<count;++i){Item item=default(Item);item.n=reader.ReadInt32();item.inner=new Inner();item.inner.data=reader.ReadBytes(2);if(item.inner.data.Length!=2)throw new EndOfStreamException();r.data[i]=item;}return r;}
 static string Hex(byte[] bytes){return BitConverter.ToString(bytes).Replace("-","").ToLowerInvariant();}static string Units(string s){var r=new StringBuilder();foreach(char c in s)r.Append(((int)c).ToString("x4"));return r.ToString();}
 static void Main(string[] args){int rows=0;using(var log=new StreamWriter(args[0],false,new UTF8Encoding(false))){log.WriteLine("# lower\tcount\tmode\tstdout_utf16\tbytes\toutcome");foreach(int lower in new[]{-3,0,7})foreach(int count in new[]{1,2,4})for(int mode=0;mode<2;++mode){calls=0;output=new StringBuilder();string error="ok";byte[] bytes=null;try{var a=new Items(count);for(int i=0;i<count;++i){a.data[i].n=(lower+i)*11+13;a.data[i].inner.data[0]=(byte)(i+7);}output.Append('A').Append(calls).Append('\n');var b=a.Clone();b.data[0].n=999;b.data[0].inner.data[0]=88;output.Append('B').Append(calls).Append(':').Append(a.data[0].n).Append(',').Append(a.data[0].inner.data[0]).Append('\n');ByValue(a.Clone());ByConst(a);
 // RunTimeSizeOf probes the array wrapper constructor before inspecting its length.
 var probe=new Items(count);GC.KeepAlive(probe);output.Append('F').Append(calls).Append('\n');
 using(var memory=new MemoryStream()){var writer=new BinaryWriter(memory,Encoding.ASCII,true);foreach(var item in a.data){writer.Write(item.n);writer.Write(item.inner.data);}bytes=memory.ToArray();memory.Position=0;var reader=new BinaryReader(memory,Encoding.ASCII,true);var r=new Items(count);r=Read(reader,count).Clone();output.Append('R').Append(calls).Append(':').Append(r.data[0].n).Append(',').Append(r.data[0].inner.data[0]).Append('\n');if(mode==1)r=Read(reader,count).Clone();}}
 catch(EndOfStreamException){error="E_EOF";}log.WriteLine(lower+"\t"+count+"\t"+mode+"\t"+Units(output.ToString())+"\t"+Hex(bytes)+"\t"+error);++rows;}}Console.WriteLine(rows+" actual wrapper/record copy executions");}
}
