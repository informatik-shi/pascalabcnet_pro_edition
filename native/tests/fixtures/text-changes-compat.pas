procedure MoveFile(f:Text; name:string);
begin f.Rename(name); end;
begin
  var f:Text;
  Assign(f,'native-text-change-source.txt');
  Rewrite(f); Writeln(f,'native file'); Close(f);
  MoveFile(f,'native-text-change-destination.txt');
  Writeln(FileExists('native-text-change-source.txt'),' ',FileExists('native-text-change-destination.txt'));
  Writeln(f.Name);
  var alias:=f;
  Erase(alias);
  Writeln(ReadAllLines('native-text-change-destination.txt')[0]);
  Assign(alias,'native-text-change-destination.txt');
  f.Rename('native-text-change-destination.txt');
  Writeln(f.Name);
  alias.Erase; Erase(f);
  Writeln(FileExists('native-text-change-destination.txt'));
  Rewrite(f); Write(f,'again'); Close(f);
  Rename(f,'native-text-change-source.txt');
  Writeln(ReadAllText('native-text-change-source.txt'));
  Assign(f,'native-text-change-source.txt'); f.Erase();
end.
