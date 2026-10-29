function Parts(value,index:integer):sequence of integer;
begin Result:=SeqFill(index,value); end;
function Chars(value:string):string;
begin Result:=value; end;
begin
  var s:=Seq(5,6,7).SelectMany(Parts);
  Writeln(s.ToArray,' ',s.ToArray);
  Writeln(Seq('ab','cd').SelectMany(Chars).ToArray);
  Writeln(s.Take(2).Sum);
end.
