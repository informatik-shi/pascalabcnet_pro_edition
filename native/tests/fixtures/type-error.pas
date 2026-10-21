begin
  Writeln('This must never be printed');
  if false then
  begin
    var n := 10;
    n := 'wrong type';
  end;
end.
