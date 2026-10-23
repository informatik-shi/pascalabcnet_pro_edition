var calls:integer;
var sharedFile:Text;
function GetFile:Text;
begin Inc(calls); Result:=sharedFile; end;
function BeforePrint:integer;
begin sharedFile.Write('pre'); Result:=2; end;
begin
  var f:TextFile;
  f.Write; f.Print(); f.Flush;
  Assign(f,'native-text-methods.txt');
  Writeln(f.Name,' ',f.FullName=f.FullName());
  f.Rewrite; f.Write(12,' ',34); f.Writeln(' tail');
  f.Print(1,2); f.Println('a','b'); f.Println;
  f.Writeln('True'); f.Writeln('1.5'); f.Writeln('last word'); f.Flush; f.Close;
  f.Reset(); Writeln(f.ReadInteger,' ',f.ReadlnInteger());
  Writeln('[',f.ReadString,']'); Writeln('[',f.ReadlnString,']');
  Writeln(f.ReadlnBoolean,' ',f.ReadlnReal);
  Writeln(f.ReadWord,' [',f.ReadlnString,'] ',f.Eof,' ',f.Eoln());
  Writeln('[',f.ReadToEnd,']'); f.Close;
  f.Append; f.Println('added',3); f.Close;
  f.Reset; f.Readln; f.Readln; f.Readln; f.Readln; f.Readln; f.Readln;
  Writeln(f.ReadString,' ',f.SeekEof()); f.Close;
  sharedFile:=OpenWrite('native-text-methods.txt');
  GetFile().Print(1,BeforePrint); sharedFile.Close;
  Writeln(ReadAllText('native-text-methods.txt'),' ',calls);
  f.Reset; Writeln('[',f.ReadWord,'] ',f.SeekEoln,' ',f.ReadToEnd='2 '); f.Close;
  DeleteFile('native-text-methods.txt');
end.
