begin
  Randomize(1);
  var s := SeqRandom(3,1,9);
  Writeln('created');
  Writeln(s.ToArray);
  Randomize(1);
  Writeln(s.Take(2).ToArray);
  var next := Random(100);
  Randomize(1);
  Random(1,9); Random(1,9);
  Writeln(next=Random(100));
  Randomize(1);
  var a := SeqRandomReal(2,0,10).ToArray;
  Randomize(1);
  Writeln(a.SequenceEqual(Arr(RandomReal(0,10,1),RandomReal(0,10,1))));
  Writeln(SeqRandom(-1,0,MaxInt).Count);
end.
