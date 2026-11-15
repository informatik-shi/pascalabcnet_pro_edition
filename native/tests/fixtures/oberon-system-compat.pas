program OberonStore;
uses Oberon00System;
type
  Section = class
  end;
  Sample = record
    item: integer;
  end;
  Color = (black, red);
begin
  var a: SHORTINT := 200;
  var b: Oberon00System.LONGINT := int64(5000000000);
  var r: LONGREAL := 2.5;
  var items: iset := Arr(3,4);
  Writeln(typeof(SHORTINT).Name,' ',typeof(LONGINT).Name,' ',typeof(LONGREAL).Name);
  Writeln(a,' ',b,' ',r,' ',items.Length);
  Oberon00System.Print('tag ');
  Oberon00System.Println(4,5);
  Print('one');
  Println(6,7);
  Println(1,2,3);
  Oberon00System.Println(nil);
  Oberon00System.Println;
  Oberon00System.Println(items);
  Oberon00System.Println(new List<integer>);
  Oberon00System.Println((1,2));
  Oberon00System.Println(new Section);
  var row: Sample;
  Oberon00System.Println(row);
  Oberon00System.Println(Color.red);
  Oberon00System.Println(KV&<integer,string>(3,'three'));
  Oberon00System.Println(Oberon00System.SHORTINT(199));
  Print(8);
  Writeln('|');
  Print(nil);
  Writeln('|');
end.
