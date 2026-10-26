begin
  Writeln(' a  b '.ToWords);
  Writeln('a,b;c'.ToWords(',',';'));
  var separators:array of char;
  Writeln(('a'#9'b'#8195'c').ToWords(separators));
  var lines:=('a'#13#10#10'b'#13).ToLines;
  Writeln(lines.Length);
  foreach var line in lines do Writeln('[',line,']');
  Writeln(' 12 -3'#9'7 '.ToIntegers);
  Writeln(ToIntegers('12abc',1));
  Writeln('1.5 -2e1'.ToReals);
  Writeln('1,5 2,25'.ToReals(','));
  var missing:string:=nil;
  Writeln(missing.ToIntegers(0));
end.
