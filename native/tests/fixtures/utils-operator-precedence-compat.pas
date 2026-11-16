program UtilsOperatorPrecedenceSource;
uses Utils;
begin
  var cases:=Arr('2%3*2',
    '8%3*2',
    '9*3%8',
    '5+13%5*2',
    '2+1 IN (3,5)',
    '(2+1) NOT IN (3,5)',
    '-1 IN (1)',
    '(-1) NOT IN (1)',
    '1 NOT = 2',
    '(1) NOT = (''2'')',
    '(TRUE) NOT IN (TRUE)',
    '(TRUE) NOT',
    '(FALSE) NOT NOT',
    'NOT ''false ''',
    '''true'' NOT',
    '(NULL) NOT IN (1)',
    '(NULL) NOT LIKE ''a''',
    '1+NULL NOT IN (1)',
    'CONVERT(NULL,System.Int32) NOT IN (1)',
    '1 > 2 IS NULL',
    '1 IS NULL IS NULL',
    'NOT FALSE IS NULL',
    '(1 IN (1)) = TRUE',
    '(''x'' LIKE ''x'') = TRUE',
    '(''x'' NOT LIKE ''x'') = FALSE',
    '(1 IN (1)) AND TRUE');
  foreach var item in cases do Writeln('[',EvalStr(item),']')
end.
