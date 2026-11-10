uses FilesOperations;
type
  Shade = (Dark, Light);
  Pair = record
    number: integer;
    mark: char;
    enabled: boolean;
    shade: Shade;
  end;
begin
  var filename := 'native-binary-records.tmp';
  var f: file of Pair;
  Assign(f, filename);
  Rewrite(f, Encoding.GetEncoding(1251));
  var p: Pair;
  p.number := -123456;
  p.mark := 'Ж';
  p.enabled := true;
  p.shade := Light;
  Write(f, p);
  Writeln(FileSize(f), ' ', FilePos(f));
  Seek(f, 0);
  var q: Pair;
  Read(f, q);
  Writeln(q.number, ' ', q.mark, ' ', q.enabled, ' ', Ord(q.shade));
  Writeln(Eof(f));
  Close(f);
  Erase(f);

  var bytes: file;
  Rewrite(bytes, 'native-binary-values.tmp', Encoding.UTF8);
  Write(bytes, 42, 'Привет', true, real(1.25));
  bytes.Position := 0;
  Writeln(bytes.ReadInteger, ' ', bytes.ReadString, ' ', bytes.ReadBoolean, ' ', bytes.ReadReal);
  Close(bytes);
  Erase(bytes);
end.
