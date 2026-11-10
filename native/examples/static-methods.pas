type Counter=class
 static total:integer:=10;
 public
 static function Add(n:integer):integer;
 begin total+=n;Result:=total;end;
 static function MakeAdder(n:integer):Func<integer,integer>;
 begin Result:=value->Add(n+value);end;
 static function MakeReader:Func0<integer>;
 begin Result:=()->total;end;
end;
begin
 var add:Func<integer,integer>:=Counter.Add;
 Writeln(add(2));
 var addMore:=Counter.MakeAdder(3);
 Writeln(addMore(4));
 Writeln(Counter.Add(1));
 var reader:Func0<integer>:=Counter.MakeReader;
 Writeln(reader());
end.
