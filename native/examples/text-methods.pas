begin
  var f:Text;
  Assign(f,'Заметка.txt');
  f.Rewrite;
  f.Println('Цена:',125,'руб.');
  f.Println('Количество:',3);
  f.Close;

  Writeln('Файл: ',f.Name);
  f.Reset;
  Writeln('Первое слово: ',f.ReadWord);
  Writeln('Число: ',f.ReadlnInteger);
  Write('Остаток: ',f.ReadToEnd);
  Writeln('Конец файла: ',f.Eof);
  f.Close;
  DeleteFile('Заметка.txt');
end.
