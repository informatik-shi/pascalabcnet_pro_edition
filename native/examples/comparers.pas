type
  Score=class(IComparable<Score>)
  public
    points:integer;
    constructor Create(value:integer);begin points:=value;end;
    function CompareTo(other:Score):integer;
    begin if other=nil then Result:=1 else Result:=points-other.points;end;
  end;
begin
  var low:=new Score(10);var value:=new Score(25);var high:=new Score(30);
  Writeln(InRange(value,low,high));
  Writeln(Clamp(value,low,high).points);
  var order:=Comparer<Score>.Default;
  Writeln(order.Compare(value,high));
  var reverse:=Comparer<Score>.Create((a,b)->b.CompareTo(a));
  var contract:IComparer<Score>:=reverse;
  Writeln(contract.Compare(value,high));
end.
