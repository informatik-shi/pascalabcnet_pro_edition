type Pair = KeyValuePair<string,integer>;

procedure Increase(d:IDictionary<string,integer>;key:string;amount:integer);
begin
  var previous:integer;
  if d.TryGetValue(key,previous) then d[key]:=previous+amount
  else d.Add(key,amount);
end;

begin
  var storage:=new Dictionary<string,integer>;
  var counts:IDictionary<string,integer>:=storage;
  counts.Add('cat',2);
  counts.Add(KV&<string,integer>('dog',3));
  Increase(counts,'cat',4);
  Increase(counts,'owl',1);
  Writeln(counts.Count,' ',counts['cat'],' ',counts.ContainsKey('owl'));

  var keys:ICollection<string>:=counts.Keys;
  Writeln(keys.Count,' ',keys.IsReadOnly,' ',counts.Values.Contains(3));
  var pairs:=new Pair[counts.Count];
  counts.CopyTo(pairs,0);
  foreach var pair in pairs do Writeln(pair.Key,'=',pair.Value);

  Writeln(counts.Remove(KV&<string,integer>('dog',9)),' ',counts.Remove('dog'));
  Writeln(keys.Count,' ',storage.Count);
  var source:IEnumerable<Pair>:=counts;
  var same:=IDictionary&<string,integer>(source);
  Writeln(same=counts);
end.
