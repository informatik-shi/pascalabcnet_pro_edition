program PropertySetValueCompatibility;
uses Countries,System;
type
  RootData=class
  public
    value:integer;
    text:string;
    kindStorage:System.Type;
    static sharedValue:integer;
    static constructor;
    begin
      sharedValue:=100;
      Writeln('static init');
    end;
    function ReadNumber:integer;virtual:=value;
    procedure PutNumber(n:integer);virtual:=value:=n;
    function GetAt(index:integer):integer:=value+index;
    procedure PutAt(index:integer;n:integer):=value:=index+n;
    [Att(7)]property Number:integer read ReadNumber write PutNumber;
    property Caption:string read text write text;
    property Kind:System.Type read kindStorage write kindStorage;
    property Item[position:integer]:integer read GetAt write PutAt;
    static property Shared:integer read sharedValue write sharedValue;
    property ReadOnly:integer read 1;
  end;
  ChildData=class(RootData)
  public
    procedure PutNumber(n:integer);override:=inherited PutNumber(n*2);
  end;
begin
  Writeln('metadata');
  var sharedProperty:=typeof(RootData).GetProperty('Shared');
  Writeln('before set');
  sharedProperty.SetValue(nil,byte(5));
  Writeln(sharedProperty.GetValue(7));
  var instance:=new ChildData;
  var numberProperty:=typeof(RootData).GetProperty('Number');
  numberProperty.SetValue(instance,byte(7));
  Writeln(instance.value,'|',numberProperty.GetValue(instance));
  numberProperty.SetValue(instance,nil);
  Writeln(instance.value);
  var captionProperty:=typeof(RootData).GetProperty('Caption');
  captionProperty.SetValue(instance,nil);
  Writeln(captionProperty.GetValue(instance)=nil);
  captionProperty.SetValue(instance,'done');
  Writeln(instance.Caption);
  var indices:=new object[1];
  indices[0]:=byte(3);
  var itemProperty:=typeof(RootData).GetProperty('Item');
  itemProperty.SetValue(instance,byte(4),indices);
  Writeln(instance.value,'|',itemProperty.GetValue(instance,indices),'|',indices[0].GetType.Name);
  var kindProperty:=typeof(RootData).GetProperty('Kind');
  kindProperty.SetValue(instance,typeof(integer));
  Writeln(instance.Kind.Name);
  kindProperty.SetValue(instance,nil);
  Writeln(instance.Kind=nil);
  Writeln(typeof(RootData).GetProperty('ReadOnly').CanWrite);
  Writeln(Att(numberProperty.GetCustomAttributes(False)[0]).Num);
end.
