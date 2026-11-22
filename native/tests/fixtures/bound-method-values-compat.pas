program BoundMethodValuesCompatibility;
type
  RootData=class
  public
    state:integer;
    function Value:integer;virtual:=state;
    procedure Add(n:integer);virtual:=state+=n;
  end;
  LeafData=class(RootData)
  public
    function Value:integer;override:=inherited Value+5;
    procedure Add(n:integer);override:=inherited Add(n*2);
  end;
type
  AbstractData=abstract class
  public
    function Answer:integer;virtual;abstract;
  end;
  ConcreteData=class(AbstractData)
  public
    function Answer:integer;override:=42;
  end;
function Twice(n:integer):integer;overload:=n*2;
function Twice(n:string):string;overload:=n+n;
begin
  var leaf:=new LeafData;
  var root:RootData:=leaf;
  root.Add(3);
  Writeln(root.Value,'|',leaf.Value,'|',root.state);
  var getter:()->integer:=root.Value;
  var update:integer->():=root.Add;
  update(2);
  Writeln(getter(),'|',root.state);
  Writeln(Twice(4),'|',Twice('ab'));
  var abstractReference:AbstractData:=new ConcreteData;
  Writeln(abstractReference.Answer);
end.
