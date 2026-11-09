uses School;
begin
  var source:IList<string>:=Arr('cat','dog');
  var target:IList<string>:=new List<string>(Arr('кот','пёс'));
  Writeln(ReplaceMany('cat dog cat',source,target));

  var numbers:IList<integer>:=new List<integer>(Arr(3,1));
  numbers.Insert(1,7);
  numbers[0]+=2;
  Writeln(numbers.Item[0],' ',numbers.IndexOf(7));
  numbers.RemoveAt(2);
  foreach var item in numbers do Write(item,' ');
  Writeln;

  var data:=Arr(1,2);
  var fixedList:IList<integer>:=data;
  fixedList[1]:=9;
  Writeln(fixedList.IsReadOnly,' ',data);
end.
