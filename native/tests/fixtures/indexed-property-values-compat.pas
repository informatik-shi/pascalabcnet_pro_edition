program IndexedPropertyReflectionOracle;
uses System;
type
  Shade=(ShadeRed,ShadeGreen);
  KnownIndex=interface
    function Value:integer;
  end;
  RootItem=class(KnownIndex)
  public
    n:integer;
    function Value:integer:=n;
  end;
  ChildItem=class(RootItem)
  end;
  ExtraProbe=class
  public
    function ParentIndex(item:RootItem):integer:=if item=nil then -1 else item.n;
    function InterfaceIndex(item:KnownIndex):integer:=item.Value;
    function EnumIndex(item:Shade):integer:=Ord(item);
    function ArrayIndex(items:array of integer):integer:=items.Length;
    function TypeIndex(item:System.Type):string:=if item=nil then 'nil' else item.Name;
    property ParentItem[item:RootItem]:integer read ParentIndex;
    property InterfaceItem[item:KnownIndex]:integer read InterfaceIndex;
    property EnumItem[item:Shade]:integer read EnumIndex;
    property ArrayItem[items:array of integer]:integer read ArrayIndex;
    property TypeItem[item:System.Type]:string read TypeIndex;
  end;
  Probe=class
public
  function GetNumber(index:integer):integer:=index*10;
  function GetText(key:string):string:='key:'+key;
  static function GetStatic(index:integer):integer:=index+3;
  function GetPair(number:integer; text:string):string:=IntToStr(number)+':'+text;
  property Number[position:integer]:integer read GetNumber;
  property Text[query:string]:string read GetText;
  static property StaticNumber[item:integer]:integer read GetStatic;
  property Pair[number:integer; text:string]:string read GetPair;
end;
begin
  var instance:=new Probe;
  foreach var p in typeof(Probe).GetProperties do
  begin
    Writeln(p.ToString);

  end;
  var numberProperty:=typeof(Probe).GetProperty('Number');
  var indices:=new object[1];
  indices[0]:=4;
  Writeln(numberProperty.GetValue(instance,indices));
  indices[0]:=byte(5);
  Writeln(numberProperty.GetValue(instance,indices));
  Writeln(indices[0].GetType.Name);
  indices[0]:=nil;
  Writeln(numberProperty.GetValue(instance,indices));
  Writeln(indices[0]=nil);
  indices[0]:='hello';
  Writeln(typeof(Probe).GetProperty('Text').GetValue(instance,indices));
  indices[0]:=7;
  Writeln(typeof(Probe).GetProperty('StaticNumber').GetValue(nil,indices));
  var pairIndices:=new object[2];
  pairIndices[0]:=2;
  pairIndices[1]:='two';
  Writeln(typeof(Probe).GetProperty('Pair').GetValue(instance,pairIndices));
  var extra:=new ExtraProbe;
  var child:=new ChildItem;
  child.n:=8;
  indices[0]:=child;
  Writeln(typeof(ExtraProbe).GetProperty('ParentItem').GetValue(extra,indices));
  Writeln(typeof(ExtraProbe).GetProperty('InterfaceItem').GetValue(extra,indices));
  indices[0]:=ShadeGreen;
  Writeln(typeof(ExtraProbe).GetProperty('EnumItem').GetValue(extra,indices));
  indices[0]:=nil;
  Writeln(typeof(ExtraProbe).GetProperty('EnumItem').GetValue(extra,indices));
  Writeln(typeof(ExtraProbe).GetProperty('ParentItem').GetValue(extra,indices));
  indices[0]:=Arr(1,2,3);
  Writeln(typeof(ExtraProbe).GetProperty('ArrayItem').GetValue(extra,indices));
  indices[0]:=typeof(integer);
  Writeln(typeof(ExtraProbe).GetProperty('TypeItem').GetValue(extra,indices));
  var noType:System.Type;
  indices[0]:=noType;
  Writeln(typeof(ExtraProbe).GetProperty('TypeItem').GetValue(extra,indices));
end.
