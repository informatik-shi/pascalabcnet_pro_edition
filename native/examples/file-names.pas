begin
  var path:='missing-folder/../данные.tar.gz';
  Writeln(ExtractFileName(path),' ',ExtractFileExt(path));
  Writeln(ExpandFileName(path)=ExpandFileName('данные.tar.gz'));
  Writeln(ExtractFileDir(path)=GetDir);
  Writeln(ExtractFilePath(path)=GetDir+#92);
  Writeln(ExtractFileDrive(path)=ExtractFileDrive('other.txt'));
  Writeln('[',ExtractFileName('part/.'),'][',ExtractFileName('part/'),']');
end.
