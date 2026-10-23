begin
  var f:Text;
  Assign(f,'Учебный текст.txt');
  Rewrite(f);
  Writeln(f,10,' ',20);
  Writeln(f,'Строка из файла');
  Close(f);
  Reset(f);
  var a,b:integer; var line:string;
  Readln(f,a,b); Readln(f,line);
  Writeln('Сумма: ',a+b);
  Writeln(line);
  Writeln('Конец файла: ',Eof(f));
  Close(f);
  Append(f); Writeln(f,'Добавленная строка'); Close(f);
  Reset(f); Readln(f); Readln(f); Readln(f,line); Close(f);
  Writeln(line);
  DeleteFile('Учебный текст.txt');
end.
