begin
  var e := Encoding.GetEncoding('x-iscii-de');
  Writeln(e.CodePage, ' ', e.GetType.Name);
  var text := 'कকகకಕകકକਕ';
  Writeln(e.GetString(e.GetBytes(text)));
  Writeln(e.GetBytes('कকக'));
  var punjabi := Encoding.GetEncoding(57011);
  Writeln(punjabi.CodePage, ' ', punjabi.GetString(punjabi.GetBytes('ਕ')));
end.
