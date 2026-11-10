// Development-only independent CLR closure execution; no Pascal compiler.
using System;using System.IO;using System.Text;using System.Globalization;
class NestedRoutineReference{
 static Func<int> Counter(int seed,int step){int value=seed;return delegate{value+=step;return value;};}
 static int Fold(int seed){int value=seed;Func<int,int> recurse=null;recurse=delegate(int n){Func<int> adjust=delegate{++value;return n+value;};return n==0?adjust():recurse(n-1)+adjust();};return recurse(4);}
 static int Factorial(int number){Func<int,int> product=null;product=n=>n<=1?1:n*product(n-1);return product(number);}
 static string Hex(string s){var r=new StringBuilder();foreach(char c in s)r.Append(((int)c).ToString("x4"));return r.ToString();}
 static void Main(string[] args){int rows=0;using(var log=new StreamWriter(args[0],false,new UTF8Encoding(false))){log.WriteLine("# seed\tstep\tcount\tstdout_utf16_units");for(int seed=-8;seed<=8;++seed)for(int step=-3;step<=3;++step)for(int count=0;count<=8;++count){var output=new StringBuilder();var next=Counter(seed,step);var alias=next;var separate=Counter(-seed,step+1);for(int i=1;i<=count;++i)output.Append(next().ToString(CultureInfo.InvariantCulture)).Append(' ').Append(alias().ToString(CultureInfo.InvariantCulture)).Append(' ').Append(separate().ToString(CultureInfo.InvariantCulture)).Append('\n');output.Append(Fold(seed).ToString(CultureInfo.InvariantCulture)).Append(' ').Append(Factorial(count).ToString(CultureInfo.InvariantCulture)).Append('\n');log.WriteLine(seed+"\t"+step+"\t"+count+"\t"+Hex(output.ToString()));++rows;}}Console.WriteLine(rows+" actual nested closure executions");}
}
