program BoundMethodOverloadsCompatibility;
uses System;
type
  RefProc=procedure(var n:integer);
  RootData=class
  public
    state:integer;
    function Value:integer;virtual:=state;
    function Apply(n:integer):integer;overload:=state+n;
    function Apply(s:string):string;overload:=s+IntToStr(state);
    procedure Bump(var n:integer):=n+=state;
    function Make:()->integer:=()->state;
  end;
  LeafData=class(RootData)
  public
    function Value:integer;override:=inherited Value+5;
    function BaseGetter:()->integer:=inherited Value;
  end;
function MakeGetter(instance:RootData):()->integer:=instance.Value;
begin
  var instance:=new LeafData;
  instance.state:=7;
  var intApply:integer->integer:=instance.Apply;
  var textApply:string->string:=instance.Apply;
  var func:System.Func<integer,integer>:=instance.Apply;
  Writeln(intApply(3),'|',textApply('a'),'|',func(4));
  var bump:RefProc:=instance.Bump;
  var number:=10;
  bump(number);
  Writeln(number);
  var baseGetter:=instance.BaseGetter;
  var virtualGetter:=MakeGetter(instance);
  Writeln(baseGetter(),'|',virtualGetter());
  var made:()->integer:=instance.Make;
  instance.state:=20;
  Writeln(made(),'|',baseGetter(),'|',virtualGetter());
end.
