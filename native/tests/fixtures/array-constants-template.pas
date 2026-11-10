type
  Fixed=array[@LOW@..@HIGH@] of integer;
  Mixed=array[@LOW@..@HIGH@] of array of integer;
var ConstructorCalls:integer;
function Value(v:integer):integer;
begin Inc(ConstructorCalls);Result:=v;end;
const
  DynamicPool:array of integer=(@VALUES@);
  FixedPool:Fixed=(@VALUES@);
  MixedPool:Mixed=(@PAIRS@);
  RuntimePool:array of integer=(Value(5),Value(6));
var delta:integer;
procedure Step;
const LocalPool:array of integer=(10,20);
begin
  var d:=DynamicPool;var f:=FixedPool;var m:=MixedPool;var l:=LocalPool;
  d[0]+=delta;f[@LOW@]+=delta;m[@LOW@,0]+=delta;l[0]+=delta;
  Writeln(DynamicPool[0],' ',FixedPool[@LOW@],' ',f[@LOW@],' ',MixedPool[@LOW@,0],' ',LocalPool[0]);
end;
begin
  Writeln(RuntimePool[0],' ',RuntimePool[1],' ',ConstructorCalls);
  var calls:=ReadInteger;delta:=ReadInteger;
  for var k:=1 to calls do Step;
end.
