uses School;

function Apply(f:Func<integer,int64>;n:integer):int64:=f(n);

begin
  var absolute:Func<integer,integer>:=Abs;
  var square:Func<integer,int64>:=Sqr;
  var binary:Func<int64,string>:=School.Bin;
  var version:Func0<string>:=PascalABCVersion;
  Writeln(absolute(-7),' ',Apply(square,50000));
  Writeln(Seq('one','два').Select(UpperCase).ToArray);
  Writeln(binary(5),' ',version());
end.
