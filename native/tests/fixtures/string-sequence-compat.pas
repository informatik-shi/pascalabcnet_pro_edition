begin
  var s:sequence of char:='Я🌍x';
  Writeln(s.Count,' ',s.First,' ',s.Last);
  var a:=s.ToArray;
  Writeln(Ord(a[1]),' ',Ord(a[2]));
  Writeln('abcd'.Take(2).ToArray,' ','abcd'.Skip(2).ToList);
  Writeln('ab'.Concat('cd').ToArray);
  Writeln('abc'.Contains('b'),' ',Ord(''.FirstOrDefault));
end.
