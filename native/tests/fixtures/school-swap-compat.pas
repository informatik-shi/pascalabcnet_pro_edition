uses School;
begin
  var s: string := 'cat dog cat';
  SwapSubstr(s, 'cat', 'dog');
  Writeln(s);
  School.SwapSubstr(s, 'cat', 'dog');
  Writeln(s);
  s.SwapSubstr('cat', 'dog');
  Writeln(s);
  s := 'aaa aa';
  SwapSubstr(s, 'aa', 'a');
  Writeln(s);
  s := 'a';
  SwapSubstr(s, '', 'a');
  Writeln(s);
  s := 'a';
  SwapSubstr(s, 'a', '');
  Writeln(s);
  s := 'a.b z';
  SwapSubstr(s, 'a.b', 'z');
  Writeln(s);
  s := 'abc';
  SwapSubstr(s, 'missing', nil);
  Writeln(s);
  s := 'a^ b^';
  SwapSubstr(s, 'a^', 'b^');
  Writeln(s);
end.
