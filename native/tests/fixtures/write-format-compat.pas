begin
  WriteFormat('[{0,4:D2}]',3);
  PABCSystem.WritelnFormat(' [{0:F1}]',2.25);
  var f: Text;
  Assign(f,'formatted-output-test.txt');
  Rewrite(f);
  WriteFormat(f,'{0}',Cplx(1,2));
  PABCSystem.WritelnFormat(f,' {0:F1}',2.25);
  WritelnFormat(f,'{0} {1}',1,2);
  Close(f);
  Reset(f);
  Writeln(ReadlnString(f));
  Writeln(ReadlnString(f));
  Close(f);
  Erase(f);
end.
