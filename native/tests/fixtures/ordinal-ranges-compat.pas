begin
  var r: IntRange := 1..5;
  Writeln(r,' ',r.Count,' ',2.5 in r);
  Writeln(r.ToArray,' ',r.Step(2).ToArray,' ',r.Reverse.ToArray);
  var a := Arr(9,9,9);
  r.CopyTo(a,1);
  Writeln(a);
  var letters: CharRange := 'a'..'c';
  Writeln(letters,' ',HSet(letters));
  var seq: sequence of integer := r;
  r := 9..10;
  Writeln(seq.ToArray,' ',r);
  var wide := new IntRange(MaxInt,(not MaxInt));
  Writeln(wide.IsEmpty,' ',wide.Count,' ',wide.ToArray,' ',HSet(wide));
end.
