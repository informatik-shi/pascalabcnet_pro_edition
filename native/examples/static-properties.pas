type Meter=class
 static data:=Arr(10,20,30);
 public
 static function Get(i:integer):integer:=data[i];
 static procedure Put(i,value:integer);
 begin data[i]:=value;end;
 static property Item[i:integer]:integer read Get write Put;default;
 static property Total:integer read data.Sum;
end;
Pair=record
 n:integer:=7;
 static property Zero:Pair read default(Pair);
end;
begin
 Writeln(Meter.Total);
 Meter.Item[1]+=5;
 Writeln(Meter[1],' ',Meter.Total);
 var p:Pair;
 Writeln(p.n,' ',Pair.Zero.n);
end.
