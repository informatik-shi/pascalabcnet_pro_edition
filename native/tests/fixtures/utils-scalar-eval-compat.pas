program UtilsScalarEvalSource;
uses Utils;
begin
  Writeln(EvalInt('1+2*3'), ' ', Utils.EvalInt('(1+2)*3'), ' ', EvalInt('5%(-2)'), ' ', EvalInt('-5%2'));
  Writeln(EvalInt('2.5'), ' ', EvalInt('3.5'), ' ', EvalInt('-2.5'), ' ', EvalInt('-3.5'));
  Writeln(EvalReal('1/2'), ' ', EvalReal('1.0/2'), ' ', EvalReal('1e2/4'));
  Writeln(EvalBool('0.1+0.2=0.3'), ' ', EvalBool('NOT 1=2 AND false OR true'), ' ', EvalBool('''false'''));
  Writeln(EvalBool('''Ё'' = ''ё'''), ' ', EvalBool('''01'' = 1'), ' ', EvalBool('''a '' = ''a'''), ' ', EvalBool(''' a'' = ''a'''));
  Writeln(EvalBool('NULL IS NULL'), ' ', EvalBool('NULL IS NOT NULL'), ' ', EvalBool('true or NULL'), ' ', EvalBool('false and NULL'));
  Writeln('[', EvalStr('NULL and false'), '] [', EvalStr(''), '] [', Utils.EvalStr('''a''''b'' + 5'), ']');
  Writeln(EvalStr('2147483648'), ' ', EvalStr('-2147483648'), ' ', EvalStr('1.2345678901234567890123456789'));
  Writeln(EvalStr('1/0'), ' ', EvalStr('0/0'), ' ', EvalBool('0/0'));
  var parse: function(p: string): integer := Utils.EvalInt;
  Writeln(parse(''' 42 '''), ' ', EvalReal(''' 1.25 '''), ' ', EvalStr('''🌍'''))
end.
