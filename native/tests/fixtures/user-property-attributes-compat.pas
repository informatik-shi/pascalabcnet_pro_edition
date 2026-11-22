program UserCountryAttributesOracle;
uses Countries, System;
const BaseNumber=3;
type
  Sample = class
  private
    title: string := 'hello';
    count: integer := 42;
    property Hidden: integer read count;
  public
    [Att(BaseNumber*3)]
    property Caption: string read title write title;
    [Countries.Att(-BaseNumber-1)]
    property Total: integer read count;
    property Plain: integer read count;
  end;
begin
  var s:=new Sample;
  foreach var p in typeof(Sample).GetProperties do
  begin
    Writeln(p.Name,'|',p.PropertyType.FullName,'|',p.CanRead,'|',p.CanWrite,'|',p.GetValue(s));
    var attrs:=p.GetCustomAttributes(typeof(Att),False);
    Writeln(attrs.Length,'|',p.IsDefined(typeof(Att),False));
    if attrs.Length>0 then
    begin
      Writeln(Att(attrs[0]).Num);
      Att(attrs[0]).Num:=101;
      Writeln(Att(p.GetCustomAttributes(False)[0]).Num);
    end;
  end;
  Writeln(typeof(Sample).GetProperty('Hidden')=nil);
  Writeln(typeof(Sample).GetProperty('caption')=nil);
  Writeln(typeof(Sample).GetProperty('Caption')=typeof(Sample).GetProperties[0]);
end.
