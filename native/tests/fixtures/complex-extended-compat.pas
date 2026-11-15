begin
  var z:=Cplx(0.25,-0.5);
  Writeln(Abs(Complex.Tan(z)-Sin(z)/Cos(z))<1e-13);
  Writeln(Abs(Complex.Tanh(z)-Complex.Sinh(z)/Complex.Cosh(z))<1e-13);
  Writeln(Abs(Sin(Complex.Asin(z))-z)<1e-13);
  Writeln(Abs(Cos(Complex.Acos(z))-z)<1e-13);
  Writeln(Abs(Complex.Tan(Complex.Atan(z))-z)<1e-13);
  var bad:=Complex.One/Complex.Zero;
  Writeln(bad.ToString,' ',bad.Equals(bad),' ',bad=bad,' ',bad<>bad);
  Writeln(Complex.Reciprocal(Complex.Zero));
  Writeln(Complex.Zero.GetHashCode,' ',Complex.One.GetHashCode,' ',Complex.ImaginaryOne.GetHashCode);
  Writeln(Cplx(3,4).GetHashCode,' ',Cplx(0,-1).GetHashCode);
  var boxed: object := z;
  Writeln(boxed.GetType.FullName,' ',Complex(boxed)=z);
  Writeln(Format('{0}',boxed));
  var wrong: object := 1;
  var nanBox: object := bad;
  Writeln(z.Equals(boxed),' ',z.Equals(wrong),' ',bad.Equals(nanBox));
  Writeln(z.ToString('F3'));
  var provider := NumberFormat('::','_');
  provider.NegativeSign := '~';
  Writeln(z.ToString(provider),' ',z.ToString('F3',provider));
  Writeln(Format('{0:F3}',z));
  z := Cplx(1,2);
  Writeln(Complex(boxed).ToString);
end.
