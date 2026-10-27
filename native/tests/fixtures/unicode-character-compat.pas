begin
  Writeln(LowerCase('ЯЁÄΣ𐐀'),' ',UpperCase('яёäς𐐨'));
  Writeln(UpCase('я'),' ',LowCase('Ё'),' ','Я'.Code);
  Writeln('Я'.IsLetter,' ','字'.IsLetter,' ','Ⅳ'.IsLetter);
  Writeln('١'.IsDigit,' ','９'.IsDigit,' ','²'.IsDigit);
  Writeln('ß'.IsLower,' ','Σ'.IsUpper,' ','ǅ'.IsUpper);
  Writeln(Ord(LowerCase(#55296)),' ',Ord(UpperCase(#56320)));
end.
