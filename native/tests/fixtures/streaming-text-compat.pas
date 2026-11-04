uses System.Text;

begin
  var name:='native-streaming-text.tmp';
  WriteAllText(name,'17 слово остаток 🌍'#13#10#0'конец',Encoding.Unicode);
  var f:=OpenRead(name,Encoding.ASCII);
  Writeln('Число: ',f.ReadInteger);
  Writeln('Слово: ',f.ReadWord);
  Writeln('Остаток строки: [',f.ReadlnString,']');
  Writeln('Нулевой символ: ',Ord(f.ReadChar));
  Writeln('Остаток файла: ',f.ReadToEnd);
  Writeln('Конец файла: ',f.Eof);
  f.Close;DeleteFile(name);
end.
