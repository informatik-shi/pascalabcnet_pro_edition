program ForeachIndexSource;

procedure Nested;
begin
  foreach var outer in Arr(8,9) index row do
  begin
    foreach var inner in Arr(4,5) index column do
      Write(row,':',column,'=',outer+inner,' ');
  end;
  Writeln(']');
end;

begin
  foreach var value in Arr(10,20,30,40) index i do
  begin
    if i=1 then continue;
    Writeln(i,' ',value);
    if i=2 then break;
  end;

  var previous:=42;
  foreach previous in Seq(1,2,3,4).Where(v->v mod 2=0) index j do
  begin
    loop 2 do Write(j,':',previous,' ');
  end;
  Writeln(previous);

  foreach var symbol in 'ABC' index position do
    Write(position,symbol,' ');
  Writeln(']');
  foreach var missing in new integer[0] index k do Writeln(k,missing);
  Nested;
end.
