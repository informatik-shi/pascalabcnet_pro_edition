begin
  Writeln(IsMatch('abc123', '[0-9]+'));
  Writeln(Regex.IsMatch('abc', '[0-9]+'));
  Writeln(PABCSystem.IsMatch('abc', '^a'));
  Writeln('abc123'.MatchValue('[0-9]+'));
  Writeln(MatchValue('abc', 'x').Length);
  Writeln(IsMatch('', ''), ' ', IsMatch('abc', ''));
  Writeln(MatchValue('Яя 123', '\p{L}+'));
  Writeln(IsMatch('١', '\d'));
  Writeln(Ord(MatchValue(#55356#57101, '.')[1]));
end.
