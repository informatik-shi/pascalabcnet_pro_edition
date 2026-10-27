program Проверка;
uses School;
type Запись = record Поле: integer; end;
var Данные: Запись;
function Удвоить(Число: integer): integer;
begin
  Result := число*2;
end;
begin
  данные.поле := 21;
  Writeln(УДВОИТЬ(ДАННЫЕ.ПОЛЕ));
  Writeln(НОД(18,30),' ',нок(18,30));
  Writeln(School.НоД(-18,30),' ',School.НоК(-18,30));
  Writeln(НОД(-9223372036854775808,2));
  Writeln(School.Dec('ſı',36));
end.
