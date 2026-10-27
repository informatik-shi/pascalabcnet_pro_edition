uses School;
function F(a,b: boolean): boolean;
begin
  Result := a xor b;
end;
function All(a: array of boolean): boolean;
begin
  Result := True;
  foreach var x in a do Result := Result and x;
end;
begin
  var t := TrueTable(F);
  Writeln(t.RowCount,' ',t.ColCount);
  TrueTablePrint(t);
  School.TrueTablePrint(t,0,'XYZ');
  TrueTablePrint(TrueTable(3,All),1,'xyz');
  TrueTablePrint(TrueTable(0,All));
  PrintDelimDefault := '|';
  TrueTablePrint(new boolean[0,3],2,'x');
end.
