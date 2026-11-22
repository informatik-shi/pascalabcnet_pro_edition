program CountriesReflectionCompatibility;
uses Countries, System, System.Reflection;

type
  UnknownAttribute = class(System.Attribute)
  end;

begin
  var c := new Country('Alpha','Beta',84,'Gamma');
  var t := c.GetType;
  Writeln(t.GetProperties.Length);
  foreach var p: PropertyInfo in t.GetProperties do
  begin
    Writeln(p.Name,'|',p.DeclaringType.FullName,'|',p.ReflectedType.FullName,'|',p.PropertyType.FullName,'|',p.CanRead,'|',p.CanWrite);
    Writeln(p.GetValue(c));
    Writeln(p.GetValue(c,nil));
    Writeln(p.ToString);
    Writeln(p.IsDefined(typeof(Att),False),'|',p.IsDefined(typeof(System.Attribute),True),'|',p.IsDefined(typeof(UnknownAttribute),True));
    var all := p.GetCustomAttributes(False);
    var attribs := p.GetCustomAttributes(typeof(System.Attribute),True);
    Writeln(all.Length,'|',attribs.Length,'|',Att(all[0]).Num,'|',Countries.Att(attribs[0]).Num,'|',all[0]=attribs[0]);
    Writeln(p.GetCustomAttributes(typeof(UnknownAttribute),False).Length);
  end;
  Writeln(t.GetProperty('название')=nil,'|',t.GetProperty('Unknown')=nil);
  Writeln(t.GetProperty('Название')=t.GetProperty('Название'));
  Writeln(typeof(Att).BaseType.FullName);
  Writeln(typeof(Country).BaseType.FullName);
end.
