const First = 2;
type Slot = First..4;
     Letter = 'b'..'d';
     Table = array[Slot, Letter] of integer;

begin
  var values: Table;
  values[2, 'b'] := 7;
  values[4, 'd'] := 9;
  Writeln(Low(values), ' ', High(values));
  Writeln(values[2, 'b'], ' ', values[4, 'd']);

  var state: Slot := 3;
  case state of
    2: Writeln('ожидание');
    3..4: Writeln('готово');
  end;
end.
