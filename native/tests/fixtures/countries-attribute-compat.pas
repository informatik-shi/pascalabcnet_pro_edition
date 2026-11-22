program CountriesAttributeCompatibility;
uses Countries, System;

begin
  var a:=new Att(17);
  var b:=new Att(17);
  var c:=new Att(18);
  var base:System.Attribute:=a;
  Writeln(a=b,'|',a.Equals(b),'|',a.Equals(c),'|',a.Equals(nil),'|',a.Equals(17));
  Writeln(a.Match(b),'|',a.Match(c),'|',a.Match(nil));
  Writeln(a.GetHashCode,'|',a.IsDefaultAttribute);
  Writeln(a.TypeId,'|',a.TypeId.GetType.FullName);
  Writeln(base.Equals(b),'|',base.Match(c),'|',base.GetHashCode,'|',base.IsDefaultAttribute);
  Writeln(base.TypeId);
  Writeln(a.TypeId.ToString);
  Writeln(a.TypeId=b.TypeId,'|',a.TypeId=c.TypeId);
  var typeIds:=new HashSet<object>;
  typeIds.Add(a.TypeId);
  Writeln(typeIds.Add(b.TypeId),'|',typeIds.Count);
  Writeln(System.Collections.Generic.EqualityComparer&<Att>.Default.Equals(a,b),'|',System.Collections.Generic.EqualityComparer&<Att>.Default.GetHashCode(a));
  Writeln(System.Collections.Generic.EqualityComparer&<Att>.Default.Equals(nil,nil),'|',System.Collections.Generic.EqualityComparer&<Att>.Default.GetHashCode(nil));
  var items:=new HashSet<Att>;
  Writeln(items.Add(a),'|',items.Add(b),'|',items.Add(c),'|',items.Count,'|',items.Contains(new Att(17)));
  var dictionary:=new Dictionary<Att,string>;
  dictionary.Add(a,'value');
  Writeln(dictionary[b]);
  var objects:=new HashSet<object>;
  Writeln(objects.Add(a),'|',objects.Add(b),'|',objects.Contains(new Att(17)));
  var bases:=new HashSet<System.Attribute>;
  Writeln(bases.Add(a),'|',bases.Add(b),'|',bases.Contains(new Att(17)));
  var first:=typeof(Country).GetProperty('Название').GetCustomAttributes(False);
  var second:=typeof(Country).GetProperty('Название').GetCustomAttributes(False);
  Writeln(first[0]=second[0],'|',Att(first[0]).Equals(second[0]));
  a.Num:=integer.MinValue;
  Writeln(a.GetHashCode,'|',a.Equals(b));
  a.Num:=integer.MaxValue;
  Writeln(a.GetHashCode);
end.
