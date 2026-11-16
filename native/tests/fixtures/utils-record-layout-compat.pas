program UtilsRecordLayoutSource;
uses Utils, System;
const HexDigits='0123456789ABCDEF';
type
  TAligned = record b:byte; i:integer; s:smallint; end;
  TNested = record b:byte; x:TAligned; t:byte; end;
  TChars = record a,b,c,d:char; end;
  TLogical = record b:byte; enabled:boolean; tail:byte; end;
  TEmpty = record end;
  TEnum = (One, Two, Three);
  TEnumField = record a:byte; enum:TEnum; tail:byte; end;
  TDecimalField = record a:byte; d:decimal; tail:byte; end;
  TDateField = record a:byte; d:DateTime; tail:byte; end;
  TIntervalField = record a:byte; d:TimeSpan; tail:byte; end;
  TComplexField = record a:byte; d:Complex; tail:byte; end;
procedure PrintBytes(labelText:string; bytes:array of byte);
begin
  Write(labelText,' ',bytes.Length,' ');
  foreach var b in bytes do Write(HexDigits[integer(b) div 16+1],HexDigits[integer(b) mod 16+1]);
  Writeln()
end;
begin
  var a:TAligned;
  a.b:=$81; a.i:=$1234ABCD; a.s:=-8;
  var saved:object:=a;
  a.i:=0;
  PrintBytes('snapshot',Utils.RecordToByteArray(saved));
  Writeln(TAligned(saved).i=$1234ABCD);
  var n:TNested; n.b:=$44; n.x:=TAligned(saved); n.t:=$77;
  var toBytes:function(o:object):array of byte:=Utils.RecordToByteArray;
  PrintBytes('nested',toBytes(n));
  var c:TChars; c.a:='a'; c.b:='ё'; c.c:='é'; c.d:='字';
  PrintBytes('chars',RecordToByteArray(c));
  var l:TLogical; l.b:=1; l.enabled:=True; l.tail:=2;
  PrintBytes('logical',RecordToByteArray(l));
  var empty:TEmpty; PrintBytes('empty',RecordToByteArray(empty));
  var e:TEnumField; e.a:=1; e.enum:=Two; e.tail:=2;
  PrintBytes('enum field',RecordToByteArray(e));
  var d:TDecimalField; d.a:=1; d.d:=decimal(0.125); d.tail:=2;
  PrintBytes('decimal field',RecordToByteArray(d));
  var dt:TDateField; dt.a:=1; dt.tail:=2;
  dt.d:=new DateTime(2024,2,29);
  PrintBytes('date field',RecordToByteArray(dt));
  dt.d:=new DateTime(1899,12,29,6,0,0);
  PrintBytes('negative date',RecordToByteArray(dt));
  dt.d:=DateTime.MinValue;
  PrintBytes('zero date',RecordToByteArray(dt));
  var tt:TIntervalField; tt.a:=1; tt.d:=TimeSpan.FromSeconds(0.5); tt.tail:=2;
  PrintBytes('interval field',RecordToByteArray(tt));
  var cc:TComplexField; cc.a:=1; cc.d:=new Complex(1.25,-2.5); cc.tail:=2;
  PrintBytes('complex field',RecordToByteArray(cc));
  PrintBytes('whole char',RecordToByteArray(char('Ж')));
  PrintBytes('whole bool',RecordToByteArray(True));
  PrintBytes('whole int64',RecordToByteArray(int64(-8)));
  PrintBytes('whole real',RecordToByteArray(real(1.25)));
  PrintBytes('whole single',RecordToByteArray(single(1.5)));
  PrintBytes('whole decimal',RecordToByteArray(decimal(0.125)));
end.
