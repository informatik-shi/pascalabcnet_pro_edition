begin
  var текст := 'Привет, Ёжик! 𐐀';
  Writeln('Нижний регистр: ',текст.ToLower);
  Writeln('Верхний регистр: ',текст.ToUpper);
  var символ := 'Я';
  Writeln('Буква: ',символ.IsLetter,'; код: ',символ.Code);
  Writeln('Арабская цифра: ','١'.IsDigit);
end.
