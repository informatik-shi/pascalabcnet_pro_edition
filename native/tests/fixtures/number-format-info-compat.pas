begin
  var absent: NumberFormatInfo;
  Writeln(absent=nil);
  var inv := NumberFormatInfo.InvariantInfo;
  var cur := System.Globalization.NumberFormatInfo.CurrentInfo;
  Writeln(inv.IsReadOnly, ' ', cur.IsReadOnly, ' ', inv=NumberFormatInfo.InvariantInfo);
  var f := NumberFormat(',', '_');
  var alias := f;
  Writeln(f.NumberDecimalSeparator, ' ', f.NumberGroupSeparator, ' ', f.NumberDecimalDigits);
  alias.NumberDecimalDigits:=4;
  f.NumberGroupSizes:=Arr(2,3,0);
  var copy := f.NumberGroupSizes;
  copy[0]:=1;
  Writeln(alias.NumberDecimalDigits,' ',f.NumberGroupSizes[0],' ',f.NumberGroupSizes[2]);
  var protected_format := NumberFormatInfo.ReadOnly(f);
  f.NumberDecimalSeparator:='::';
  Writeln(protected_format.IsReadOnly,' ',f.IsReadOnly,' ',NumberFormatInfo.ReadOnly(protected_format)=protected_format);
  Writeln(protected_format.NumberDecimalSeparator,' ',f.NumberDecimalSeparator,' ',f.GetType.FullName);
  Writeln(FloatToStr(123.75,protected_format),' ',PABCSystem.FloatToStr(123.75,f),' ',FloatToStr(123.75,absent));
  var defaults := PABCSystem.NumberFormat;
  Writeln(defaults.NumberDecimalSeparator,' ',defaults.NumberGroupSeparator,' ',defaults.NumberNegativePattern);
  defaults.NegativeSign:='minus';
  defaults.PositiveSign:='plus';
  Writeln(FloatToStr(-1.25,defaults));
  var fresh := new System.Globalization.NumberFormatInfo;
  Writeln(fresh.NumberDecimalDigits,' ',fresh.IsReadOnly,' ',cur=NumberFormatInfo.CurrentInfo);
  cur.NumberDecimalSeparator:='~';
  Writeln(FloatToStr(2.5,absent),' ',FloatToStr(2.5,inv));
end.
