type
  Coordinate = record
    X, Y: integer;
  end;
begin
  Writeln(typeof(integer).Name);
  Writeln(typeof(integer).FullName);
  Writeln(typeof(integer).Namespace);
  Writeln(typeof(Coordinate).FullName);
  Writeln(typeof(Dictionary<integer,string>).Name);
  Writeln(new List<integer>().GetEnumerator.GetType.Name);
  Writeln(typeof(integer).AssemblyQualifiedName);
end.
