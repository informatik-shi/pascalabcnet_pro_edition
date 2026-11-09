begin
  var r := new IntRange(2,5);
  var collection: ICollection<integer> := r;
  var same := collection;
  var fresh: ICollection<integer> := r;
  Writeln(collection.Count,' ',collection.IsReadOnly);
  Writeln(collection=same,' ',collection=fresh);
  var sequence: IEnumerable<integer> := collection;
  Writeln(sequence.ToArray);
  var shortArray := new integer[2];
  collection.CopyTo(shortArray,0);
  Writeln(shortArray);
  var restored := IntRange(sequence);
  Writeln(restored.Low,' ',restored.High,' ',restored=r);
  var letters: ICollection<char> := new CharRange('a','c');
  var characters: IEnumerable<char> := letters;
  foreach var ch in characters do Write(ch);
  Writeln;
end.
