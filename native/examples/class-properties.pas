type
  Counter = class
    n: integer;
    property Value: integer read n write n;
    property DoubleValue: integer read GetDouble;
    function GetDouble: integer;
    begin
      Result := Value * 2;
    end;
  end;
begin
  var c := new Counter;
  c.Value := 7;
  Writeln('Свойство: ', c.Value);
  Writeln('Через метод: ', c.DoubleValue);
end.
