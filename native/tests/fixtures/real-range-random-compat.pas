begin
  var r: RealRange := 1.5..3.5;
  Writeln(r,' ',r.Size,' ',2.5 in r);
  Writeln(r.GetHashCode);
  Randomize(1);
  var p := Random2(r);
  var c := Random3('a'..'z');
  Randomize(1);
  Writeln(p=(Random(1.5,3.5),Random(1.5,3.5)));
  Writeln(c=(Random('a','z'),Random('a','z'),Random('a','z')));
  Randomize(1);
  var m := MatrRandomReal(1,3,r,99);
  Randomize(1);
  Writeln(m.Row(0).SequenceEqual(MatrRandomReal(1,3,1.5,3.5,2).Row(0)));
  Writeln(InternalRange(1,3),' ',InternalRange('a','c'));
end.
