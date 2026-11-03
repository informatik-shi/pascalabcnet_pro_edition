type
  Counter = class
    value: integer;
    procedure Add(step: integer := 1);
    begin
      value := value + step;
    end;
    function Current: integer;
    begin
      Result := value;
    end;
  end;
begin
  var c := new Counter;
  c.Add();
  c.Add(4);
  Writeln('Значение: ', c.Current);
  var alias := c;
  alias.Add;
  Writeln('Общая ссылка: ', c.Current());
end.
