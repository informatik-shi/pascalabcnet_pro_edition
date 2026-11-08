uses FilesOperations;
begin
  var filename:='native-stream-signature.tmp';
  // Empty UTF-16 file contains the two-byte signature FF FE.
  WriteAllText(filename,'',Encoding.Unicode);
  DefaultEncoding:=Encoding.UTF32;
  var value:=ReadFileToEnd(filename);
  Writeln('Код замены: ',Ord(value[1]));
  Writeln('Число символов: ',Length(ReadAllText(filename)));
  var f:=OpenRead(filename);
  Writeln('Text: ',Ord(f.ReadChar));
  f.Close;
  Writeln('Удалён: ',DeleteFile(filename));
end.
