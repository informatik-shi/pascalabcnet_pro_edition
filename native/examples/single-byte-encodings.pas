begin
  var dos := Encoding.GetEncoding(866);
  Writeln(dos.CodePage, ' ', dos.GetBytes('Привет'));
  Writeln(dos.GetString(dos.GetBytes('Привет')));
  var koi := Encoding.GetEncoding('koi8-r');
  Writeln(koi.CodePage, ' ', koi.GetString(koi.GetBytes('Привет')));
  var western := Encoding.GetEncoding('windows-1252');
  Writeln(western.GetBytes('€é'));
  var latin := Encoding.GetEncoding('latin1');
  Writeln(latin.GetType.Name, ' ', latin.GetBytes('éＡ'));
  var ebcdic := Encoding.GetEncoding('IBM037');
  Writeln(ebcdic.CodePage, ' ', ebcdic.GetBytes('ABC'));
end.
