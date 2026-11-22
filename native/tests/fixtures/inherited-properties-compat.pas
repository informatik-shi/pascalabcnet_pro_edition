program InheritedPropertiesCompatibility;
uses Countries, System;
type
  ParentData=class
  public
    x:integer;
    [Att(11)]property Number:integer read x;
    [Att(12)]property Title:string read 'base';
  protected
    [Att(99)]property Hidden:integer read x;
  end;
  ChildData=class(ParentData)
  public
    y:integer;
    [Att(21)]property Local:integer read y;
    property Number:integer read y;
  end;
  CaseData=class(ParentData)
  public
    property number:integer read 30;
  end;
  DifferentData=class(ParentData)
  public
    property Number:string read 'different';
  end;
  ProtectedData=class(ParentData)
  protected
    property Number:integer read 50;
  end;
  PrivateData=class(ParentData)
  private
    property Number:integer read 60;
  end;
  GrandPrivate=class(PrivateData)
  end;
  VirtualParent=class
  public
    function ReadNumber:integer;virtual;begin Result:=10;end;
    [Att(31)]property Value:integer read ReadNumber;
  end;
  VirtualChild=class(VirtualParent)
  public
    function ReadNumber:integer;override;begin Result:=40;end;
  end;
procedure Dump(t:System.Type; instance:Object);
begin
  Writeln(t.Name,'|',t.GetProperties.Length);
  foreach var p in t.GetProperties do
  begin
    Writeln(p.Name,'|',p.DeclaringType.Name,'|',p.ReflectedType.Name,'|',p.GetValue(instance));
    Writeln(p.GetCustomAttributes(False).Length,'|',p.GetCustomAttributes(True).Length,'|',p.IsDefined(typeof(Att),True));
    foreach var a in p.GetCustomAttributes(True) do Writeln(Att(a).Num);
  end;
end;
begin
  var instance:=new ChildData;
  instance.x:=10;
  instance.y:=20;
  Dump(typeof(ChildData),instance);
  var inheritedProperty:=typeof(ChildData).GetProperty('Title');
  Writeln(inheritedProperty=typeof(ChildData).GetProperty('Title'),'|',inheritedProperty=typeof(ParentData).GetProperty('Title'));
  Writeln(typeof(ChildData).GetProperty('Hidden')=nil);
  Writeln(typeof(ChildData).BaseType.Name);
  var caseInstance:=new CaseData;
  caseInstance.x:=10;
  Dump(typeof(CaseData),caseInstance);
  Writeln(typeof(CaseData).GetProperty('number').GetValue(caseInstance),'|',typeof(CaseData).GetProperty('Number').GetValue(caseInstance));
  var different:=new DifferentData;
  different.x:=10;
  Dump(typeof(DifferentData),different);
  Dump(typeof(ProtectedData),new ProtectedData);
  Dump(typeof(PrivateData),new PrivateData);
  Dump(typeof(GrandPrivate),new GrandPrivate);
  var virtualInstance:=new VirtualChild;
  Dump(typeof(VirtualChild),virtualInstance);
  Writeln(typeof(VirtualParent).GetProperty('Value').GetValue(virtualInstance));
end.
