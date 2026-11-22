program BoundMethodIdentityOracle;
uses System;
type
  RootData=class
  public
    state:integer;
    function Value:integer;virtual:=state;
    function Plain:integer:=state+100;
    procedure Add(n:integer);virtual:=state+=n;
  end;
  LeafData=class(RootData)
  public
    function Value:integer;override:=inherited Value+5;
  end;
var root:RootData;
var calls:integer;
function Target:RootData;
begin
  calls+=1;
  Result:=root;
end;
begin
  root:=new LeafData;
  root.state:=10;
  var a:()->integer:=root.Value;
  var b:()->integer:=root.Value;
  var pa:()->integer:=root.Plain;
  var pb:()->integer:=root.Plain;
  var alias:=a;
  Writeln(a=b,'|',pa=pb,'|',a=alias);
  var receiver:=root;
  var byExpression:()->integer:=Target.Value;
  Writeln(calls,'|',byExpression());
  root:=new LeafData;
  root.state:=20;
  Writeln(a(),'|',pa(),'|',byExpression(),'|',calls);
  receiver.state:=30;
  Writeln(a(),'|',pa());

end.
