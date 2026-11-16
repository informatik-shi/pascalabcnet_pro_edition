program CountriesCompatibility;
uses Countries, System;

procedure Describe(c: Country);
begin
  Writeln(c.Название,'|',c.Столица,'|',c.Население,'|',c.Континент);
end;

begin
  var values: array of Countries.Country := Countries.ЗаполнитьМассивСтран();
  Assert(values.Length=ЗаполнитьМассивСтран.Length);
  Writeln(values.Length);
  for var i:=0 to values.Length-1 do Describe(values[i]);
  var c:=new Countries.Country('Тест','Город',42,'Европа');
  Describe(c);
  Writeln(c);
  Writeln(c.ToString);
  Writeln(c=new Country('Тест','Город',42,'Европа'));
  var a:=new Countries.Att(7);
  var b: System.Attribute := a;
  Writeln(a.Num);
  a.Num:=8;
  Writeln(a.Num);
  Writeln(a);
  Writeln(a.ToString);
  Writeln(b.GetType.FullName);
  Writeln(typeof(Country).FullName);
  Writeln(typeof(System.Attribute).FullName);
end.
