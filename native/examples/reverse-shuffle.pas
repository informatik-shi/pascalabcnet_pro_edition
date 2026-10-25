begin
  var a := Arr(1,2,3,4,5);
  SetRandomSeed(1);
  Shuffle(a);
  Write('Shuffle: ');
  foreach var x in a do Write(x,' ');
  Writeln;
  Reverse(a,1,3);
  Write('Reverse: ');
  foreach var x in a do Write(x,' ');
  Writeln;
  var s: string := 'Паскаль';
  Reverse(s);
  Writeln(s);
  Swap(a[0],a[4]);
  Writeln('Swap: ',a[0],' ',a[4]);
end.
