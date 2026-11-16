program UtilsSqlGuidCompatibility;
uses Utils, System, System.Data.SqlTypes;

type TSqlGuid = System.Data.SqlTypes.SqlGuid;

begin
  var highFirst:=SqlGuid.Parse('ffffffff-ffff-ffff-ffff-000000000000');
  var highLast:=SqlGuid.Parse('00000000-0000-0000-0000-000000000001');
  Writeln(highFirst.CompareTo(highLast),' ',highFirst.Value.CompareTo(highLast.Value));
  Writeln(highFirst.Equals(highLast),' ',highFirst.Equals(SqlGuid.Parse(highFirst.ToString)));
  Writeln(SqlGuid.Null.ToString,' ',SqlGuid.Parse('Null').IsNull,' ',SqlGuid.Null.CompareTo(highFirst),' ',SqlGuid.Null.GetHashCode);
  Writeln(highLast.GetHashCode);
  var source:=highFirst.ToByteArray;
  Writeln(source.Length,' ',source[0],' ',source[15]);
  var copy:=new SqlGuid(source);
  source[0]:=0;
  Writeln(copy.ToString,' ',copy.ToByteArray[0]);
  Writeln((new SqlGuid(highLast.Value)).ToString);
  Writeln((new SqlGuid('00112233445566778899AABBCCDDEEFF')).Value.ToString);
  var unsorted:=Arr(highLast,highFirst);
  Sort(unsorted);
  Writeln(unsorted[0].ToString);
  var map:=new Dictionary<SqlGuid,String>;
  map.Add(highLast,'sorted key');
  Writeln(map[SqlGuid.Parse(highLast.ToString)]);
  var boxed:=Eval('CONVERT(CONVERT(''00000000-0000-0000-0000-000000000001'',System.Guid),System.Data.SqlTypes.SqlGuid)');
  var unboxed:=TSqlGuid(boxed);
  Writeln(boxed.GetType.FullName,' ',unboxed.ToString);
  Writeln(EvalStr('CONVERT(CONVERT(''ffffffff-ffff-ffff-ffff-000000000000'',System.Guid),System.Data.SqlTypes.SqlGuid)<CONVERT(CONVERT(''00000000-0000-0000-0000-000000000001'',System.Guid),System.Data.SqlTypes.SqlGuid)'));
end.
