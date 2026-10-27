uses School;

function Хосты: sequence of Addr32;
begin
  var сеть := new CalcIP('10.0.0.1',30);
  Result := сеть.GenAddr;
end;

begin
  var большаяСеть := new CalcIP('128.0.0.1',2);
  foreach var адрес in большаяСеть.GenAddr do
  begin
    Writeln('Первый хост большой сети: ',адрес.Value);
    break;
  end;
  var последовательность: sequence of Addr32 := Хосты;
  Writeln('Количество хостов: ',последовательность.Count);
  var массив := последовательность.ToArray;
  Writeln('Массив: ',массив[0].Value,' ',массив[1].Value);
  foreach var адрес in последовательность do
    Writeln('Повторный перебор: ',адрес.Value);
end.
