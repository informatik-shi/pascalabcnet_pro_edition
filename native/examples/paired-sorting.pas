begin
  var scores:=Arr(30,10,20);
  var names:=Arr('Вера','Аня','Борис');
  System.Array.Sort(scores,names);
  Writeln(scores);
  Writeln(names);
  Array.Sort(scores,names,Comparer<integer>.Create((a,b)->b-a));
  Writeln(scores);
  Writeln(names);
end.
