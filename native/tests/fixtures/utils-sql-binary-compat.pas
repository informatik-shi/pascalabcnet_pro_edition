program UtilsSqlBinaryCompatibility;
uses Utils, System, System.Data.SqlTypes;

begin
  var input:=Arr(byte(1),byte(255));
  var value:=new SqlBinary(input);
  input[0]:=12;
  var snapshot:=value.Value;
  snapshot[1]:=0;
  Writeln(value.ToString,' ',value.Length,' ',value[0],' ',value[1]);

  var padded:=new SqlBinary(Arr(byte(1),byte(255),byte(0),byte(0)));
  Writeln(value.CompareTo(padded),' ',value.Equals(padded),' ',value.GetHashCode,' ',padded.GetHashCode);
  var longer:=SqlBinary.Concat(value,padded);
  Writeln(longer.ToString,' ',longer[0],' ',longer[2],' ',longer[5]);
  Writeln(SqlBinary.Add(value,value).Length,' ',SqlBinary.Concat(value,SqlBinary.Null).IsNull);

  var missingBytes: array of byte;
  var empty:=new SqlBinary(new byte[0]);
  Writeln((new SqlBinary(missingBytes)).IsNull,' ',empty.IsNull,' ',empty.Length);
  Writeln((new SqlBinary(nil)).IsNull,' ',SqlBinary.Null.ToString,' ',empty.Equals(SqlBinary.Null));

  var lookup:=new Dictionary<SqlBinary,String>;
  lookup.Add(value,'same bytes');
  Writeln(lookup[padded]);
  var s:=Arr(value,empty);
  Sort(s);
  Writeln(s[0].ToString);

  var guid:=Guid.Parse('00112233-4455-6677-8899-aabbccddeeff');
  var converted:=new SqlBinary(guid.ToByteArray);
  Writeln(converted.GetType.FullName,' ',converted.ToSqlGuid.Value.ToString);
  Writeln(SqlBinary.Null.ToSqlGuid.IsNull);
  Writeln(Eval('CONVERT(NULL,System.Data.SqlTypes.SqlBinary)').GetType.FullName);
  Writeln(Eval('CONVERT(NULL,''System.Data.SqlTypes.SqlBinary[]'')').GetType.FullName);
  Writeln(Eval('CONVERT(NULL,System.Data.SqlTypes.SqlXml)').GetType.FullName);
end.
