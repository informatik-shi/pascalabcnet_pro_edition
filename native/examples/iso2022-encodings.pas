begin
  foreach var cp in Arr(50220,50221,50222) do
  begin
    var e := Encoding.GetEncoding(cp);
    Writeln(cp, ' ', e.GetBytes('日ｶA'));
  end;
  var korean := Encoding.GetEncoding('iso-2022-kr');
  Writeln(korean.CodePage, ' ', korean.GetString(korean.GetBytes('한국어')));
  var chinese := Encoding.GetEncoding('hz-gb-2312');
  Writeln(chinese.GetBytes('中文~'));
  Writeln(chinese.CodePage, ' ', chinese.GetString(chinese.GetBytes('中文~')));
  Writeln(chinese.GetType.Name);
end.
