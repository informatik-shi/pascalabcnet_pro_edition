program UtilsGuidConversionSource;
uses Utils, System;
const HexDigits='0123456789ABCDEF';
const Canonical='00010203-0405-0607-0809-0a0b0c0d0e0f';
const Expression='CONVERT(''00010203-0405-0607-0809-0a0b0c0d0e0f'',System.Guid)';
type Envelope=record code:byte; id:Guid; tail:byte; end;
procedure Dump(data:array of byte);
begin
  foreach var b in data do Write(HexDigits[integer(b) div 16+1],HexDigits[integer(b) mod 16+1]);
  Writeln()
end;
begin
  var boxed:=Utils.Eval(Expression);
  Writeln(boxed.GetType.FullName,' ',boxed.ToString);
  var g:System.Guid:=Guid(boxed);
  Writeln(g.ToString('N'));
  Writeln(g.ToString('B'));
  Writeln(g.ToString('P'));
  Writeln(g.ToString('X'));
  Writeln(System.Guid.Empty);
  var reparsed:=System.Guid.Parse(g.ToString('X'));
  var fromBytes:=new Guid(g.ToByteArray);
  Writeln(g.Equals(reparsed),' ',g.Equals(fromBytes),' ',g.CompareTo(Guid.Empty));
  var fromText:=new Guid(Canonical);
  Writeln(fromText.Equals(g));
  Dump(g.ToByteArray);
  Dump(Utils.RecordToByteArray(g));
  var wrapped:Envelope;
  wrapped.code:=1; wrapped.id:=g; wrapped.tail:=2;
  Dump(RecordToByteArray(wrapped));
  Writeln(EvalBool(Expression+'=''00010203-0405-0607-0809-0A0B0C0D0E0F'''));
  Writeln(EvalBool(Expression+' IN ('+Expression+', NULL)'));
  Writeln(EvalBool('''00010203-0405-0607-0809-0a0b0c0d0e0f'' IN ('+Expression+')'));
  Writeln(EvalStr('CONVERT('+Expression+',System.String)'));
  Writeln(EvalStr('CONVERT(''{0X1,0X2,0X3,{0X4,0X5,0X6,0X7,0X8,0X9,0XA,0XB}}'',System.Guid)'));
  Writeln(Utils.Eval('CONVERT(NULL,System.Guid)')=object(DBNull.Value));
end.
