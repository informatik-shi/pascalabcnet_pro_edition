type
  State = (Ready, Running, Done);
  [Flags] Access = (None=0, CanRead=1, CanWrite=2, CanExecute=4);
begin
  var state: State;
  Writeln('Начало: ',state);
  Inc(state);
  Writeln('После Inc: ',state,' / ',Ord(state));
  var rights := CanRead or CanWrite;
  Writeln('Права: ',rights);
  Writeln('Запись: ',rights.HasFlag(CanWrite));
  var values := Arr(Done,Ready,Running);
  values.Sort;
  Writeln('Список: ',values);
  for var item:=Ready to Done step 2 do Writeln('Шаг: ',item);
end.
