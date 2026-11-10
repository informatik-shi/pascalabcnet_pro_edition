// Development-only CLR static calls and delegates; never shipped.
using System;using System.IO;using System.Text;using System.Reflection;using System.Reflection.Emit;
public class StaticMethodsReference {
 public static int Mode,Seed,Delta,Calls;public static Type Current;public static StringBuilder Output;
 public static int Init(){Output.Append('I');++Calls;if(Mode==9)Current.GetMethod("Use").Invoke(null,new object[]{Delta});return Seed;}
 public static int Make(int n){Output.Append('F');return n;}
 public class Model<T>{
  public static int X;static Model(){X=Init();}
  public static int Use(int v){Output.Append('M');X+=v;return X;}
  public static void Ref(ref int v){Output.Append('R');v+=X;++X;}
  public int Get(){return Use(Delta);}
  public static int Rec(int n){Output.Append('Q');return n==0?X:Rec(n-1)+n;}
  public static int Take(Func<int,int> f,int v){Output.Append('T');return f(v);}
  public static int Default(int v){Output.Append('D');return X+v;}
  public static int Sum(params int[] values){Output.Append('P');int sum=X;foreach(int n in values)sum+=n;return sum;}
  public static int Pick(int n){Output.Append('A');return X+n;}
  public static int Pick(string s){Output.Append('B');return X+s.Length;}
  public static Func<int,int> Factory(int n){Output.Append('G');return z=>X+n+z;}
 }
 static object Call(string name,params object[] args){return Current.GetMethod(name).Invoke(null,args);}
 static Func<int,int> Target(string name){return (Func<int,int>)Delegate.CreateDelegate(typeof(Func<int,int>),Current.GetMethod(name,new[]{typeof(int)}));}
 static string Run(){Output=new StringBuilder();Calls=0;Output.Append('b');
  if(Mode==0){var f=Target("Use");Output.Append(Calls).Append('\n');GC.KeepAlive(f);}
  else if(Mode==1)Output.Append(Call("Use",Make(Delta))).Append(' ').Append(Calls).Append('\n');
  else if(Mode==2){var f=Target("Use");Output.Append(Calls);Output.Append(f(Make(Delta))).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==3){var f=Target("Use");Output.Append(f(Make(Delta))).Append(' ');Output.Append(f(Make(Delta))).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==4){object[] a={Make(Delta)};Call("Ref",a);Output.Append(a[0]).Append(' ').Append(Current.GetField("X").GetValue(null)).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==5){var o=Activator.CreateInstance(Current);Output.Append(Current.GetMethod("Get").Invoke(o,null)).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==6){var f=(Func<int,int>)Call("Factory",Make(Delta));Output.Append(f(3)).Append(' ').Append(f(5)).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==7)Output.Append(Call("Rec",Make(3))).Append(' ').Append(Calls).Append('\n');
  else if(Mode==8){var f=Target("Use");Output.Append(Call("Take",f,Make(Delta))).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==9)Output.Append(Call("Use",Make(Delta))).Append(' ').Append(Calls).Append('\n');
  else if(Mode==10)Output.Append(Calls).Append('\n');
  else if(Mode==11)Output.Append(Call("Default",Make(7))).Append(' ').Append(Calls).Append('\n');
  else if(Mode==12){Output.Append(Call("Sum",new object[]{new[]{Make(Delta),Make(3)}})).Append(' ');Output.Append(Call("Sum",new object[]{new int[0]})).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==13){Output.Append(Current.GetMethod("Pick",new[]{typeof(int)}).Invoke(null,new object[]{Make(Delta)})).Append(' ');Output.Append(Current.GetMethod("Pick",new[]{typeof(string)}).Invoke(null,new object[]{"xy"})).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==14){var f=Target("Pick");Output.Append(Calls);Output.Append(f(Make(Delta))).Append(' ').Append(Calls).Append('\n');}
  else {var f=(Func<int,int>)Call("Factory",Make(Delta));Output.Append(f(Make(3))).Append(' ').Append(Calls).Append('\n');}
  return Output.ToString();
 }
 static string Units(string s){var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("x4"));return b.ToString();}
 static void Main(string[] args){var a=AssemblyBuilder.DefineDynamicAssembly(new AssemblyName("StaticMethodTags"),AssemblyBuilderAccess.Run);var m=a.DefineDynamicModule("tags");int rows=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# mode\tseed\tdelta\tstdout_utf16");foreach(int mode in new[]{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15})foreach(int seed in new[]{-11,5})foreach(int delta in new[]{-7,9}){Mode=mode;Seed=seed;Delta=delta;var tag=m.DefineType("Tag"+rows,TypeAttributes.Public).CreateType();Current=typeof(Model<>).MakeGenericType(tag);if((Current.Attributes&TypeAttributes.BeforeFieldInit)!=0)throw new Exception("Strict static constructor required");file.WriteLine(mode+"\t"+seed+"\t"+delta+"\t"+Units(Run()));++rows;}}Console.WriteLine(rows+" actual CLR static method/delegate executions");}
}
