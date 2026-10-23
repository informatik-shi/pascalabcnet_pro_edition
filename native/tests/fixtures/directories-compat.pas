begin
  var home:=GetDir;
  Writeln(home=GetCurrentDir());
  Writeln(CreateDir('native-directory-case/nested'),' ',CreateDir('native-directory-case'));
  MkDir('native-directory-case/empty');
  Writeln(SetCurrentDir('native-directory-case'),' ',GetDir=GetCurrentDir);
  WriteAllText('note.txt','hello');
  WriteAllLines('lines.txt',Arr('alpha','beta'));
  var lines:=ReadAllLines('lines.txt');
  Writeln(FileExists('note.txt'),' ',ReadAllText('note.txt'),' ',Length(lines),' ',lines[1]);
  Writeln(CreateDir('note.txt'),' ',SetCurrentDir('note.txt'),' ',RemoveDir('note.txt'));
  Writeln(SetCurrentDir('missing'),' ',FileExists('note.txt'));
  Writeln(RemoveDir('nested'),' ',RemoveDir('nested'));
  RmDir('empty');
  Writeln(DeleteFile('note.txt'),' ',DeleteFile('note.txt'));
  Writeln(DeleteFile('lines.txt'),' ',DeleteFile('.'),' ',FileExists('.'));
  ChDir(home);
  Writeln(GetCurrentDir=home,' ',RemoveDir('native-directory-case'),' ',RemoveDir('native-directory-case'));
  Writeln(CreateDir(nil),' ',RemoveDir(nil),' ',DeleteFile(nil),' ',SetCurrentDir(nil));
  Writeln(CreateDir(''),' ',RemoveDir(''),' ',SetCurrentDir(''));
  Writeln(ChangeFileNameExtension('report.old.txt','dat'),' ',ChangeFileNameExtension('report','txt'));
  Writeln('[',ChangeFileNameExtension('.hidden','.txt'),'] [',ChangeFileNameExtension('a.b.c',''),'] [',ChangeFileNameExtension('a.b.c',nil),']');
  Writeln('[',ChangeFileNameExtension('a/','.txt'),'] [',ChangeFileNameExtension('','.txt'),'] [',ChangeFileNameExtension(nil,'.txt'),']');
  Writeln(ChangeFileNameExtension('dir.ext/file','.txt'),' ',ChangeFileNameExtension('dir.ext/file',nil));
end.
