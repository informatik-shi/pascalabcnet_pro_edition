type Strings = array of string;

begin
  var f: file;
  Rewrite(f, 'reference-strings.dat');
  f.WriteBytes(new byte[68](
    0,1,0,0,0,255,255,255,255,1,0,0,0,0,0,0,
    0,17,1,0,0,0,7,0,0,0,6,2,0,0,0,5,
    102,105,114,115,116,13,2,6,3,0,0,0,6,208,175,240,
    159,140,141,10,6,4,0,0,0,4,108,97,115,116,9,2,
    0,0,0,11
  ));
  Close(f);
  var from_file := Strings(Deserialize('reference-strings.dat'));
  Writeln(from_file[0], ' ', from_file[1]=nil, ' ', from_file[2]=nil, ' ', from_file[3],
    ' ', from_file[4]=nil, ' ', from_file[5], ' ', from_file[6]);
  from_file[6] := 'changed';
  assert(from_file[0]='first');

  Serialize('new-strings.dat', Arr('alpha','βeta','last'));
  Reset(f, 'new-strings.dat');
  Writeln(f.ReadBytes(80));
  Close(f);

  var many := new string[300];
  for var i:=0 to many.Length-1 do many[i]:=nil;
  Serialize('many-nil-strings.dat', many);
  var nil_strings := Strings(Deserialize('many-nil-strings.dat'));
  Writeln(nil_strings.Length, ' ', nil_strings[299]=nil);
end.
