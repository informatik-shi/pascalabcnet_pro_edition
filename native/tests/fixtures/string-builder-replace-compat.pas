begin
  var b := new StringBuilder(2);
  b.Append('ab').Append('cdab').Append('cdab');
  Writeln(b.ToString, ' ', b.Capacity);
  b.Replace('bc','WXYZ');
  Writeln(b.ToString, ' ', b.Capacity);
  b.Replace('WXYZ','q');
  Writeln(b.ToString, ' ', b.Capacity);
  b.Replace('qda','QQ', 0, 5);
  Writeln(b.ToString, ' ', b.Capacity);
  var absent: string := nil;
  b.Replace('d',absent);
  Writeln(b.ToString, ' ', b.Capacity);

  var split := new StringBuilder(3);
  split.Append('aaa').Append('aa').Append('aaa');
  split.Replace('aa','bbbbb');
  Writeln(split.ToString, ' ', split.Capacity);
  split.Replace('bbbbb','');
  Writeln(split.Length, ' ', split.Capacity);

  var same := new StringBuilder('шушу');
  same.Replace('шу','жо',1,3);
  Writeln(same.ToString, ' ', same.Capacity);
  same.Replace('шужо','шужо');
  Writeln(same.ToString, ' ', same.Capacity);
  var restricted := new StringBuilder(2);
  restricted.Append('ab').Append('bb').Append('bb').Append('ab');
  restricted.Replace('bb','*',2,4);
  Writeln(restricted.ToString, ' ', restricted.Capacity);
end.
