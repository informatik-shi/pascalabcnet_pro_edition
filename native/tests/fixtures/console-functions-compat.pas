begin
  Read; Read();
  var n:=ReadInteger;
  var word:=ReadLexem();
  var rest:=ReadString;
  Writeln(n,' ',word,' [',rest,']');
  var r:=ReadlnReal('Real:');
  var s:=ReadlnString('Text:');
  Writeln(r,' ',s);
  Writeln(ReadString.Length,' ',Ord(ReadChar));
end.
