begin
  var w:longword:=MaxLongWord;
  Writeln('Граница 32 бит: ',w);
  Writeln('С расширением: ',w+1);
  var u:uint64:=MaxUInt64;
  Writeln('Граница 64 бит: ',u);
  Writeln('Квадрат word: ',Sqr(MaxWord));
  Dec(u);
  Writeln('Предыдущее: ',u);
  var flag:=false;
  Inc(flag); Write('Переключение: ',flag);
  Dec(flag); Writeln(' ',flag);
end.
