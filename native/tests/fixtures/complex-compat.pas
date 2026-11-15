uses System.Numerics;

begin
  var z: Complex;
  z := (3,4);
  var alias := z;
  var n: System.Numerics.Complex := 2;
  Writeln(z.Real,' ',z.Imaginary,' ',Abs(z),' ',z.Magnitude);
  Writeln(z.ToString,' ',z,' ',z.GetType.FullName);
  z += (1,-2);
  Writeln(alias,' ',z,' ',n+z,' ',z-n,' ',-z);
  Writeln(z*n,' ',z/n,' ',Complex.Divide(Cplx(3,4),Cplx(1,1)));
  Writeln(PABCSystem.Cplx(3,-4).Conjugate,' ',Conjugate(z));
  Writeln(Complex.Zero,' ',Complex.One,' ',Complex.ImaginaryOne);
  Writeln(new Complex(1,2),' ',Complex.Create(2,1),' ',Complex.Reciprocal(Complex.Zero),' ',Complex.One.Phase);
  Writeln(n=2,' ',z<>alias,' ',alias.Equals(Cplx(3,4)));
  var tupled := (0,2.5);
  var i: Complex := tupled;
  Writeln(i,' ',Complex.FromPolarCoordinates(2,0));
  Writeln(Abs(Complex.Sqrt(Complex.ImaginaryOne)*Complex.Sqrt(Complex.ImaginaryOne)-Complex.ImaginaryOne)<1e-12);
  Writeln(Abs(Exp(Ln(alias))-alias)<1e-12,' ',Abs(Power(alias,2)-alias*alias)<1e-12);
  Writeln(Complex.Zero**Complex.Zero,' ',default(Complex));
  Writeln(Abs(Sin(Complex.Zero))=0,' ',Cos(Complex.Zero)=Complex.One);
  Writeln(-2**2,' ',2**3**2,' ',2**(-2),' ',2bi**6);
end.
