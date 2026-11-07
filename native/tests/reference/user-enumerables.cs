using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
public static class UserEnumerableReference {
 static StringBuilder log;
 static void W(object value){log.Append("=").Append(Convert.ToString(value,CultureInfo.InvariantCulture)).Append(';');}
 static void A(IEnumerable<int> source){W(String.Join(",",source));}
 sealed class Source:IEnumerable<int> {
  readonly string id;readonly int count,flags;
  public Source(string tag,int n,int mode){id=tag;count=n;flags=mode;}
  public IEnumerator<int> GetEnumerator(){log.Append("G").Append(id).Append(';');if((flags&1)!=0)throw new DivideByZeroException();return new Cursor(id,count,flags);}
  IEnumerator IEnumerable.GetEnumerator(){return GetEnumerator();}
 }
 sealed class Cursor:IEnumerator<int> {
  readonly string id;readonly int count,flags;int n;
  public Cursor(string tag,int limit,int mode){id=tag;count=limit;flags=mode;}
  public bool MoveNext(){log.Append("M").Append(id).Append('(').Append(n).Append(");");if((flags&2)!=0 && n==1)throw new DivideByZeroException();++n;return n<=count;}
  public int Current{get{log.Append("C").Append(id).Append('(').Append(n).Append(");");if((flags&4)!=0 && n==1)throw new DivideByZeroException();return n;}}
  object IEnumerator.Current{get{return Current;}}
  public void Dispose(){log.Append("D").Append(id).Append(';');if((flags&8)!=0)throw new IndexOutOfRangeException();}
  public void Reset(){n=0;}
 }
 static void Body(int n,int flags,string tag){log.Append(tag).Append(n).Append(';');if((flags&16)!=0)throw new DivideByZeroException();}
 static int FirstReturn(IEnumerable<int> source,int flags){foreach(int x in source){Body(x,flags,"B");return x;}return 0;}
 static int FirstReturnBoth(IEnumerable<int> first,IEnumerable<int> second,int flags){foreach(int x in first)foreach(int y in second){Body(x+y,flags,"N");return x+y;}return 0;}
 static void Run(int scenario,int count,int flags){
  var source=new Source("a",count,flags);
  switch(scenario){
   case 0:foreach(int x in source)Body(x,flags,"B");break;
   case 1:foreach(int x in source){Body(x,flags,"B");break;}break;
   case 2:foreach(int x in source){Body(x,flags,"B");if(x==1)continue;W(x);}break;
   case 3:W(FirstReturn(source,flags));break;
   case 4:W(source.Count());break;
   case 5:A(source.ToArray());break;
   case 6:W(source.Any());break;
   case 7:W(source.First());break;
   case 8:W(source.Last());break;
   case 9:W(source.Single());break;
   case 10:W(source.Sum());break;
   case 11:W(source.Average());break;
   case 12:A(source.Take(1).ToArray());break;
   case 13:A(source.Skip(1).ToArray());break;
   case 14:A(source.Reverse().ToArray());break;
   case 15:A(source.Select(x=>{Body(x,flags,"S");return x*2;}).ToArray());break;
   case 16:A(source.Where(x=>{Body(x,flags,"W");return x%2==1;}).ToArray());break;
   case 17:A(source.SelectMany(x=>{Body(x,flags,"S");return new Source("b",1,flags);}).ToArray());break;
   case 18:A(source.Zip(new Source("b",count+1,flags),(x,y)=>{Body(x,flags,"Z");return x+y;}).ToArray());break;
   case 19:W(source.SequenceEqual(new Source("b",count,flags)));break;
   case 20:var e=source.GetEnumerator();W(e.Current);W(e.MoveNext());e.Dispose();W(e.MoveNext());W(e.Current);e.Dispose();break;
   case 21:W(source.FirstOrDefault());break;
   case 22:W(source.LastOrDefault());break;
   case 23:W(source.ElementAtOrDefault(1));break;
   case 24:A(source.Take(0).ToArray());break;
   case 25:foreach(int x in source)Body(x,flags,"A");break;
   case 26:A(source.Select(x=>x).Zip(new Source("b",count+1,flags),(x,y)=>{Body(x,flags,"Z");return x+y;}).ToArray());break;
   case 27:A(source.Concat(new Source("b",1,flags)).ToArray());break;
   case 28:A(source.TakeWhile(x=>{Body(x,flags,"T");return x<2;}).ToArray());break;
   case 29:A(source.SkipWhile(x=>{Body(x,flags,"T");return x<2;}).ToArray());break;
   case 30:W(source.Where(x=>x%2==1).FirstOrDefault());break;
   case 31:A(source.Take(1).Zip(new Source("b",count+1,flags),(x,y)=>{Body(x,flags,"Z");return x+y;}).ToArray());break;
   case 32:W(source.Select(x=>x).SequenceEqual(new Source("b",count,flags)));break;
   case 33:W(source.Take(1).SequenceEqual(new Source("b",count,flags)));break;
   case 34:foreach(int x in source)foreach(int y in new Source("b",1,flags))Body(x+y,flags,"N");break;
   case 35:W(FirstReturnBoth(source,new Source("b",1,flags),flags));break;
   case 36:W(source.Any(x=>{Body(x,flags,"P");return x>=2;}));break;
   case 37:W(source.All(x=>{Body(x,flags,"P");return x<2;}));break;
   case 38:W(source.Count(x=>{Body(x,flags,"P");return x>=2;}));break;
   case 39:W(source.ToDictionary(x=>{Body(x,flags,"K");return x;}).Count);break;
   case 40:W(new HashSet<int>(source).Count);break;
   case 41:W(new List<int>(source).Count);break;
   case 42:W(source.ToArray().Length);break;
  }
 }
 public static void Generate(string output){
  using(var writer=new StreamWriter(output,false,new UTF8Encoding(false))){
   writer.WriteLine("# Independent CLR "+Environment.Version+" real IEnumerable/IEnumerator and LINQ/foreach finally traces");
   for(int scenario=0;scenario<43;++scenario)foreach(int count in new[]{0,1,3})foreach(int flags in new[]{0,1,2,4,8,16,10,12,24}){
    log=new StringBuilder();string error="ok";
    try{Run(scenario,count,flags);}catch(DivideByZeroException){error="E_DIV_ZERO";}catch(IndexOutOfRangeException){error="E_BOUNDS";}catch(InvalidOperationException){error=count==0 ? "E_EMPTY" : "E_MANY";}
    writer.WriteLine(scenario+"\t"+count+"\t"+flags+"\t"+log+"\t"+error);
   }
  }
 }
}
