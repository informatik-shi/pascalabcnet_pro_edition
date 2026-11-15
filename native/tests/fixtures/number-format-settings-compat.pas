begin
  var f:=NumberFormat('::','_');
  f.NegativeSign:='minus';
  f.PositiveSign:='plus';
  Writeln(StrToFloat('minus12_345::50Eplus02',f));
  Writeln(StrToReal('plus3::125',f),' ',StrToFloat(nil,f));
  Writeln(StrToReal('2',f));
  var nilinfo:NumberFormatInfo;
  Writeln(StrToFloat('2,345.75',nilinfo));
  SetNumberFormat(',', '_');
  Writeln(PABCSystem.Format('{0:N2}',1234.5),' ',FloatToStr(1.25),' ',StrToReal('1_234,50'));
  SetDecimalSeparator('::');
  Writeln(Format('{0:N2}',1234.5),' ',FloatToStr(1.25),' ',StrToFloat('1_234::50'));
  SetNumberFormat;
  Writeln(Format('{0:N2}',1234.5),' ',FloatToStr(1.25));
end.
