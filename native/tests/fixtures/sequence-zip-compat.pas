function Add(a,b: integer): integer;
begin
  Result:=a+b;
end;
begin
  Writeln(Zip(Seq(1,2,3),Seq(10,20)).ToArray);
  Writeln(Zip(Seq(1),Seq(2),Seq(3),Seq(4),Seq(5)).First);
  Writeln(Seq(1,2).Zip(Seq(10,20),Add).ToArray);
  Writeln('ab'.ZipTuple(Seq(1,2)).ToArray);
  var empty: sequence of integer;
  var z:=Zip(empty,Seq(1));
  Writeln(z.Take(0).Count);
end.
