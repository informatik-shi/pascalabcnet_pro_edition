begin
  var prices := DictStrInt(('чай', 120), ('кофе', 180));
  prices['чай'] += 10;
  Writeln(prices);
  var keys := prices.Keys;
  prices.GetOrAdd('сок', key -> 90);
  Writeln(keys);
  var selected := prices + DictStrInt(('чай', 140), ('вода', 60));
  selected -= Seq('кофе', 'сок');
  Writeln(selected);
  Writeln(Seq(2, 1, 2, 3).EachCount);
  Writeln(Seq(1, 2, 3).ToDictionary(x -> x, x -> x * x));
  foreach var pair in prices do
    Writeln(pair.Key, ': ', pair.Value);
end.
