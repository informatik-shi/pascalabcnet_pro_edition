type
 LastDigitEquality = class(IEqualityComparer<integer>)
 public
  function Equals(a,b:integer):boolean := a mod 10 = b mod 10;
  function GetHashCode(a:integer):integer := a mod 10;
 end;
begin
 var comparison:IEqualityComparer<integer> := new LastDigitEquality;
 var words := new Dictionary<integer,string>(comparison);
 words.Add(1,'one');
 words[11] := 'same last digit';
 Writeln(words.Count,' ',words[21]);
 var retained := Copy(words);
 var ordinary := new Dictionary<integer,string>(words);
 Writeln(retained.Comparer=comparison,' ',retained.ContainsKey(31));
 Writeln(ordinary.Comparer=EqualityComparer<integer>.Default,' ',ordinary.ContainsKey(31));
 var missing:Dictionary<integer,string>;
 Writeln(Copy(missing)=nil);
end.
