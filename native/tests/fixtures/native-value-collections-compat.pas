uses System;
begin
  var amounts:=new HashSet<Decimal>;
  Writeln(amounts.Add(Decimal.Parse('3.00')),' ',amounts.Add(Decimal.Parse('3.0')),
    ' ',amounts.Contains(Decimal(3)),' ',amounts.Count);
  var amountsByLabel:=new Dictionary<Decimal,string>;
  amountsByLabel.Add(Decimal.Parse('2.00'),'two');
  Writeln(amountsByLabel[Decimal.Parse('2.000')]);
  var dates:=new HashSet<DateTime>;
  var d:=new DateTime(2024,2,29);
  var utc:=DateTime.SpecifyKind(d,DateTimeKind.Utc);
  Writeln(dates.Add(d),' ',dates.Add(utc),' ',dates.Contains(utc),' ',dates.Count);
  var numbers:=new HashSet<Complex>;
  var n:=Complex.One/Complex.Zero;
  Writeln(numbers.Add(n),' ',numbers.Add(n),' ',numbers.Contains(n),' ',numbers.Count);
  Writeln(System.Collections.Generic.EqualityComparer&<Decimal>.Default.Equals(Decimal.Parse('3.00'),Decimal(3)));
  Writeln(System.Collections.Generic.Comparer&<Decimal>.Default.Compare(Decimal.Parse('3.00'),Decimal(3)));
  Writeln(System.Collections.Generic.Comparer&<DateTime>.Default.Compare(d,utc),' ',
    System.Collections.Generic.Comparer&<TimeSpan>.Default.Compare(TimeSpan.FromTicks(-2),TimeSpan.Zero));
  var encoded: file of int64;
  Assign(encoded,'different-nans.bin');
  Rewrite(encoded);
  Write(encoded,9221120237041090561,9221120237041090609);
  Close(encoded);
  var doubles: file of real;
  Assign(doubles,'different-nans.bin');
  Reset(doubles);
  var a,b: real;
  Read(doubles,a,b);
  Close(doubles);
  var x:=Cplx(a,1);
  var y:=Cplx(b,1);
  var nanNumbers:=new HashSet<Complex>;
  Writeln(x.Equals(y),' ',x.GetHashCode,' ',y.GetHashCode);
  Writeln(nanNumbers.Add(x),' ',nanNumbers.Add(y),' ',nanNumbers.Count);
end.
