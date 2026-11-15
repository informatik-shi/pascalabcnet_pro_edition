uses System;
var active: SortedSet<integer>;
function Rebind: integer;
begin
  active:=SSetInt(8);
  Result:=9;
end;
begin
  var reverse:=System.Collections.Generic.Comparer&<integer>.Create((a,b) -> b-a);
  var a:=new SortedSet<integer>(Arr(4,2),reverse);
  var b:=SSetInt(1,2,3);
  var joined:=a+b;
  var difference:=a-b;
  var common:=a*b;
  Writeln(joined.Min,' ',joined.Max,' ',difference.Count,' ',difference.Min,' ',common.Min);
  var plus:=a+5;
  var minus:=a-4;
  Writeln(plus.Min,' ',plus.Max,' ',minus.Min,' ',a.Min,' ',a.Max);
  var mn: SortedSet<integer>;
  var otherNil: SortedSet<integer>;
  Writeln(mn=otherNil,' ',a=mn,' ',a<>mn,' ',SSetInt(4,2)=a);
  Writeln(SSetInt(2)<a,' ',a<=joined,' ',joined>b,' ',joined>=a);
  active:=SSetInt(3);
  var prev:=active;
  active+=Rebind;
  active+=Seq(1,2);
  active-=9;
  active-=Seq(2,8);
  Writeln(prev.Count,' ',prev.Min,' ',active.Count,' ',active.Min);
end.
