type
  Row=array[-4..5] of integer;
  Item=record data:Row;end;
begin
  var a:array of integer:=(1,2,3,4,5,6,7,8,9,10);
  Writeln(a.Length,' ',a[0],' ',a[9]);
  var row:Row:=(1,2,3,4,5,6,7,8,9,10);
  Writeln(row[-4],' ',row[5]);
  var item:Item;item.data[-4]:=7;
  var items:array of Item:=(item,item);
  items[0].data[-4]:=99;
  Writeln(item.data[-4],' ',items[1].data[-4],' ',items[0].data[-4]);
  var singleton:array of string:=('hello');
  Writeln(singleton[0]);
end.
