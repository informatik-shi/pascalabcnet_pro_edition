var calls:integer;
function Next:integer;
begin
  Inc(calls);
  Result:=calls;
end;
function F(a:integer:=Next;b:integer:=Next):integer;
begin
  Result:=a+b;
end;
function TextIsNil(a:Text:=default(Text)):boolean;
begin
  Result:=a=nil;
end;
begin
  Writeln(F,' ',F(9),' ',F(4,5),' ',calls);
  Writeln(default(string)=nil,' ',default(array of integer)=nil,' ',TextIsNil);
end.
