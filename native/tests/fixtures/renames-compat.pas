begin
  var home:=GetDir;
  MkDir('native-renames-case'); ChDir('native-renames-case');
  WriteAllText('from.txt','source'); WriteAllText('occupied.txt','target');
  Writeln(RenameFile('from.txt','renamed.txt'),' ',FileExists('from.txt'),' ',ReadAllText('renamed.txt'));
  Writeln(RenameFile('renamed.txt','./renamed.txt'));
  Writeln(RenameFile('renamed.txt','occupied.txt'),' ',ReadAllText('renamed.txt'),' ',ReadAllText('occupied.txt'));
  Writeln(RenameFile('missing.txt','new.txt'),' ',RenameFile('missing.txt','missing.txt'));
  MkDir('empty'); MkDir('from-dir/child'); WriteAllText('from-dir/child/note.txt','inside');
  Writeln(RenameFile('from-dir','bad-dir'),' ',RenameFile('renamed.txt','empty'));
  Writeln(RenameDirectory('from-dir','renamed-dir'),' ',ReadAllText('renamed-dir/child/note.txt'));
  Writeln(RenameDirectory('renamed-dir','renamed-dir'),' ',RenameDirectory('renamed-dir','empty'),' ',RenameDirectory('renamed-dir','occupied.txt'));
  Writeln(RenameDirectory('renamed-dir','renamed-dir/child/subdir'),' ',RenameDirectory('missing-dir','new-dir'));
  Writeln(RenameFile('renamed.txt','missing-parent/target.txt'),' ',RenameDirectory('renamed-dir','missing-parent/target'));
  Writeln(RenameDirectory('renamed.txt','via-directory.txt'),' ',ReadAllText('via-directory.txt'));
  WriteAllText('case.txt','case'); MkDir('case-dir');
  Writeln(RenameFile('case.txt','CASE.txt'),' ',RenameDirectory('case-dir','CASE-DIR'),' ',ReadAllText('CASE.txt'));
  WriteAllText('старое имя.txt','contents');
  Writeln(RenameFile('старое имя.txt','новое имя.txt'),' ',ReadAllText('новое имя.txt'));
  Writeln(RenameFile(nil,'x'),' ',RenameFile('x',nil),' ',RenameDirectory(nil,'x'),' ',RenameDirectory('x',nil));
  Writeln(RenameFile('','x'),' ',RenameFile('x',''),' ',RenameDirectory('','x'),' ',RenameDirectory('x',''));
  DeleteFile('via-directory.txt'); DeleteFile('occupied.txt'); DeleteFile('CASE.txt'); DeleteFile('новое имя.txt');
  DeleteFile('renamed-dir/child/note.txt'); RmDir('renamed-dir/child'); RmDir('renamed-dir'); RmDir('empty'); RmDir('CASE-DIR');
  ChDir(home); Writeln(RemoveDir('native-renames-case'),' ',GetDir=home);
end.
