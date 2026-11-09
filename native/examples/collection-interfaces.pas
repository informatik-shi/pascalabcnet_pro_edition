begin
  var list:=new List<integer>;
  var collection:ICollection<integer>:=list;
  collection.Add(7);
  collection.Add(8);
  Writeln(collection.Count,' ',collection.IsReadOnly,' ',collection.Contains(7));

  var destination:=new integer[3];
  collection.CopyTo(destination,1);
  Writeln(destination);
  foreach var item in collection do Write(item,' ');
  Writeln;
  Writeln(collection.Remove(7),' ',collection.Count);

  var dictionary:=new Dictionary<integer,string>;
  var pairs:ICollection<KeyValuePair<integer,string>>:=dictionary;
  pairs.Add(KV(1,'one'));
  Writeln(pairs.Contains(KV(1,'one')));
  var keys:ICollection<integer>:=dictionary.Keys;
  Writeln(keys.Count,' ',keys.IsReadOnly);

  collection.Clear;
  Writeln(collection.Count);
end.
