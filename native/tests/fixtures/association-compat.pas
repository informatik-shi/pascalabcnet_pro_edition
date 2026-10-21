uses Collections;
type Point=record x:integer; end;
begin
  var m:=new AssocArray<string,integer>();
  Writeln(m['missing'],' [',m.ToString,']');
  m['b']:=2; m.Elem['a']:=1; m['b']:=3;
  Writeln(m['b'],' ',m.Elem['a']);
  m.Print; Write('|'); m.Println;
  Writeln(Length(m.ToString));
  var p:Point; p.x:=7;
  var records:=new AssocArray<string,Point>; records['p']:=p;
  p.x:=9; var copied:=records['p']; copied.x:=11;
  copied:=records['p']; var missing:=records['missing'];
  Writeln(copied.x,' ',p.x,' ',missing.x);
  var keys:=new AssocArray<Point,string>; keys[p]:='stored';
  p.x:=10; keys[p]:='other';
  p.x:=9; Writeln(keys[p]); p.x:=10; Writeln(keys[p]);
  var a:=Arr(1); var arrays:=new AssocArray<array of integer,integer>;
  arrays[a]:=7; a[0]:=8;
  Writeln(arrays[a],' ',arrays[Arr(8)]);
  arrays[nil]:=9; Writeln(arrays[nil]);
  var refs:=new AssocArray<string,array of integer>; refs['a']:=a;
  var alias:=refs['a']; alias[0]:=9;
  Writeln(a[0],' ',refs['absent']=nil);
  var nested:=new AssocArray<string,AssocArray<string,integer>>;
  nested['map']:=m; var shared:=nested['map']; shared['b']:=4;
  Writeln(m['b'],' ',nested['absent']=nil);
end.
