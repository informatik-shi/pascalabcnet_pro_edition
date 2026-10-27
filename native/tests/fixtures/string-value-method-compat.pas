begin
  var s := 'abcde';
  Writeln('[',s.PadLeft(8),'] [',s.PadRight(7,'0'),']');
  Writeln(s.Substring(2),' ',s.Substring(1,3),'|',s.Substring(5));
  Writeln(s.Remove(2),' ',s.Remove(1,3));
  Writeln(s.Insert(0,'X'),' ',s.Insert(5,'Y'),' ',s.Insert(2,''));
  Writeln(s);
  var u := 'Я🌍';
  Writeln(u.PadLeft(4,'0'),' ',u.Substring(1,2));
  Writeln(Ord(u.Substring(1,1)[1]),' ',Ord(u.Substring(2)[1]));
  Writeln(''.PadRight(3,'z'));
end.
