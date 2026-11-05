begin
  var numbers: (integer,integer,integer) := ReadlnInteger3('Три числа:');
  Writeln(numbers,' sum=',numbers.Item1+numbers.Item2+numbers.Item3);
  Writeln(ReadReal2('Два вещественных:'));
  Readln;
  var letters := ReadlnChar2('Две буквы:');
  Writeln(letters);
  Writeln(ReadlnString2('Две строки:'));
end.
