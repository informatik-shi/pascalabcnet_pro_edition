type
  Item=class(IComparable<Item>)
  public
    n:integer;
    constructor Create(v:integer);begin n:=v;end;
    function CompareTo(other:Item):integer;
    begin
      Write('C(',n,',');
      if other=nil then Write('nil') else Write(other.n);
      Write(');');
      if n=1 then begin if failMode=1 then begin var zero:=0;var q:=1 div zero;end;end;
      if other=nil then Result:=7 else if failMode=2 then Result:=other.n-n else Result:=n-other.n;
    end;
  end;
  ItemComparer=class(IComparer<Item>)
  public
    function Compare(a,b:Item):integer;
    begin Write('U;');if a=nil then begin if b=nil then Result:=0 else Result:=-1;end else if b=nil then Result:=1 else Result:=a.CompareTo(b);end;
  end;
var scenario,a,b,c,failMode:integer;
function Make(v:integer):Item;begin if v=-99 then Result:=nil else Result:=new Item(v);end;
function CompareItems(a,b:Item):integer;
begin Write('F;');if a=nil then begin if b=nil then Result:=0 else Result:=-8;end else if b=nil then Result:=8 else Result:=a.CompareTo(b);end;
begin
 Readln(scenario,a,b,c,failMode);
 var x:=Make(a);var y:=Make(b);var z:=Make(c);
 if scenario=0 then Write(Comparer<Item>.Default.Compare(x,y));
 if scenario=1 then Write(Comparer<Item>.Create(CompareItems).Compare(x,y));
 if scenario=2 then begin var q:IComparer<Item>:=new ItemComparer;Write(q.Compare(x,y));end;
 if scenario=3 then Write(InRange(x,y,z));
 if scenario=4 then Write(Between(x,y,z));
 if scenario=5 then Write(Between(x,y,z,False));
 if scenario=6 then begin var q:=Clamp(x,y,z);if q=nil then Write('nil') else Write(q.n);end;
 if scenario=7 then Write(x.CompareTo(y));
 if scenario=8 then begin var q:IComparable<Item>:=x;Write(q.CompareTo(y));end;
 if scenario=9 then begin var q:IComparer<Item>:=Comparer<Item>.Default;Write(q.Compare(x,x));end;
 if scenario=10 then Write(Comparer<Item>.Create(CompareItems).Compare(x,x));
 if scenario=11 then Write(Comparer<Item>.Default.Compare(x,x));
end.
