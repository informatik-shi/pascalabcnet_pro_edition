uses Arrays;
type FillInts=procedure(a:array of integer;value:integer);
begin
  var a:=Arr(3,1,2);
  Arrays.Sort&<integer>(a);
  WritelnArray&<integer>(a,'/');
  var descending:=Arrays.SortDescending&<integer>;
  descending(a);
  Arrays.WritelnArray&<integer>(a,'|');
  var fill:FillInts:=FillByValue&<integer>;
  fill(a,7);
  WriteArray&<integer>(a,',');
  Writeln;
  var m:=new real[1,2];
  FillByValue&<real>(m,1.5);
  WriteMatrix&<real>(m);
  Arrays.WriteMatrix(m);
end.
