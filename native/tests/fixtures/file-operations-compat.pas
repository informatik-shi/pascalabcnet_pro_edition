uses FilesOperations, Events, PointRect;
function ReadFileToEnd(name:string):string;
begin Result:='user'; end;
begin
  WriteStringToFile('native-file-operations.txt','one'#13'two'#10'three'#13#10'four');
  Write(FilesOperations.ReadFileToEnd('native-file-operations.txt'));
  FilesOperations.WriteStringToFile('native-file-operations.txt','');
  Writeln(FilesOperations.ReadFileToEnd('native-file-operations.txt')='');
  var s:string:=nil;
  WriteStringToFile('native-file-operations.txt',s);
  Writeln(ReadAllText('native-file-operations.txt'));
  WriteStringToFile('native-file-operations.txt','Я🌍');
  Write(FilesOperations.ReadFileToEnd('native-file-operations.txt'));
  Writeln(ReadFileToEnd('native-file-operations.txt'));
  Writeln(PABCSystem.Abs(-3),' ',PABCSystem.Chr(65));
  Writeln(DeleteFile('native-file-operations.txt'));
end.
