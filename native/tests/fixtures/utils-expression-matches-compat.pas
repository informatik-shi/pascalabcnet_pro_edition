program UtilsLikeMembershipSource;
uses Utils;
begin
  Writeln(EvalBool('''AbC'' LIKE ''a%'''), ' ', EvalBool('''AbC'' LIKE ''*b*'''), ' ', EvalBool('''fooabc'' LIKE ''%ABC'''));
  Writeln(EvalBool('''abc'' LIKE ''a_c'''), ' ', EvalBool('''a*c'' LIKE ''a[*]c'''), ' ', EvalBool('''a%c'' LIKE ''a[%]c'''));
  Writeln(EvalBool('''a[c'' LIKE ''a[[]c'''), ' ', EvalBool('''a]c'' LIKE ''a[]]c'''), ' ', EvalBool('''a'' LIKE ''[a]'''));
  Writeln(EvalBool(''''' LIKE ''%'''), ' ', EvalBool('NULL LIKE ''%'' IS NULL'), ' ', EvalBool('''ab'' NOT LIKE ''%c%'''));
  Writeln(EvalBool(''' A'' LIKE ''A'''), ' ', EvalBool('''A '' LIKE ''A'''), ' ', EvalBool('''Ba'' LIKE ''%a '''));
  Writeln(EvalBool('1 IN (2,1)'), ' ', EvalBool('1 IN (NULL,2)'), ' ', EvalBool('NULL IN (NULL,1) IS NULL'));
  Writeln(EvalBool('1 IN (''01'',2)'), ' ', EvalBool('''01'' IN (1,2)'), ' ', EvalBool('''AB'' IN (''ab'',''AC'')'));
  Writeln(EvalBool('2 IN (1+1,5)'), ' ', EvalBool('2 IN (2.5)'), ' ', EvalBool('3 IN (2.5)'));
  Writeln(EvalBool('1 NOT IN (2,3)'), ' ', EvalBool('NOT (1 IN (2,3))'), ' ', EvalBool('''АБВ'' LIKE ''%б%'''));
  Writeln(EvalBool('''Straße'' LIKE ''%SS%'''), ' ', EvalBool('''ß'' LIKE ''%s%'''), ' ', EvalBool('''XÆ'' LIKE ''%AE'''));
end.
