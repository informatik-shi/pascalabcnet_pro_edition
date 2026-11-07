type
  CounterCursor = class(IEnumerator<integer>)
  private
    position, limit: integer;
  public
    constructor Create(last: integer);
    begin limit := last; end;
    function MoveNext: boolean;
    begin
      position += 1;
      Result := position <= limit;
    end;
    property Current: integer read position;
    procedure Reset;
    begin position := 0; end;
    procedure Dispose;
    begin Writeln('closed'); end;
  end;
  Counters = class(IEnumerable<integer>)
  public
    function GetEnumerator: IEnumerator<integer>;
    begin Result := new CounterCursor(3); end;
  end;
begin
  var values: IEnumerable<integer> := new Counters;
  foreach var value in values do
  begin
    Writeln(value);
    if value = 2 then break;
  end;
  Writeln(values.Sum);
end.
