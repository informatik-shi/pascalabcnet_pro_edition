begin
  var h := HSet(1,2,3,4);
  h.Remove(2);
  var c := Copy(h);
  c.Add(5);
  Writeln(h,' ',c);
  Writeln(HSetInt.Count,' ',HSetStr('one','two','one'));
  Writeln(PABCSystem.HSet&<byte>(2,1,2));
  var a := HashSet&<integer>([3,4,3]);
  Writeln(a,' ',a.ToSet&<integer>);
  var nested := HSet(a);
  a.Add(9);
  Writeln(nested.Count,' ',nested.First.Count);
  Writeln(HSet(a.AsEnumerable));
  var empty: HashSet<integer>;
  Writeln(Copy(empty)=nil);
  var functions := HSet&<integer->integer>(x -> x+1);
  var f := functions.First;
  Writeln(f(4));
end.
