type
 Item=class(IEquatable<Item>)
 public id:integer;
  constructor Create(value:integer);begin id:=value;end;
  function Equals(other:Item):boolean:=id=other.id;
 end;
 Parity=class(IEqualityComparer<integer>)
 public function Equals(a,b:integer):boolean:=a mod 2=b mod 2;
  function GetHashCode(value:integer):integer:=value mod 2;
 end;
begin
 var integers:=EqualityComparer<integer>.Default;
 Writeln(integers.Equals(7,7),' ',integers.GetHashCode(7));
 var a:=new Item(7);var b:=new Item(7);
 Writeln(EqualityComparer<Item>.Default.Equals(a,b));
 var known:IEquatable<Item>:=a;
 Writeln(Item(known)=a);
 var parity:IEqualityComparer<integer>:=new Parity;
 Writeln(parity.Equals(1,3),' ',parity.GetHashCode(5));
end.
