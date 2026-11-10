// Development-only real CLR properties, accessors and class initialization.
using System;using System.IO;using System.Text;using System.Reflection;using System.Reflection.Emit;
public class StaticPropertiesReference {
 public static int Mode,Seed,Delta,Calls;public static Type Current;public static StringBuilder Output;
 public static int Init(){Output.Append('I');++Calls;if(Mode==10)Output.Append(Current.GetProperty("Const").GetValue(null,null));return Seed;}
 public static int Make(int n){Output.Append('F');return n;}
 public static int Index(int n){Output.Append('K');return n;}
 public class Model<T>{
  public static int X;static Model(){X=Init();}
  public static int P{get{Output.Append('G');return X;}set{Output.Append('S');X=value;}}
  public static int Field{get{return X;}set{X=value;}}
  public static int Calc{get{return X+7;}set{X=value-7;}}
  public static int Const{get{return 7;}}
  public static int WO{set{Output.Append('S');X=value;}}
  public static int GetItem(int i,int j){Output.Append('G');return X+i*3+j;}
  public static void SetItem(int i,int j,int v){Output.Append('S');X=v-i*3-j;}
  public static Func<int,int> Callback{get{Output.Append('G');return n=>X+n;}}
 }
 static int Get(string name){return (int)Current.GetProperty(name).GetValue(null,null);}
 static void Set(string name,int n){Current.GetProperty(name).SetValue(null,n,null);}
 static int Raw(){return (int)Current.GetField("X").GetValue(null);}
 static int Item(int i,int j){return (int)Current.GetMethod("GetItem").Invoke(null,new object[]{i,j});}
 static void Item(int i,int j,int n){Current.GetMethod("SetItem").Invoke(null,new object[]{i,j,n});}
 static Func<int,int> Callback(){return (Func<int,int>)Current.GetProperty("Callback").GetValue(null,null);}
 static string Run(){Output=new StringBuilder();Calls=0;Output.Append('b');
  if(Mode==0)Output.Append(Calls).Append('\n');
  else if(Mode==1)Output.Append(Get("P")).Append(' ').Append(Calls).Append('\n');
  else if(Mode==2){Set("P",Make(Delta));Output.Append(Raw()).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==3){int n=Get("P")+Make(Delta);Set("P",n);Output.Append(Raw()).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==4){Output.Append(Get("Field")).Append(' ');Output.Append(Get("Field")).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==5){Output.Append(Get("Calc")).Append(' ');Set("Calc",Make(Delta));Output.Append(Raw()).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==6)Output.Append(Item(Index(-3),Index(2))).Append(' ').Append(Calls).Append('\n');
  else if(Mode==7){Item(Index(-3),Index(2),Make(Delta));Output.Append(Raw()).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==8){int i=Index(-3),j=Index(2);int n=Item(i,j)+Make(Delta);Item(i,j,n);Output.Append(Raw()).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==9)Output.Append(Callback()(Make(Delta))).Append(' ').Append(Calls).Append('\n');
  else if(Mode==10)Output.Append(Get("Const")).Append(' ').Append(Calls).Append('\n');
  else if(Mode==11){Set("WO",Make(Delta));Output.Append(Raw()).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==12){var f=Callback();Output.Append(Calls);Output.Append(f(Make(Delta))).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==13)Output.Append(Item(Index(-3),Index(2))).Append(' ').Append(Calls).Append('\n');
  else if(Mode==14){Set("Calc",Make(Delta));Output.Append(Raw()).Append(' ').Append(Calls).Append('\n');}
  else {int n=Get("P");Output.Append('D').Append(n).Append(' ').Append(Calls).Append('\n');}
  return Output.ToString();
 }
 static string Units(string s){var b=new StringBuilder();foreach(char c in s)b.Append(((int)c).ToString("x4"));return b.ToString();}
 static void Main(string[] args){var a=AssemblyBuilder.DefineDynamicAssembly(new AssemblyName("StaticPropertyTags"),AssemblyBuilderAccess.Run);var m=a.DefineDynamicModule("tags");int rows=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# mode\tseed\tdelta\tstdout_utf16");for(int mode=0;mode<16;++mode)foreach(int seed in new[]{-11,5})foreach(int delta in new[]{-7,9}){Mode=mode;Seed=seed;Delta=delta;var tag=m.DefineType("Tag"+rows,TypeAttributes.Public).CreateType();Current=typeof(Model<>).MakeGenericType(tag);if((Current.Attributes&TypeAttributes.BeforeFieldInit)!=0)throw new Exception("Strict static constructor required");file.WriteLine(mode+"\t"+seed+"\t"+delta+"\t"+Units(Run()));++rows;}}Console.WriteLine(rows+" actual CLR static property/accessor executions");}
}
