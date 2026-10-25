uses Arrays;
begin
  var a,b:integer;
  Readln(a,b);
  Writeln(a,' ',b);
  var c,d:char;
  Read(c,d);
  Writeln(Ord(c),' ',Ord(d));
  var s:string;
  Read(s); Writeln('[',s,']'); Readln;
  var numbers:=new integer[3];
  ReadArray(numbers); numbers.Writeln(',');
  var names:=new string[3];
  Arrays.ReadArray(names); names.Writeln('|');
  Readln();
  Readln(s); Writeln(s);
end.
