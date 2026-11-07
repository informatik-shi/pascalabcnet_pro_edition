type
  Item=record name:string;points:integer;end;
function Score(name:string;points:integer):Item;
begin Result.name:=name;Result.points:=points;end;
begin
  var scores:=Arr(Score('Alice',70),Score('Boris',95),Score('Vera',80));
  SortDescending(scores,x->x.points);
  foreach var item in scores do Writeln(item.name,': ',item.points);
  var numbers:=Arr(5,1,4,2,3);
  Sort(numbers,(a,b)->a-b);
  Writeln(numbers);
  Sort(numbers,(a,b)->a>b);
  Writeln(numbers);
end.
