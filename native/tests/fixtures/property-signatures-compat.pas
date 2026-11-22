program PropertySignaturesOracle;
uses System;
type
  Sample=class
  private
    i:integer; b:boolean; s:string; ch:char;
    a:array of integer; ss:array of string; l:List<integer>; dt:DateTime; g:Guid; o:object;
  public
    property Number:integer read i;
    property Flag:boolean read b;
    property Text:string read s;
    property Character:char read ch;
    property Integers:array of integer read a;
    property Strings:array of string read ss;
    property Items:List<integer> read l;
    property Date:DateTime read dt;
    property Identity:Guid read g;
    property Box:object read o;
  end;
begin
  foreach var p in typeof(Sample).GetProperties do
  begin
    Writeln(p.ToString);
    Writeln(p);
  end;
end.
