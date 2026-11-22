program StaticPropertiesCompatibility;
uses Countries;
type
  ParentData=class
  public
    static x:integer;
    static constructor;
    begin
      x:=10;
      Writeln('parent init');
    end;
    [Att(1)]static property BaseValue:integer read x write x;
    property Item:integer read 20;
  end;
  ChildData=class(ParentData)
  public
    static y:integer;
    static constructor;
    begin
      y:=30;
      Writeln('child init');
    end;
    static function ReadLast:integer:=y+2;
    function ReadFirst:integer:=y+1;
    property First:integer read ReadFirst;
    [Att(2)]static property LocalValue:integer read y write y;
    property Middle:string read 'middle';
    static property Last:integer read ReadLast;
    static property Item:integer read y;
  end;
type GrandChildData=class(ChildData)end;
begin
  Writeln('metadata');
  foreach var p in typeof(ParentData).GetProperties do
    Writeln(p.Name,'|',p.DeclaringType.Name,'|',p.ReflectedType.Name,'|',p.CanRead,'|',p.CanWrite);
  Writeln('child');
  foreach var p in typeof(ChildData).GetProperties do
    Writeln(p.Name,'|',p.DeclaringType.Name,'|',p.ReflectedType.Name,'|',p.CanRead,'|',p.CanWrite);
  Writeln(typeof(ChildData).GetProperty('BaseValue')=nil);
  Writeln('grandchild');
  foreach var p in typeof(GrandChildData).GetProperties do
    Writeln(p.Name,'|',p.DeclaringType.Name,'|',p.ReflectedType.Name);
  Writeln(typeof(GrandChildData).GetProperty('Item')=nil);
  var p:=typeof(ChildData).GetProperty('LocalValue');
  Writeln('before value');
  Writeln(p.GetValue(nil),'|',p.GetValue(7),'|',p.GetValue(new ParentData));
  Writeln(p.GetValue(nil,nil),'|',p.GetValue(nil,new object[0]));
  Writeln(Att(p.GetCustomAttributes(True)[0]).Num);
  ChildData.LocalValue:=50;
  Writeln(p.GetValue(nil));
  Writeln(typeof(ChildData).GetProperty('Last').GetValue(nil));
  Writeln(typeof(ParentData).GetProperty('BaseValue').GetValue(nil));
end.
