type P=Predicate<integer>;F=Func<integer,boolean>;
 Holder=record Value:P;end;
 Box=class public Value:F;end;
var calls,created,seed,argument,route,operation:integer;
var source,other:P;
var functionSource:F;
function Positive(n:integer):boolean;begin Inc(calls);Result:=n>=0;end;
function Negative(n:integer):boolean;begin Inc(calls);Result:=n<0;end;
function Make(value:integer):P;
begin Result:=n->begin Inc(calls);Result:=n>=value;end;end;
function NextP:P;
begin
 Inc(created);Write('N');
 if route=1 then Result:=nil
 else if (route=2) and (created=2) then Result:=nil
 else if (route=3) and (created=2) then Result:=other else Result:=source;
end;
function NextF:F;
begin
 Inc(created);Write('N');
 if route=1 then Result:=nil
 else if (route=2) and (created=2) then Result:=nil else Result:=functionSource;
end;
begin
 Read(seed,argument,route,operation);
 source:=Positive;other:=Positive;
 if route=1 then source:=nil;
 if route=2 then begin source:=Make(seed);other:=Make(seed);end;
 if route=3 then source:=Negative;
 if operation=0 then begin
  Writeln(typeof(P)=typeof(F));var types:=new HashSet<System.Type>;types.Add(typeof(P));types.Add(typeof(F));Writeln(types.Count);
 end;
 if operation=1 then begin var f:F:=source;Writeln(source.GetType=typeof(P),' ',f.GetType=typeof(F));Writeln(source(argument),' ',f(argument),' ',calls);end;
 if operation=2 then begin
  var a:F:=source;var b:F:=source;var equal:=EqualityComparer<F>.Default.Equals(a,b);
  Writeln(equal);var setOfFunctions:=new HashSet<F>(Arr(a,b));Writeln(setOfFunctions.Count);
  if equal then Writeln(EqualityComparer<F>.Default.GetHashCode(a)=EqualityComparer<F>.Default.GetHashCode(b)) else Writeln('-');
 end;
 if operation=3 then begin var a:F:=source;var b:F:=other;Writeln(EqualityComparer<P>.Default.Equals(source,other));Writeln(EqualityComparer<F>.Default.Equals(a,b));Writeln(new HashSet<F>(Arr(a,b)).Count);end;
 if operation=4 then begin var f:F:=source;var back:P:=f;Writeln(EqualityComparer<P>.Default.Equals(source,back),' ',back=nil);if back<>nil then Writeln(back(argument),' ',calls);end;
 if operation=5 then begin var f:F:=source;source:=Negative;Writeln(f(argument),' ',calls);end;
 if operation=6 then begin Writeln(Arr(-2,0,3).Where(source).ToArray);Writeln(calls);end;
 if operation=7 then begin var values:=new List<P>(Arr(source));Writeln(values.Contains(other));Writeln(new HashSet<P>(Arr(source,other)).Count);end;
 if operation=8 then begin var f:F:=NextP();Writeln(' ',created,' ',f=nil);if f<>nil then Writeln(f(argument),' ',calls);end;
 if operation=9 then begin functionSource:=source;created:=0;var p:P:=NextF();Writeln(' ',created,' ',p=nil);if p<>nil then Writeln(p(argument),' ',calls);end;
 if operation=10 then begin var p:P;var f:F:=p;var back:P:=f;Writeln(f=nil,' ',back=nil);end;
 if operation=11 then begin var f:F:=source;var types:=Arr(source.GetType,f.GetType);Writeln(types[0]=typeof(P),' ',types[1]=typeof(F),' ',types[0]=types[1]);end;
 if operation=12 then begin var f:F:=source;var a:P:=f;var b:P:=f;Writeln(EqualityComparer<P>.Default.Equals(a,b),' ',EqualityComparer<P>.Default.Equals(a,source));Writeln(new HashSet<P>(Arr(a,b,source)).Count);end;
 if operation=13 then begin var h:Holder;h.Value:=source;var b:=new Box;b.Value:=h.Value;h.Value:=nil;Writeln(b.Value(argument),' ',calls);end;
end.
