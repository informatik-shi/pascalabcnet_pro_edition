type IA=interface
 property Value:integer read write;
 property Item[i:integer]:integer read write;default;
 property Flag:boolean read;
 property Computed:integer read;
 property Fn:integer->integer read;
 property Enumerator:integer read;
 property KeyCollection:string read;
end;
IB=interface property Value:integer read write;end;
IChild=interface(IA)end;
IResolved=interface(IA,IB)property Value:integer read write;end;
AliasA=IA;
C=class(IChild,IB,IResolved)
private
 a,b,plain,resolved:integer;
 function GetA:integer;begin Write('A');Result:=a;end;
 procedure SetA(n:integer);begin Write('a');a:=n;end;
 function GetResolved:integer;begin Write('C');Result:=resolved;end;
 procedure SetResolved(n:integer);begin Write('c');resolved:=n;end;
 procedure SetItem(i,n:integer);begin Write('S');a:=n-i;end;
 property AliasA.Value:integer read GetA write SetA(value);
 property IB.Value:integer read b write b:=value;
 property IResolved.Value:integer read GetResolved write begin SetResolved(value);end;
 property IA.Item[i:integer]:integer read Trace('G',a+i) write begin Write('S');a:=value-i;end;
 property IA.Flag:boolean read boolean(true);
 property IA.Computed:integer read Trace('Q',Self.Forward+a+Bias);
 property IA.Fn:integer->integer read (n->n+a+Bias);
 property IA.Enumerator:integer read 7;
 property IA.KeyCollection:string read 'keys';
public
 constructor(seed:integer);begin a:=seed;b:=seed+1;plain:=seed+2;resolved:=seed+3;end;
 property Value:integer read plain write plain;
 property Forward:integer read 40;
end;
function Trace(tag:string;value:integer):integer;begin Write(tag);Result:=value;end;
var Bias:integer;
begin
 var seed,route,isNil:integer;Read(seed,route,isNil);Bias:=seed+2;
 var c:=new C(seed);var other:=new C(seed+10);var a:IChild:=c;var b:IB:=c;var resolved:IResolved:=c;
 if route=1 then a:=other else if route=2 then a:=IChild(b);if isNil<>0 then a:=nil;
 Write('Start');resolved.Value:=seed+20;
 Writeln(c.Value,' ',a.Value,' ',b.Value,' ',resolved.Value,' ',a.Flag,' ',a.Enumerator,' ',a.KeyCollection);
 a[2]+=5;a.Value+=a[1];var f:=a.Fn;
 Writeln(a.Value,' ',a.Computed,' ',a[2],' ',f(4));
 Inc(Bias);a:=nil;b:=nil;resolved:=nil;c:=nil;other:=nil;Writeln(f(5));
end.
