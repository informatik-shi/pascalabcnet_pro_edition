begin
  PABCSystem.Serialize('serial-int.dat', 42);
  Serialize('serial-string.dat', 'Pascal Я🌍');
  Writeln(integer(PABCSystem.Deserialize('serial-int.dat')), ' ', string(Deserialize('serial-string.dat')));

  var f: file;
  Reset(f, 'serial-int.dat');
  Writeln(f.ReadBytes(64));
  Close(f);
  Erase(f);

  Rewrite(f, 'serial-stream.dat');
  f.Serialize(5);
  f.Serialize('b');
  f.Serialize(byte(7));
  Close(f);
  Reset(f, 'serial-stream.dat');
  var first := integer(f.Deserialize);
  var next := char(f.Deserialize);
  var last := byte(f.Deserialize);
  Writeln(first, ' ', next, ' ', last, ' ', Eof(f));
  Close(f);
  Erase(f);
end.
