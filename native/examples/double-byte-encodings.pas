begin
  var japanese := Encoding.GetEncoding('shift_jis');
  Writeln(japanese.CodePage, ' ', japanese.GetBytes('日本語'));
  Writeln(japanese.GetString(japanese.GetBytes('日本語')));
  var chinese := Encoding.GetEncoding('gb2312');
  Writeln(chinese.CodePage, ' ', chinese.GetString(chinese.GetBytes('中文')));
  var traditional := Encoding.GetEncoding('big5');
  Writeln(traditional.CodePage, ' ', traditional.GetString(traditional.GetBytes('中文')));
  var korean := Encoding.GetEncoding(949);
  Writeln(korean.CodePage, ' ', korean.GetString(korean.GetBytes('한국어')));
  Writeln(japanese.GetType.Name);
  var euc := Encoding.GetEncoding('euc-jp');
  Writeln(euc.CodePage, ' ', euc.GetString(euc.GetBytes('日本語')), ' ', euc.GetType.Name);
end.
