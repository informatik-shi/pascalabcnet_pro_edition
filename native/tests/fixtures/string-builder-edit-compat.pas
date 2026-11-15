begin
  var b := new StringBuilder(4);
  b.Append('abcd').Insert(1,'Жy');
  Writeln(b.ToString, ' ', b.Length, ' ', b.Capacity);
  b.Remove(2,2);
  Writeln(b.ToString, ' ', b.Length, ' ', b.Capacity);
  b.Insert(0,'-=', 2).Insert(b.Length, '!');
  Writeln(b.ToString, ' ', b.Length, ' ', b.Capacity);
  b.Remove(0,b.Length);
  Writeln(b.Length, ' ', b.Capacity);

  var more := new StringBuilder(2);
  more.Append('ab').Append('cdef').Append('ghijk');
  Writeln(more.ToString, ' ', more.Length, ' ', more.Capacity);
  more.Insert(1, '---');
  Writeln(more.ToString, ' ', more.Length, ' ', more.Capacity);
  more.Remove(2,8);
  Writeln(more.ToString, ' ', more.Length, ' ', more.Capacity);
  more.Insert(2,42).Insert(1,True).Insert(0,1.5);
  Writeln(more.ToString, ' ', more.Length, ' ', more.Capacity);
  more.Length := 4;
  Writeln(more.ToString, ' ', more.Length, ' ', more.Capacity);
  more.Append('Q', 25);
  Writeln(more.Length, ' ', more.Capacity);
  var chars := Arr('A','B','C','D');
  more.Insert(2,chars,1,2).Insert(1,chars);
  Writeln(more.ToString(0,12), ' ', more.Length, ' ', more.Capacity);
end.
