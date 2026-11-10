type Vec=array[@LOW@..@HIGH@] of integer;
var mode,seed,delta,calls:integer;
function Init:integer;
begin Write('I');Inc(calls);if mode=8 then C.y:=delta;Result:=seed;end;
function Element(n:integer):integer;
begin if mode=9 then begin Write('E');Write(C.v[@LOW@]);end;Result:=n*11-17;end;
function Make(value:integer):integer;
begin Write('F');Result:=value;end;
procedure SetValue(var target:integer;value:integer);
begin target:=value;end;
type C=class
public
  static x:integer:=Init;
  static y:integer;
  static v:Vec:=(@VALUES@);
  constructor Create(value:integer);
  begin Write('C');end;
  function Get:integer:=x;
end;
begin
  mode:=ReadInteger;seed:=ReadInteger;delta:=ReadInteger;Write('b');
  if mode=0 then Writeln(calls)
  else if mode=1 then Writeln(C.x,' ',C.x,' ',calls)
  else if mode=2 then begin C.x:=Make(delta);Writeln(C.x,' ',calls);end
  else if mode=3 then begin var o:=new C(Make(delta));Writeln(C.x,' ',calls);end
  else if mode=4 then begin SetValue(C.x,Make(delta));Writeln(C.x,' ',calls);end
  else if mode=5 then begin var o:=new C(0);Writeln(o.Get,' ',calls);end
  else if mode=6 then begin Writeln(Low(C.v),' ',High(C.v),' ',calls);Writeln(C.v[@LOW@],' ',calls);end
  else if mode=7 then begin var copy:=C.v;copy[@LOW@]:=delta;Writeln(C.v[@LOW@],' ',copy[@LOW@],' ',calls);end
  else if mode=8 then Writeln(C.x,' ',C.y,' ',calls)
  else Writeln(C.v[@LOW@],' ',calls);
end.
