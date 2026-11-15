begin
  var b := new StringBuilder(4);
  b.AppendFormat('left {{X}} [{1,-4}] [{0,6}] {0:D4} {2:F2} {3:x2}', 7, 'ёж', 2.125, 255);
  Writeln(b.ToString, ' ', b.Capacity);
  b.Clear;
  b.AppendFormat('{{{0}}} {1:N2} {2,-5} {3:X}', 12,1234.5,'я',shortint(-1));
  Writeln(b.ToString);
  var values := new object[3];
  values[0] := 5;
  values[1] := 'тест';
  values[2] := true;
  b.Clear;
  b.AppendFormat('{2}: {1} [{0,5}] ', values).AppendFormat('{0} / {1}', nil, 'end');
  Writeln(b.ToString);
  Writeln(Format('{0:N2}|{1,-5}|{2}:{3}',1234.5,'Х',nil,True));
  Writeln(PABCSystem.Format('{2}-{1}-{0}', values));
  Writeln(Format('{{no args}}'));
  Writeln(Format('{0:F0}:{1:F3}:{2:D4}',-2.5,0.0625,-24));
  Writeln(Format('{0} {1:X} {2:x}', 'Ю', -1, int64(-1)));
  Writeln(Format('{0} {1}',new object,Arr(1,2)));
  Writeln(Format('{0:G2}/{1:g2}/{2:G2}', -12, 12345, -12345));
  Writeln(Format('{0:E0}/{1:e2}/{2:E2}', 99.9, 0.125, -999.9));
end.
