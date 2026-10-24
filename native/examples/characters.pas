begin
  var letter:char:='А';
  Inc(letter);
  Writeln('Буква: ',letter);
  var code:word:=Ord(letter);
  Writeln('Код UTF-16: ',code);
  var name:='Анна';
  name[1]:='И';
  Writeln('Имя: ',name);
  Write('Алфавит: ');
  for var c:='а' to 'е' do Write(c);
  Writeln;
  Write('Символы: ');
  foreach var c in name do Write(c,'|');
  Writeln;
end.
