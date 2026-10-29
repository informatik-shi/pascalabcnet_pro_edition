begin
  var s := 0 + Range(1,3).Concat(Seq(4,5)) + 6;
  Writeln(s.ToArray);
  Writeln(s.Take(3).Sum, ' ', s.Skip(5).ToList);
  Writeln(Concat(Arr(1,2),Arr(3,4)).ToArray);
  Writeln(('a'+Range('b','c')+'d').ToArray);
end.
