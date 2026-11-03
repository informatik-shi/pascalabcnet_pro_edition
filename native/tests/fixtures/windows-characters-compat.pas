begin
  var bytes := Arr(byte(207), byte(240), byte(232), byte(226), byte(229), byte(242));
  Writeln(bytes.Select(ChrWindows).JoinToString);
  Writeln('Коды: ', OrdWindows('П'), ' ', OrdAnsi('ё'));
  Writeln('Замена: ', ChrAnsi(OrdWindows('界')));
end.
