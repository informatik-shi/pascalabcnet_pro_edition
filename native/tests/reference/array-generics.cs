// Independent closed generic array recipes; development-only, never installed.
using System;using System.IO;using System.Text;using System.Globalization;
[assembly:System.Runtime.Versioning.TargetFramework(".NETCoreApp,Version=v10.0")]
class Reference {
 static string Text(object value){return value==null?"nil":value.ToString();}
 static string Hex(string text){var result=new StringBuilder();foreach(char c in text)result.Append(((int)c).ToString("x4"));return result.Length==0?"-":result.ToString();}
 static void Fill<T>(T[] a,T value){for(int i=0;i<a.Length;++i)a[i]=value;}
 static void Fill<T>(T[,] a,T value){for(int i=0;i<a.GetLength(0);++i)for(int j=0;j<a.GetLength(1);++j)a[i,j]=value;}
 static void Write<T>(StringBuilder output,T[] a,string delimiter,bool newline){if(a!=null)for(int i=0;i<a.Length;++i){if(i>0)output.Append(Text(delimiter));output.Append(Text(a[i]));}if(newline)output.Append('\n');}
 static void Matrix<T>(StringBuilder output,T[,] a,int width){for(int i=0;i<a.GetLength(0);++i){for(int j=0;j<a.GetLength(1);++j)output.Append(Text(a[i,j]).PadLeft(width)).Append(' ');output.Append('\n');}}
 static string Run(int op,int n,int nil,int width,out string code){
  int[] a=new int[n];string[] words=new string[n];double[,] m=new double[n,2];int[,] integers=new int[n,2];var values=new[]{"b","a","c","A",null,"x"};
  for(int i=0;i<n;++i){a[i]=n-i-2;words[i]=values[i%6];for(int j=0;j<2;++j){m[i,j]=1.5+i+j/4.0;integers[i,j]=i*10+j;}}
  if(nil==1){a=null;words=null;m=null;integers=null;}
  var output=new StringBuilder();code="ok";string delimiter=width==5?null:"|";
  try{
   switch(op){
    case 0:Fill<int>(a,7);Write<int>(output,a,"|",false);break;
    case 1:Fill<double>(m,1.5);Matrix<double>(output,m,0);break;
    case 2:Write<int>(output,a,delimiter,false);break;
    case 3:Write<string>(output,words,delimiter,true);break;
    case 4:Array.Sort<string>(words);Write<string>(output,words,"|",true);break;
    case 5:Array.Sort<int>(a);Array.Reverse(a);Write<int>(output,a,"|",true);break;
    case 6:Matrix<double>(output,m,width);break;
    case 7:Matrix<int>(output,integers,width);break;
   }
  }catch(NullReferenceException){code="E_NIL";}catch(ArgumentNullException){code="E_NIL";}catch(ArgumentOutOfRangeException){code="E_RANGE";}
  return output.ToString();
 }
 public static void Main(string[] args){
  System.Threading.Thread.CurrentThread.CurrentCulture=CultureInfo.GetCultureInfo("en-US");int rows=0;
  using(var output=new StreamWriter(args[0],false,new UTF8Encoding(false))){
   output.WriteLine("# Independent .NET "+Environment.Version+" closed generic array recipes; operation,size,nil,call form,width,UTF-16 output hex,error");
   for(int op=0;op<8;++op)foreach(int size in new[]{0,1,2,5,7})for(int nil=0;nil<2;++nil)for(int form=0;form<3;++form)foreach(int width in new[]{0,5,-1}){string code;var value=Run(op,size,nil,width,out code);output.WriteLine(op+"\t"+size+"\t"+nil+"\t"+form+"\t"+width+"\t"+Hex(value)+"\t"+code);++rows;}
  }
  Console.WriteLine(rows+" independent generic array outcomes");
 }
}
