using System;
using System.IO;
using System.Text;
public static class UserInterfaceReference {
 interface IValue { int Value {get;set;} int Add(int a); void Change(ref int x); int Sum(params int[] xs); }
 interface IDouble:IValue { int Twice(); }
 class A:IDouble { int n; public int Value {get{return n;}set{n=value;}} public int Add(int a){return n+=a;}public void Change(ref int x){x+=n;}public int Sum(params int[] xs){int r=n;foreach(int x in xs)r+=x;return r;}public int Twice(){return n*2;} }
 class B:IValue { int n;public int Value {get{return n;}set{n=value;}}public int Add(int a){return n-=a;}public void Change(ref int x){x-=n;}public int Sum(params int[] xs){int r=n;foreach(int x in xs)r-=x;return r;} }
 public static void Generate(string output){
  using(var writer=new StreamWriter(output,false,new UTF8Encoding(false))){
   writer.WriteLine("# Independent .NET Framework CLR "+Environment.Version+" named interfaces/classes/properties/ref/params/cross casts; Random(2671)");
   var random=new Random(2671);
   for(int block=0;block<24;++block){
    IValue[] slots={new A(),new B(),null,null};
    for(int step=0;step<48;++step){
     string[] ops={"get","set","add","change","sum","alias","identity","cast","newa","newb","nil"};string op=step<8 ? new string[]{"set","get","alias","add","identity","cast","cast","change"}[step] : ops[random.Next(ops.Length)];
     int slot=step<8 ? new int[]{0,0,2,2,2,0,1,0}[step] : random.Next(4);int arg=op=="alias" || op=="identity" ? (step==2 || step==4 ? 0 : random.Next(4)) : random.Next(-4,8);string result;
     try{switch(op){case "get":result=slots[slot].Value.ToString();break;case "set":slots[slot].Value=arg;result="ok";break;case "add":result=slots[slot].Add(arg).ToString();break;case "change":int x=arg;slots[slot].Change(ref x);result=x.ToString();break;case "sum":result=slots[slot].Sum(1,arg,2).ToString();break;case "alias":slots[slot]=slots[arg];result="ok";break;case "identity":result=Object.ReferenceEquals(slots[slot],slots[arg]).ToString();break;case "cast":result=((IDouble)slots[slot]).Twice().ToString();break;case "newa":slots[slot]=new A();result="ok";break;case "newb":slots[slot]=new B();result="ok";break;default:slots[slot]=null;result="ok";break;}}
     catch(NullReferenceException){result="E_NIL";}catch(InvalidCastException){result="E_CAST";}
     writer.WriteLine(block+"\t"+op+"\t"+slot+"\t"+arg+"\t"+result);
    }
   }
  }
 }
}
