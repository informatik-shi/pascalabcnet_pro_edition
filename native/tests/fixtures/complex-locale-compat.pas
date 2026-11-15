begin
  var z:=Cplx(0.25,-0.5);
  var boxed:object:=z;
  Writeln(z,' ',boxed,' ',z.ToString,' ',Format('{0}',boxed));
  SetDecimalSeparator(',');
  Writeln(z,' ',boxed,' ',z.ToString,' ',Format('{0}',boxed));
  NumberFormatInfo.CurrentInfo.NumberDecimalSeparator:='::';
  Writeln(z,' ',boxed,' ',z.ToString,' ',Format('{0}',boxed));
end.
