begin
  MkDir('native-enumeration-demo/nested');
  WriteAllText('native-enumeration-demo/alpha.txt','a');
  WriteAllText('native-enumeration-demo/plain','p');
  WriteAllText('native-enumeration-demo/nested/бета.txt','b');
  var files: sequence of string := EnumerateFiles('native-enumeration-demo');
  Writeln(files.Count,' ',EnumerateAllFiles('native-enumeration-demo').Count,' ',
    EnumerateDirectories('native-enumeration-demo').Count,' ',
    EnumerateAllDirectories('native-enumeration-demo').Count);
  Writeln(files.Count,' ',EnumerateAllFiles('native-enumeration-demo','*.txt').Count);
  Writeln(EnumerateFiles('native-enumeration-demo','nested/*.txt').Count);
  var ok:=True;
  foreach var path in EnumerateAllFiles('native-enumeration-demo') do
    ok:=ok and FileExists(path);
  Writeln(ok);
  DeleteFile('native-enumeration-demo/alpha.txt');
  DeleteFile('native-enumeration-demo/plain');
  DeleteFile('native-enumeration-demo/nested/бета.txt');
  RmDir('native-enumeration-demo/nested');
  RmDir('native-enumeration-demo');
end.
