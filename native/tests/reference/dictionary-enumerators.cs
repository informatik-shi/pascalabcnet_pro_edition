using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
public static class EnumeratorReference {
 static Dictionary<int,string> d=new Dictionary<int,string>();
 static Dictionary<int,string>.Enumerator[] p=new Dictionary<int,string>.Enumerator[4];
 static Dictionary<int,string>.KeyCollection.Enumerator[] k=new Dictionary<int,string>.KeyCollection.Enumerator[4];
 static Dictionary<int,string>.ValueCollection.Enumerator[] v=new Dictionary<int,string>.ValueCollection.Enumerator[4];
 static StreamWriter writer;
 static string Text(string value){return value ?? "~";}
 static string Try(Func<string> action){try{return action();}catch(InvalidOperationException){return "E_MODIFIED";}catch(NullReferenceException){return "E_NIL";}catch(ArgumentException){return "E_KEY_DUPLICATE";}}
 static void Emit(string op,int slot=0,int arg=0,string val="x"){
  string result;
  if(op=="new"){p[slot]=d.GetEnumerator();k[slot]=d.Keys.GetEnumerator();v[slot]=d.Values.GetEnumerator();result="ok";}
  else if(op=="copy"){p[slot]=p[arg];k[slot]=k[arg];v[slot]=v[arg];result="ok";}
  else if(op=="default"){p[slot]=default(Dictionary<int,string>.Enumerator);k[slot]=default(Dictionary<int,string>.KeyCollection.Enumerator);v[slot]=default(Dictionary<int,string>.ValueCollection.Enumerator);result="ok";}
  else if(op=="current")result=Try(()=>p[slot].Current.Key+":"+Text(p[slot].Current.Value))+"|"+Try(()=>k[slot].Current.ToString())+"|"+Try(()=>Text(v[slot].Current));
  else if(op=="next")result=Try(()=>p[slot].MoveNext().ToString())+"|"+Try(()=>k[slot].MoveNext().ToString())+"|"+Try(()=>v[slot].MoveNext().ToString());
  else if(op=="dispose"){p[slot].Dispose();k[slot].Dispose();v[slot].Dispose();result="ok|ok|ok";}
  else result=Try(()=>{if(op=="clear")d.Clear();else if(op=="remove")return d.Remove(arg).ToString();else if(op=="add")d.Add(arg,val);else d[arg]=val;return "ok";});
  writer.WriteLine(op+"\t"+slot+"\t"+arg+"\t"+val+"\t"+result);
 }
 public static void Generate(string output){
  using(writer=new StreamWriter(output,false,new UTF8Encoding(false))){
   writer.WriteLine("# .NET Framework CLR "+Environment.Version+"; independent typed dictionary/key/value struct enumerators; seed 7931");
   Random random=new Random(7931);
   for(int block=0;block<96;++block){
    Emit("clear");for(int j=0;j<4;++j)Emit("add",0,j,"v"+block+"_"+j);Emit("remove",0,1);
    Emit("new",0);Emit("current",0);Emit("next",0);Emit("copy",1,0);Emit("next",0);Emit("current",1);Emit("next",1);
    Emit("dispose",0);Emit("current",0);Emit("next",0);Emit("next",0);Emit("current",0);Emit("next",0);
    Emit("set",0,0,"changed"+block);Emit("current",1);Emit("next",1);Emit("dispose",1);Emit("current",1);
    Emit("new",2);for(int j=0;j<5;++j){Emit("next",2);Emit("current",2);}
    Emit("default",3);Emit("current",3);Emit("next",3);Emit("dispose",3);Emit("current",3);
    Emit("new",3);Emit("remove",0,99);Emit("next",3);Emit("add",0,0,"duplicate");Emit("next",3);
    Emit("clear");Emit("new",0);Emit("clear");Emit("next",0);Emit("current",0);
    for(int j=0;j<24;++j){
     int choice=random.Next(10),slot=random.Next(4),arg=random.Next(-8,9);string value="r"+random.Next(1000);
     if(choice==0)Emit("new",slot);else if(choice==1)Emit("copy",slot,random.Next(4));else if(choice==2)Emit("default",slot);
     else if(choice<=4)Emit("next",slot);else if(choice==5)Emit("current",slot);else if(choice==6)Emit("dispose",slot);
     else if(choice==7)Emit("add",0,arg,value);else if(choice==8)Emit("remove",0,arg);else Emit("set",0,arg,value);
    }
   }
  }
 }
}
