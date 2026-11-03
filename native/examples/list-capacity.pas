begin
  var l := new List<integer>(3);
  Writeln('Пустой список: ', l.Count, ' ', l.Capacity);
  l.AddRange(Arr(1, 2, 3));
  l.Add(4);
  Writeln('После роста: ', l, ' ', l.Capacity);
  l.TrimExcess;
  Writeln('После сжатия: ', l.Capacity);
  Writeln('Из последовательности: ', new List<integer>(SeqGen(3, i -> i+1)));
  foreach var x in l do
  begin
    l.EnsureCapacity(20);
    l.TrimExcess;
    Write(x);
  end;
  Writeln;
end.
