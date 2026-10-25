uses Arrays;
type Item=record
  name:string;
  scores:array of integer;
end;
begin
  var a:=new Item[2];
  a[0].name:='Анна'; a[0].scores:=Arr(5,4);
  a[1].name:='Борис'; a[1].scores:=new integer[0];
  Writeln(a);
  WritelnArray(a,' / ');
  Writeln('[',FormatValue(a[0],20),']');
  var m:=new Item[1,2]; m[0,0]:=a[0]; m[0,1]:=a[1];
  Writeln(ObjectToString(m));
  var l:=new List<Item>; l.Add(a[0]); l.Add(a[1]);
  Writeln(l);
  var f:Text;
  Assign(f,'object-output-test.txt'); Rewrite(f);
  f.Writeln(a); f.Println(a[0],m); Close(f);
  Writeln(ReadAllText('object-output-test.txt'));
  Erase(f);
  Writeln('done');
end.
