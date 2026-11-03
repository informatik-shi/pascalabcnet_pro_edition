begin
  var l := Arr(1,2,3).ToList;
  l.AddRange(Arr(4,5));
  Writeln('Добавление: ', l);
  l.InsertRange(1,SeqGen(2,i -> i+7));
  Writeln('Вставка: ', l);
  var copy := Arr(1,2,3).ToList;
  copy.InsertRange(1,copy);
  Writeln('Вставка в себя: ', copy);
  var chars := new List<char>;
  chars.AddRange('abc');
  Writeln('Символы: ', chars);
end.
