begin
  Print(1,2); Println(3,4);
  PrintDelimDefault:='|';
  Println(Arr(1,2),new integer[0,2]);
  var n:=ReadInteger('N'); Println(n,n+1);
  var f:=OpenWrite('print-test.txt');
  Print(f,1,2); Println(f,3,4);
  PrintDelimDefault:='/';
  f.Print(5,6); f.Println(7,8);
  PrintDelimDefault:='';
  f.Print(9,0); f.Println(1,2);
  Close(f); Reset(f);
  Writeln(f.ReadlnString); Writeln(f.ReadlnString); Writeln(f.ReadlnString);
  Close(f); Erase(f);
  PABCSystem.PrintDelimDefault:=';';
  PABCSystem.Println(1.5:5:1,7:3);
end.
