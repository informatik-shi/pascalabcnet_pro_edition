// Development-only actual CLR class initialization, using fresh closed types.
using System;using System.IO;using System.Text;using System.Reflection;using System.Reflection.Emit;
public class StaticFieldsReference{
 public static int Mode,Seed,Delta,Count,Calls;public static FieldInfo CurrentY;public static StringBuilder Output;
 public static int Init(){Output.Append('I');++Calls;if(Mode==8)CurrentY.SetValue(null,Delta);return Seed;}
 public static int Make(int value){Output.Append('F');return value;}
 public static int Element(int[] data,int n){if(Mode==9)Output.Append('E').Append(data[0]);return n*11-17;}
 public class Model<T>{public static int X;public static int Y;public static int[] V;static Model(){X=Init();V=new int[Count];for(int n=0;n<V.Length;++n)V[n]=Element(V,n);}public Model(int value){Output.Append('C');}public int Get(){return X;}}
 static string Run(Type type,int lower){Output=new StringBuilder();Calls=0;var x=type.GetField("X");CurrentY=type.GetField("Y");var v=type.GetField("V");Output.Append('b');
  if(Mode==0)Output.Append(Calls).Append('\n');
  else if(Mode==1)Output.Append(x.GetValue(null)).Append(' ').Append(x.GetValue(null)).Append(' ').Append(Calls).Append('\n');
  else if(Mode==2){x.SetValue(null,Make(Delta));Output.Append(x.GetValue(null)).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==3){Activator.CreateInstance(type,new object[]{Make(Delta)});Output.Append(x.GetValue(null)).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==4){x.GetValue(null);int value=Make(Delta);x.SetValue(null,value);Output.Append(x.GetValue(null)).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==5){var instance=Activator.CreateInstance(type,new object[]{0});Output.Append(type.GetMethod("Get").Invoke(instance,null)).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==6){Output.Append(lower).Append(' ').Append(lower+Count-1).Append(' ').Append(Calls).Append('\n');var data=(int[])v.GetValue(null);Output.Append(data[0]).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==7){var copy=(int[])((int[])v.GetValue(null)).Clone();copy[0]=Delta;Output.Append(((int[])v.GetValue(null))[0]).Append(' ').Append(copy[0]).Append(' ').Append(Calls).Append('\n');}
  else if(Mode==8)Output.Append(x.GetValue(null)).Append(' ').Append(CurrentY.GetValue(null)).Append(' ').Append(Calls).Append('\n');
  else {var data=(int[])v.GetValue(null);Output.Append(data[0]).Append(' ').Append(Calls).Append('\n');}
  return Output.ToString();}
 static string Units(string value){var text=new StringBuilder();foreach(char c in value)text.Append(((int)c).ToString("x4"));return text.ToString();}
 static void Main(string[] args){var assembly=AssemblyBuilder.DefineDynamicAssembly(new AssemblyName("StaticFieldReferenceTags"),AssemblyBuilderAccess.Run);var module=assembly.DefineDynamicModule("tags");int rows=0;using(var file=new StreamWriter(args[0],false,new UTF8Encoding(false))){file.WriteLine("# lower\tcount\tmode\tseed\tdelta\tstdout_utf16");foreach(int lower in new[]{-3,2})foreach(int count in new[]{1,3})foreach(int mode in new[]{0,1,2,3,4,5,6,7,8,9})foreach(int seed in new[]{-11,5})foreach(int delta in new[]{-7,9}){Mode=mode;Seed=seed;Delta=delta;Count=count;var tag=module.DefineType("Tag"+rows,TypeAttributes.Public).CreateType();var type=typeof(Model<>).MakeGenericType(tag);if((type.Attributes&TypeAttributes.BeforeFieldInit)!=0)throw new Exception("Reference must use strict class initialization");file.WriteLine(lower+"\t"+count+"\t"+mode+"\t"+seed+"\t"+delta+"\t"+Units(Run(type,lower)));++rows;}}Console.WriteLine(rows+" actual strict CLR class initialization executions");}
}
