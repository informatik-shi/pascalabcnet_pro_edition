uses School;

function ИсключающееИли(a,b: boolean): boolean;
begin
  Result := a xor b;
end;

function ВсеИстинны(значения: array of boolean): boolean;
begin
  Result := True;
  foreach var значение in значения do
    Result := Result and значение;
end;

begin
  Writeln('Исключающее ИЛИ:');
  var таблица := TrueTable(ИсключающееИли);
  TrueTablePrint(таблица);
  Writeln('Три переменные, только истинные строки:');
  TrueTablePrint(TrueTable(3,ВсеИстинны),1,'xyz');
end.
