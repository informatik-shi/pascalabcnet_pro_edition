begin
  Writeln(12 and 10,' ',12 or 10,' ',12 xor 10,' ',not 0);
  Writeln(-8 shr 1,' ',1 shl 31,' ',1 shl 32,' ',1 shl -1);
  var a:byte:=240; var b:byte:=15;
  var c:byte:=a or b;
  Writeln(c,' ',not a,' ',a and b,' ',a shl 8);
  var u:longword:=MaxLongWord;
  Writeln(u shr 31,' ',not u);
  var w:uint64:=MaxUInt64;
  Writeln(w shr 63,' ',w and 255,' ',w xor w);
  Writeln(true xor false,' ',true xor true);
end.
