var mode,seed,delta,calls:integer;
function Init:integer;
begin Write('I');Inc(calls);if mode=10 then Write(C.ConstValue);Result:=seed;end;
function Make(n:integer):integer;
begin Write('F');Result:=n;end;
function Index(n:integer):integer;
begin Write('K');Result:=n;end;
type C=class
 static raw:integer=Init;
 public
 static function GetP:integer;
 begin Write('G');Result:=raw;end;
 static procedure SetP(n:integer);
 begin Write('S');raw:=n;end;
 static function GetItem(i,j:integer):integer;
 begin Write('G');Result:=raw+i*3+j;end;
 static procedure SetItem(i,j,n:integer);
 begin Write('S');raw:=n-i*3-j;end;
 static function GetCallback:Func<integer,integer>;
 begin Write('G');Result:=n->raw+n;end;
 static property P:integer read GetP write SetP;
 static property FieldValue:integer read raw write raw;
 static property Calc:integer read raw+7 write raw:=value-7;
 static property ConstValue:integer read 7;
 static property WO:integer write SetP;
 static property Item[i,j:integer]:integer read GetItem write SetItem;default;
 static property Callback:Func<integer,integer> read GetCallback;
end;
function DefaultValue(n:integer:=C.P):integer;
begin Write('D');Result:=n;end;
begin
 Read(mode,seed,delta);Write('b');
 if mode=0 then Writeln(calls)
 else if mode=1 then Writeln(C.P,' ',calls)
 else if mode=2 then begin C.P:=Make(delta);Writeln(C.raw,' ',calls);end
 else if mode=3 then begin C.P+=Make(delta);Writeln(C.raw,' ',calls);end
 else if mode=4 then begin Write(C.FieldValue,' ');Writeln(C.FieldValue,' ',calls);end
 else if mode=5 then begin Write(C.Calc,' ');C.Calc:=Make(delta);Writeln(C.raw,' ',calls);end
 else if mode=6 then Writeln(C.Item[Index(-3),Index(2)],' ',calls)
 else if mode=7 then begin C.Item[Index(-3),Index(2)]:=Make(delta);Writeln(C.raw,' ',calls);end
 else if mode=8 then begin C.Item[Index(-3),Index(2)]+=Make(delta);Writeln(C.raw,' ',calls);end
 else if mode=9 then Writeln(C.Callback(Make(delta)),' ',calls)
 else if mode=10 then Writeln(C.ConstValue,' ',calls)
 else if mode=11 then begin C.WO:=Make(delta);Writeln(C.raw,' ',calls);end
 else if mode=12 then begin var f:=C.Callback;Write(calls);Writeln(f(Make(delta)),' ',calls);end
 else if mode=13 then Writeln(C[Index(-3),Index(2)],' ',calls)
 else if mode=14 then begin C.Calc:=Make(delta);Writeln(C.raw,' ',calls);end
 else Writeln(DefaultValue,' ',calls);
end.
