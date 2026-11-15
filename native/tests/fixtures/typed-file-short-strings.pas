type Tiny = string[3];
type Row = record
  key: byte;
  first: Tiny;
  second: string[4];
end;

var f: file of Row;

begin
  var r: Row;
  Rewrite(f, 'short-strings.dat', Encoding.GetEncoding(1251));
  r.key := 7;
  r.first := #1071#1073;
  r.second := 'text';
  Write(f, r);
  r.key := 8;
  r.first := '';
  r.second := #1102;
  Write(f, r);
  Writeln(FilePos(f), ' ', FileSize(f));
  Close(f);

  Reset(f, 'short-strings.dat', Encoding.GetEncoding(1251));
  Seek(f, 1);
  Read(f, r);
  Writeln(r.key, ' ', r.first.Length, ' ', Ord(r.second[1]), ' ', FilePos(f), ' ', Eof(f));
  Seek(f, 0);
  Read(f, r);
  Writeln(r.key, ' ', r.first.Length, ' ', Ord(r.first[1]), ' ', Ord(r.first[2]), ' ', r.second);
  Close(f);

  var raw: file;
  Reset(raw, 'short-strings.dat');
  Writeln(raw.ReadBytes(20));
  Close(raw);
  Erase(raw);
end.
