function BelowThree(n: integer): boolean;
begin
  Result:=n<3;
end;

function FirstTwo(n,index: integer): boolean;
begin
  Result:=index<2;
end;

begin
  var numbers:=Seq(1,2,3,1);
  Writeln('Начало: ',numbers.TakeWhile(BelowThree).ToArray);
  Writeln('Остаток: ',numbers.SkipWhile(BelowThree).ToArray);
  Writeln('После двух: ',Seq(9,8,7,6).SkipWhile(FirstTwo).ToArray);
end.
