var calls:integer;
function Next:integer;
begin
  Inc(calls);
  Result:=calls;
end;
function Add(a:integer; b:integer:=Next):integer;
begin
  Result:=a+b;
end;
function EmptyText(s:string:=default(string)):boolean;
begin
  Result:=s=nil;
end;
begin
  Writeln('Первый вызов: ',Add(10));
  Writeln('Второй вызов: ',Add(10));
  Writeln('Явный аргумент: ',Add(10,7));
  Writeln('Вычислений по умолчанию: ',calls);
  Writeln('Строковый default равен nil: ',EmptyText);
end.
