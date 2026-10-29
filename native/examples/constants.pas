const
  Count = 3;
  LastIndex: byte = Count-1;
  Greeting = 'Постоянные значения';
  Rounded = Round(1.3);

procedure PrintValues;
const
  Step = 2;
begin
  var values := new integer[Count](Step,Step*2,Step*3);
  Writeln(Greeting,': ',values);
end;

begin
  PrintValues;
  Writeln('Индекс: ',LastIndex,'; округление: ',Rounded);
end.
