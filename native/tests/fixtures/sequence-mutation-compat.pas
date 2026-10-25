begin
  var a:=Arr(1,2,3,4,5);
  SetRandomSeed(1);
  Shuffle(a);
  foreach var x in a do Write(x);
  Writeln(' ',Random(100));
  Reverse(a,1,3);
  foreach var x in a do Write(x);
  Writeln;
  var s:string:='Паскаль';
  Reverse(s,2,3);
  Writeln(s);
  var l:=new List<integer>;
  l.Add(10); l.Add(20); l.Add(30);
  l.Reverse;
  Swap(l[0],l[2]);
  foreach var x in l do Write(x,'|');
  Writeln;
  SetRandomSeed(1);
  Writeln(a.Shuffle()=a);
  var same:=a.Swap(0,4);
  Writeln(same=a);
end.
