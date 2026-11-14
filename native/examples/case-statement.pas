type Signal = (red, yellow, green);

function Describe(signal: Signal): string;
begin
  case signal of
    red: Result := 'стоп';
    yellow: Result := 'внимание';
    green: Result := 'можно идти';
  end;
end;

begin
  Writeln(Describe(yellow));
  var speed := 42;
  case speed of
    0: Writeln('стоит');
    1..60: Writeln('в пределах нормы');
    else Writeln('слишком быстро');
  end;
end.
