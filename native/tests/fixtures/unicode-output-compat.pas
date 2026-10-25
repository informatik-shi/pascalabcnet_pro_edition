uses Arrays,Collections;
begin
  foreach var c in '🌍' do Write(c);
  Writeln;
  Write(#55356); Write(''); Write(#57101); Writeln;
  WritelnArray(Arr(#55356,#57101),'');
  WritelnArray(Arr(#55356,#57101));
  var q:=new Queue<char>; q.Enqueue(#57101);
  Write(#55356); q.Print; Writeln('|');
  Write(#55356); Write(1); Write(#57101); Writeln;
end.
