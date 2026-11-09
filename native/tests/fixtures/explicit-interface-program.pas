type IA=interface
 function F(n:integer):integer;
 function F(s:string):string;
 function Make(seed:integer):integer->integer;
end;
IB=interface
 function F(n:integer):integer;
 procedure Change(var n:integer);
 function Sum(params a:array of integer):integer;
end;
IChild=interface(IA)end;
IResolved=interface(IA,IB)function F(n:integer):integer;end;
AliasA=IA;
C=class(IChild,IB,IResolved)
private
 value:integer;
 function IA.F(n:integer):integer;
 function IA.F(s:string):string;
 function IA.Make(seed:integer):integer->integer;
 function IB.F(n:integer):integer;
 procedure IB.Change(var n:integer);
 function IB.Sum(params a:array of integer):integer;
 function IResolved.F(n:integer):integer;
public
 constructor(initial:integer);begin value:=initial;end;
 function F(n:integer):integer;begin Write('P');Result:=100+n;end;
end;
function C.AliasA.F(n:integer):integer;begin Write('A');Inc(value);Result:=n+value;end;
function C.IA.F(s:string):string;begin Write('S');Result:=s+'!';end;
function C.IA.Make(seed:integer):integer->integer;begin Write('M');Result:=n->Self.F(n)+seed+value;end;
function C.IB.F(n:integer):integer;begin Write('B');Result:=200+n+value;end;
procedure C.IB.Change(var n:integer);begin Write('R');Inc(value);n+=value;end;
function C.IB.Sum(params a:array of integer):integer;begin Write('T');Result:=value;foreach var n in a do Result+=n;end;
function C.IResolved.F(n:integer):integer;begin Write('Z');Result:=300+n+value;end;
function Next(var n:integer):integer;begin Write('X');Inc(n);Result:=n;end;
begin
 var seed,route,isNil:integer;Read(seed,route,isNil);
 var c:=new C(seed);var other:=new C(seed+10);
 var a:IChild:=c;var b:IB:=c;
 if route=1 then a:=other else if route=2 then a:=IChild(b);
 if isNil<>0 then a:=nil;
 Writeln(c.F(2));
 var resolved:IResolved:=c;Writeln(resolved.F(1));
 var n:=seed;
 Writeln(a.F(Next(n)));
 Writeln(a.F('cat'));
 Writeln(b.F(n));
 b.Change(n);Writeln(n);
 Writeln(b.Sum(),' ',b.Sum(2,3),' ',b.Sum(Arr(4,5)));
 var f:=a.Make(seed);
 Writeln(f(7));
 a.F(1);c:=nil;other:=nil;
 Writeln(f(8));
end.
