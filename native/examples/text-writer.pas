begin
  var f := OpenWrite('writer-demo.txt',Encoding.UTF8);
  f.Write('buffer');
  f.Rewrite(Encoding.ASCII); // Existing writer keeps its encoding and pending characters.
  f.Write('ed');
  f.Close;
  Writeln(ReadAllText('writer-demo.txt'));
  f.Rewrite(Encoding.ASCII);
  f.Write('abcdef');
  f.Flush;
  f.Rewrite;
  f.Write('XY');
  f.Close;
  Writeln(ReadAllText('writer-demo.txt'));
  f.Append;
  f.Write('Z');
  f.Close;
  Writeln(ReadAllText('writer-demo.txt'));
  Writeln(DeleteFile('writer-demo.txt'));
end.
