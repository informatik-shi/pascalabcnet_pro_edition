begin
  var файл := OpenWrite('big-integer-example.txt');
  Writeln(файл,123456789012345678901234567890bi);
  Close(файл);
  файл := OpenRead('big-integer-example.txt');
  var число := файл.ReadBigInteger;
  Close(файл);
  Writeln('Из файла: ',число*2);
  DeleteFile('big-integer-example.txt');
end.
