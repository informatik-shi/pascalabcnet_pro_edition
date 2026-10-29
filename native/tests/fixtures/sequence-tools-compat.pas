begin
  var a:=Arr(1,2,3);
  Writeln(a.AsEnumerable.Reverse.ToArray,' ',a);
  Writeln((new integer[0]).DefaultIfEmpty.ToList);
  Writeln(''.DefaultIfEmpty('x').Single);
  Writeln(a.SequenceEqual(Seq(1,2,3)),' ',SequenceEqual('ab','ac'));
  Writeln('abcd'.Reverse.ToArray);
end.
