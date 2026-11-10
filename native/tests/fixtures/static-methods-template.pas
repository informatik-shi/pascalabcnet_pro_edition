var mode,seed,delta,calls:integer;
function Init:integer;
begin Write('I');Inc(calls);if mode=9 then C.Use(delta);Result:=seed;end;
function Make(n:integer):integer;
begin Write('F');Result:=n;end;
type C=class
 static x:integer:=Init;
 public
 static function Use(v:integer):integer;
 begin Write('M');x+=v;Result:=x;end;
 static procedure Ref(var v:integer);
 begin Write('R');v+=x;Inc(x);end;
 function Get:integer:=Use(delta);
 static function Rec(n:integer):integer;
 begin Write('Q');if n=0 then Result:=x else Result:=Rec(n-1)+n;end;
 static function Take(f:Func<integer,integer>;v:integer):integer;
 begin Write('T');Result:=f(v);end;
 static function WithDefault(v:integer:=Make(7)):integer;
 begin Write('D');Result:=x+v;end;
 static function Sum(params values:array of integer):integer;
 begin Write('P');Result:=x;foreach var n in values do Result+=n;end;
 static function Pick(n:integer):integer;
 begin Write('A');Result:=x+n;end;
 static function Pick(s:string):integer;
 begin Write('B');Result:=x+s.Length;end;
 static function Factory(n:integer):Func<integer,integer>;
 begin Write('G');Result:=z->x+n+z;end;
end;
begin
 Read(mode,seed,delta);Write('b');
 if mode=0 then begin var f:Func<integer,integer>:=C.Use;Writeln(calls);end
 else if mode=1 then Writeln(C.Use(Make(delta)),' ',calls)
 else if mode=2 then begin var f:Func<integer,integer>:=C.Use;Write(calls);Writeln(f(Make(delta)),' ',calls);end
 else if mode=3 then begin var f:Func<integer,integer>:=C.Use;Write(f(Make(delta)),' ');Writeln(f(Make(delta)),' ',calls);end
 else if mode=4 then begin var v:=Make(delta);C.Ref(v);Writeln(v,' ',C.x,' ',calls);end
 else if mode=5 then begin var o:=new C;Writeln(o.Get,' ',calls);end
 else if mode=6 then begin var f:=C.Factory(Make(delta));Write(f(3),' ');Writeln(f(5),' ',calls);end
 else if mode=7 then Writeln(C.Rec(Make(3)),' ',calls)
 else if mode=8 then begin var f:Func<integer,integer>:=C.Use;Writeln(C.Take(f,Make(delta)),' ',calls);end
 else if mode=9 then Writeln(C.Use(Make(delta)),' ',calls)
 else if mode=10 then Writeln(calls)
 else if mode=11 then Writeln(C.WithDefault,' ',calls)
 else if mode=12 then begin Write(C.Sum(Make(delta),Make(3)),' ');Writeln(C.Sum,' ',calls);end
 else if mode=13 then begin Write(C.Pick(Make(delta)),' ');Writeln(C.Pick('xy'),' ',calls);end
 else if mode=14 then begin var f:Func<integer,integer>:=C.Pick;Write(calls);Writeln(f(Make(delta)),' ',calls);end
 else if mode=15 then begin var f:=C.Factory(Make(delta));Writeln(f(Make(3)),' ',calls);end;
end.
