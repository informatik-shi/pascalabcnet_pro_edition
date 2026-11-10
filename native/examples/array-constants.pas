type Fixed=array[-1..0] of integer;
const
  Table:Fixed=(10,20);
  Sizes:array of integer=(2,3);
  N=Sizes[0]+Sizes[1];
type Buffer=array[1..N] of integer;
procedure Step;
const Pool:array of integer=(7,8);
begin
  var alias:=Pool;Inc(alias[0]);Writeln(Pool[0]);
end;
begin
  var copy:=Table;copy[-1]:=99;
  Writeln(Table[-1],' ',copy[-1]);
  var b:Buffer;Writeln(b.Length);
  Step;Step;
end.
