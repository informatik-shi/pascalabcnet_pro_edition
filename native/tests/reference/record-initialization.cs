// Development-only lowering oracle. C# 4 has no struct field initializers:
// Init recipes below explicitly model the inspected Pascal NETGenerator paths.
// The Framework supplies struct copies, shared references, Array.Resize,
// generic defaults, List/Dictionary consumers and exception propagation.
using System;using System.IO;using System.Text;using System.Collections.Generic;
using System.Linq;using System.Globalization;
class RecordInitializationReference{
 struct Leaf{public int X;public int[] Data;}
 struct Outer{public int A;public Leaf Child;public int B;}
 struct Special{public int X;}
 class Holder{public Outer R;}
 interface IFactory{Outer Make();}
 struct Factory:IFactory{Outer IFactory.Make(){var result=default(Outer);Init(ref result);Console.Write('E');result=NewOuter();return result;}}
 static int counter,bias,failAt;static bool watch,watchMatrix;static Leaf[] saved;static Leaf[,] savedMatrix;
 static Outer globalRecord;static Special globalSpecial;
 static int Step(char label){++counter;Console.Write(label);if(watch)Console.Write("("+saved.Length+")");if(watchMatrix)Console.Write("("+savedMatrix.Length+")");if(counter==failAt)throw new DivideByZeroException();return bias+counter;}
 static void Init(ref Leaf value){value.X=Step('L');value.Data=new[]{Step('D')};}
 static void Init(ref Outer value){value.A=Step('A');Init(ref value.Child);value.B=Step('B');}
 static void Init(ref Special value){value.X=Step('S');}
 static Leaf NewLeaf(){var value=default(Leaf);Init(ref value);return value;}
 static Leaf NewLeaf(int n){var value=NewLeaf();Console.Write('C');value.X=n;return value;}
 static Outer NewOuter(){var value=default(Outer);Init(ref value);return value;}
 static Outer NewOuter(int n){var value=NewOuter();Console.Write('O');value.Child=NewLeaf(n);value.A=n;return value;}
 static Special NewSpecial(){var value=default(Special);Init(ref value);Console.Write('Z');value.X+=10;return value;}
 static int Argument(){Console.Write('N');return bias;}
 static Outer Echo(Outer value){var result=default(Outer);Init(ref result);Console.Write('E');result=value;return result;}
 static Leaf MakeLeaf(){var result=default(Leaf);Init(ref result);Console.Write('E');result=NewLeaf(bias);return result;}
 static Outer MakeOuter(){var result=default(Outer);Init(ref result);Console.Write('E');result=NewOuter();return result;}
 static Leaf[] NewArray(int n){var values=new Leaf[n];for(int i=0;i<n;++i)Init(ref values[i]);return values;}
 static Leaf[,] NewMatrix(int rows,int columns){var values=new Leaf[rows,columns];for(int r=0;r<rows;++r)for(int c=0;c<columns;++c)Init(ref values[r,c]);return values;}
 static void Resize(ref Leaf[] values,int n){int old=values==null?0:values.Length;Array.Resize(ref values,n);for(int i=old;i<n;++i)Init(ref values[i]);}
 static void ResizeMatrix(ref Leaf[,] values,int rows,int columns){var result=NewMatrix(rows,columns);if(values!=null)for(int r=0;r<Math.Min(rows,values.GetLength(0));++r)for(int c=0;c<Math.Min(columns,values.GetLength(1));++c)result[r,c]=values[r,c];values=result;}
 static void DumpMatrix(Leaf[,] values){Console.Write(values.Length+":");for(int r=0;r<values.GetLength(0);++r)for(int c=0;c<values.GetLength(1);++c)Console.Write(values[r,c].X+",");Console.WriteLine(" "+counter);}
 static void Show(Leaf value){Console.Write(value.X+" ");if(value.Data==null)Console.Write('-');else Console.Write(value.Data[0]);Console.WriteLine(" "+counter);}
 static void ShowOuter(Outer value){Console.Write(value.A+" "+value.B+" ");Show(value.Child);}
 static void Dump(Leaf[] values){Console.Write(values.Length+":");foreach(var item in values)Console.Write(item.X+",");Console.WriteLine(" "+counter);}
 static void Run(int b,int length,int failure,int op){
  counter=bias=0;failAt=-1;watch=watchMatrix=false;saved=null;savedMatrix=null;globalRecord=NewOuter();globalSpecial=default(Special);Init(ref globalSpecial);
  bias=b;failAt=failure;counter=0;Console.Write('|');
  switch(op){
   case 0:Show(NewLeaf());break;
   case 1:Show(NewLeaf());break;
   case 2:Show(NewLeaf(Argument()));break;
   case 3:{var value=NewLeaf();var copy=value;copy.X+=10;copy.Data[0]+=20;Show(value);Show(copy);break;}
   case 4:ShowOuter(NewOuter());break;
   case 5:ShowOuter(NewOuter(bias));break;
   case 6:{var value=new Holder();Init(ref value.R);ShowOuter(value.R);break;}
   case 7:Dump(NewArray(length));break;
   case 8:{var values=NewArray(1);Resize(ref values,length);Dump(values);break;}
   case 9:{var values=NewMatrix(2,length);Console.Write(values.Length+":");for(int r=0;r<2;++r)for(int c=0;c<length;++c)Console.Write(values[r,c].X+",");Console.WriteLine(" "+counter);break;}
   case 10:Resize(ref saved,length);Dump(saved);Resize(ref saved,length+1);Dump(saved);break;
   case 11:Show(default(Leaf));break;
   case 12:{var values=new List<Leaf>(length);Console.WriteLine(values.Count+" "+values.Capacity+" "+counter);break;}
   case 13:Show(new Leaf[0].AsEnumerable().FirstOrDefault());break;
   case 14:{var value=NewLeaf();var values=new Dictionary<int,Leaf>();Console.WriteLine(values.TryGetValue(0,out value));Show(value);break;}
   case 15:{var value=NewOuter();var copy=Echo(value);ShowOuter(copy);ShowOuter(value);break;}
   case 16:Show(MakeLeaf());break;
   case 17:{var value=NewLeaf();var values=new[]{value,value};values[0].X+=10;values[0].Data[0]+=20;Show(values[0]);Show(values[1]);Show(value);break;}
   case 18:{var value=new Holder();value.R=MakeOuter();ShowOuter(value.R);break;}
   case 19:for(int i=1;i<=length;++i)Show(NewLeaf());Console.WriteLine(counter);break;
   case 20:{var value=NewSpecial();Console.WriteLine(value.X+" "+counter);break;}
   case 21:{var value=default(Special);Console.WriteLine(value.X+" "+globalSpecial.X+" "+counter);break;}
   case 22:watch=true;Resize(ref saved,length);Dump(saved);break;
   case 23:watch=true;saved=NewArray(length);Dump(saved);break;
   case 24:{var values=NewMatrix(1,1);ResizeMatrix(ref values,2,length);DumpMatrix(values);break;}
   case 25:savedMatrix=NewMatrix(1,1);watchMatrix=true;ResizeMatrix(ref savedMatrix,2,length);DumpMatrix(savedMatrix);break;
   case 26:{IFactory factory=new Factory();ShowOuter(factory.Make());break;}
  }
 }
 static string Hex(string text){if(text.Length==0)return "-";var b=new StringBuilder();foreach(char c in text.Replace("\r\n","\n"))b.Append(((int)c).ToString("x4"));return b.ToString();}
 static void Main(string[] args){
  CultureInfo.CurrentCulture=CultureInfo.InvariantCulture;var console=Console.Out;
  using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){
   output.WriteLine("# bias\tlength\tfailAt\toperation\tstdout_utf16_hex\terror");
   foreach(int b in new[]{-2,0,3})foreach(int length in new[]{0,1,3})foreach(int failure in new[]{-1,1,2,4,5})for(int op=0;op<27;++op){
    var capture=new StringWriter(CultureInfo.InvariantCulture);Console.SetOut(capture);string error="ok";
    try{Run(b,length,failure,op);}catch(DivideByZeroException){error="E_DIV_ZERO";}catch(NullReferenceException){error="E_NIL";}finally{Console.SetOut(console);}
    output.WriteLine(b+"\t"+length+"\t"+failure+"\t"+op+"\t"+Hex(capture.ToString())+"\t"+error);
   }
  }
 }
}
