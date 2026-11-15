uses System.Text;
begin
  var d:=Decimal.Parse('12345.6700');
  Writeln(d.ToString,' ',d.ToString('G'),' ',d.ToString('G4'));
  Writeln(d.ToString('F3'),' ',d.ToString('N2'),' ',d.ToString('E3'));
  Writeln(Decimal.Parse('1.245').ToString('F2'),' ',Decimal.Parse('-1.245').ToString('F2'));
  var p:=NumberFormat('::','_');
  p.NegativeSign:='~';
  Writeln((-d).ToString('N2',p),' ',(-d).ToString(p));
  Writeln(Format('{0} {0:F2} {0:N1}',d));
  var o:object:=d;
  d:=Decimal.Zero;
  Writeln(o.GetType.FullName,' ',Decimal(o),' ',Format('{0,15:F2}',o));
  var b:=new StringBuilder;
  b.Append(Decimal.Parse('1.2300'));
  b.AppendFormat(p,' [{0:N2}]',o);
  Writeln(b.ToString);
end.
