uses School;
begin
  Writeln(ReplaceMany('ab','ab','bc'),' ',Translate('ab','ab','bc'));
  var s := 'ааа';
  ReplaceLast(s,'аа','X');
  Writeln(s);
  Writeln(ReplaceMany('abab',Arr('ab','X'),Arr('X'+'','Y')));
  Writeln(ReplaceMany('aaa',Arr('aa'),Arr('X'+'')));
  Writeln('2147483648 -9223372036854775808'.ToInts64);
  Writeln(Imp(False,False),' ',False.Imp(True),' ',True.Imp(False),' ',True.Imp(True));
end.
