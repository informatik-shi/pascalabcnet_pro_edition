type Coordinate=record x,y:real;end;

begin
  Writeln(TypeName(default(Coordinate)));
  var t:System.Type:=typeof(array[,] of integer);
  Writeln(TypeToTypeName(t));
  Writeln(TypeName(Seq(1,2)));
  Writeln(TypeName(Seq(1,2).Select(n->n+1)));
  Writeln(Arr(typeof(integer),typeof(string),typeof(Func<integer,string>)).Select(TypeToTypeName).ToArray);
  Writeln(TypeName(nil),' ',TypeToTypeName(default(System.Type)));
end.
