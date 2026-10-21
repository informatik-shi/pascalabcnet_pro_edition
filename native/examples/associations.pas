uses Collections;
begin
  var prices := new AssocArray<string,integer>;
  prices['Яблоки'] := 120;
  prices['Груши'] := 150;
  prices['Яблоки'] := 125;
  Writeln('Цена яблок: ',prices['Яблоки']);
  Writeln('Нет цены: ',prices['Сливы']);
  Writeln('Все цены:');
  prices.Print;
end.
