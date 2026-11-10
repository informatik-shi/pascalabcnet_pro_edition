begin
  var e := Encoding.GetEncoding(50222);
  Writeln(e.WebName, ' ', e.BodyName, ' ', e.WindowsCodePage);
  Writeln(e.IsSingleByte, ' ', e.IsReadOnly);
  Writeln(e.GetMaxByteCount(3), ' ', e.GetMaxCharCount(3));
  Writeln(Encoding.Unicode.GetMaxCharCount(3));
  Writeln(Encoding.GetEncoding(65000).GetMaxCharCount(0));
end.