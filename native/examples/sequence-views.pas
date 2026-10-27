uses School;

begin
  var массив := Arr(7,-3,9);
  var последовательность: sequence of integer := массив;
  массив[0] := 2;
  Writeln('Представление: ',последовательность.ToArray);
  Writeln('Минимум и максимум: ',последовательность.MinMax);
  var список := new List<integer>;
  список.Add(4);
  var элементы: sequence of integer := список;
  список.Add(5);
  Writeln('Количество: ',Count(элементы));
  Writeln('Копия списка: ',ToArray(список));
  foreach var значение in элементы do
    Writeln('Элемент: ',значение);
end.
