begin
  var a:=Arr(1,2,3,4);
  Writeln(a.Sum,' ',a.Average,' ',a.Product);
  Writeln(Sum(a),' ',PABCSystem.Average(a),' ',Product(a));
  Writeln(Arr(MaxInt,MaxInt).Average,' ',Arr(MaxInt,MaxInt).Product);
  Writeln(Arr(int64(1),int64(-2),int64(4)).Sum);
  Writeln(Arr(0.25,0.5,0.75).Sum,' ',Arr(0.25,0.5,0.75).Product);
  Writeln(Arr(1bi,2bi).Sum,' ',Arr(1bi,2bi).Average,' ',Arr(-3bi,7bi).Product);
  Writeln((new integer[0]).Sum,' ',(new integer[0]).Product,' ',(new BigInteger[0]).Average);
  Writeln(Range(1,4).Sum,' ',Range(1,4).Average,' ',Range(1,4).Product);
  Writeln(Arr(1e16,1.0,-1e16).Sum);
end.
