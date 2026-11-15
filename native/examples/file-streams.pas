uses FilesOperations;
begin
  var filename:='native-stream-signature.tmp';
  // Empty UTF-16 file contains the two-byte signature FF FE.
  WriteAllText(filename,'',Encoding.Unicode);
  DefaultEncoding:=Encoding.UTF32;
  Writeln('ReadFileToEnd: ',Length(ReadFileToEnd(filename)));
  Writeln('ReadAllText: ',Length(ReadAllText(filename)));
  var f:=OpenRead(filename);
  Writeln('Text.ReadToEnd: ',Length(f.ReadToEnd));
  f.Close;
  Writeln('Удалён: ',DeleteFile(filename));
end.
