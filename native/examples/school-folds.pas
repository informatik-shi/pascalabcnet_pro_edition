uses School;
begin
  var numbers := Arr(int64(18),30,42);
  Writeln('НОД массива: ',GCD(numbers));
  Writeln('НОК массива: ',LCM(numbers));
  Writeln('НОД нескольких чисел: ',GCD(18,30,42,54));
  Writeln('НОК нескольких чисел: ',LCM(2,3,4,5));
end.
