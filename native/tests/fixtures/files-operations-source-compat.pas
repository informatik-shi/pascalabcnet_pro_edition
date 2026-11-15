program FilesOperationsSource;
uses FilesOperations;

procedure WriteBytes(name: string; data: array of integer);
begin
  var stream: file of byte;
  Assign(stream,name);
  Rewrite(stream);
  foreach var value in data do Write(stream,byte(value));
  Close(stream);
end;

procedure DumpBytes(name: string; cp: integer);
begin
  var stream: file of byte;
  Assign(stream,name);
  Reset(stream,Encoding.GetEncoding(1251));
  Write(cp,' bytes');
  while not Eof(stream) do
  begin
    var value: byte;
    Read(stream,value);
    Write(' ',value);
  end;
  Writeln;
  Close(stream);
end;

procedure DumpText(cp, sample: integer; data: string);
begin
  Write(cp,' ',sample);
  foreach var symbol in data do
    if symbol <> #13 then Write(' ',Ord(symbol));
  Writeln;
end;

begin
  Writeln(DefaultEncoding.CodePage);
  WriteBytes('0.bin',Arr(129));
  WriteBytes('1.bin',Arr(130));
  WriteBytes('2.bin',Arr(255,254));
  WriteBytes('3.bin',Arr(239,187));
  WriteBytes('4.bin',Arr(65,226,130));
  WriteBytes('5.bin',Arr(129,48,129));
  WriteBytes('6.bin',Arr(14));
  WriteBytes('7.bin',Arr(27,36));
  WriteBytes('8.bin',Arr(126));
  WriteBytes('9.bin',Arr(43,50,68));
  WriteBytes('10.bin',Arr(194));
  WriteBytes('11.bin',Arr(216,65));
  WriteBytes('12.bin',Arr(0,216));
  WriteBytes('13.bin',Arr(27,36,40,67,161));
  WriteBytes('14.bin',Arr(239,187,191,65));
  WriteBytes('15.bin',Arr(129,65,129));
  WriteBytes('16.bin',Arr(65,66,67,68,226));
  WriteBytes('17.bin',Arr(255,254,0,0,65,0,0));
  WriteBytes('18.bin',Arr(65,0,129));
  WriteBytes('19.bin',Arr(0,0,254,255,0,0,0,65,0));
  WriteBytes('20.bin',Arr(97,13,98));
  WriteBytes('21.bin',Arr(97,13,10,98,10));
  WriteBytes('22.bin',new integer[0]);
  var pages := Arr(37,437,500,708,720,737,775,850,852,855,857,858,860,861,862,863,864,865,866,869,870,874,875,932,936,949,950,1026,1047,1140,1141,1142,1143,1144,1145,1146,1147,1148,1149,1200,1201,1250,1251,1252,1253,1254,1255,1256,1257,1258,1361,10000,10001,10002,10003,10004,10005,10006,10007,10008,10010,10017,10021,10029,10079,10081,10082,12000,12001,20000,20001,20002,20003,20004,20005,20105,20106,20107,20108,20127,20261,20269,20273,20277,20278,20280,20284,20285,20290,20297,20420,20423,20424,20833,20838,20866,20871,20880,20905,20924,20932,20936,20949,21025,21866,28591,28592,28593,28594,28595,28596,28597,28598,28599,28603,28605,29001,38598,50220,50221,50222,50225,50227,51932,51936,51949,52936,54936,57002,57003,57004,57005,57006,57007,57008,57009,57010,57011,65000,65001);
  foreach var cp in pages do
  begin
    DefaultEncoding:=Encoding.GetEncoding(cp);
    FilesOperations.WriteStringToFile('encoded.bin','first Я ø œ あ 한 你 € ✓'+#13+'second'+#10+'third'+#13+#10+'end'+#0);
    DumpBytes('encoded.bin',cp);
    DumpText(cp,-1,FilesOperations.ReadFileToEnd('encoded.bin'));
    for var i:=0 to 22 do
      DumpText(cp,i,ReadFileToEnd(IntToStr(i)+'.bin'));
  end;
  DefaultEncoding:=Encoding.UTF8;
  WriteStringToFile('nil.bin',nil);
  Writeln(ReadFileToEnd('nil.bin')='nil'+NewLine);
  WriteStringToFile('empty.bin','');
  Writeln(ReadFileToEnd('empty.bin')='');
end.