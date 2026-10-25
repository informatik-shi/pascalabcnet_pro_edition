uses Arrays;
begin
  Write('По символам UTF-16: ');
  foreach var c in '🌍' do Write(c);
  Writeln;
  Write('Отдельные Write: ');
  Write(#55356); Write(#57101); Writeln;
  Write('Массив: ');
  WritelnArray(Arr(#55356,#57101),'');
end.
