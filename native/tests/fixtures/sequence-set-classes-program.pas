type
 Item=class(IEquatable<Item>) public function Equals(x:Item):boolean;begin Write('E(',Tag(Self),',',Tag(x),');');Result:=not reject and (x<>nil);end;end;
 Order=class(IEqualityComparer<Item>) public function Equals(a,b:Item):boolean;begin Write('C(',Tag(a),',',Tag(b),');');Result:=(policy<>3) and (if a=nil then b=nil else b<>nil);end;function GetHashCode(a:Item):integer;begin Write('H;');Result:=0;end;end;
 InterfaceOrder=class(IEqualityComparer<IEquatable<Item>>) public function Equals(a,b:IEquatable<Item>):boolean;begin Write('I(',ITag(a),',',ITag(b),');');Result:=(policy<>3) and (if a=nil then b=nil else b<>nil);end;function GetHashCode(a:IEquatable<Item>):integer;begin Write('IH;');Result:=0;end;end;
var method,policy:integer;reject:boolean;leftClass,rightClass:sequence of Item;leftInterface,rightInterface:sequence of IEquatable<Item>;policyClass:IEqualityComparer<Item>;policyInterface:IEqualityComparer<IEquatable<Item>>;
function Tag(x:Item):string:=if x=nil then 'nil' else 'obj';
function ITag(x:IEquatable<Item>):string:=if x=nil then 'nil' else 'obj';
function LClass:sequence of Item;begin Write('L;');Result:=leftClass;end;
function RClass:sequence of Item;begin Write('R;');Result:=rightClass;end;
function PClass:IEqualityComparer<Item>;begin Write('P;');Result:=policyClass;end;
function QClass(style:integer):sequence of Item;
begin
 if method=0 then begin if policy=0 then begin if style=0 then Result:=Distinct(LClass) else Result:=LClass().Distinct;end else begin if style=0 then Result:=Distinct(LClass,PClass) else Result:=LClass().Distinct(PClass);end;exit;end;
 if method=1 then begin if policy=0 then begin if style=0 then Result:=Union(LClass,RClass) else Result:=LClass().Union(RClass);end else begin if style=0 then Result:=Union(LClass,RClass,PClass) else Result:=LClass().Union(RClass,PClass);end;exit;end;
 if method=2 then begin if policy=0 then begin if style=0 then Result:=Intersect(LClass,RClass) else Result:=LClass().Intersect(RClass);end else begin if style=0 then Result:=Intersect(LClass,RClass,PClass) else Result:=LClass().Intersect(RClass,PClass);end;exit;end;
 if method=3 then begin if policy=0 then begin if style=0 then Result:=&Except(LClass,RClass) else Result:=LClass().Except(RClass);end else begin if style=0 then Result:=&Except(LClass,RClass,PClass) else Result:=LClass().Except(RClass,PClass);end;exit;end;
end;
procedure WalkClass(style:integer);begin var q:=QClass(style);Write('Q;');foreach var x in q do Write('Y',Tag(x),';');Write('DONE;');end;
function LInterface:sequence of IEquatable<Item>;begin Write('L;');Result:=leftInterface;end;
function RInterface:sequence of IEquatable<Item>;begin Write('R;');Result:=rightInterface;end;
function PInterface:IEqualityComparer<IEquatable<Item>>;begin Write('P;');Result:=policyInterface;end;
function QInterface(style:integer):sequence of IEquatable<Item>;
begin
 if method=0 then begin if policy=0 then begin if style=0 then Result:=Distinct(LInterface) else Result:=LInterface().Distinct;end else begin if style=0 then Result:=Distinct(LInterface,PInterface) else Result:=LInterface().Distinct(PInterface);end;exit;end;
 if method=1 then begin if policy=0 then begin if style=0 then Result:=Union(LInterface,RInterface) else Result:=LInterface().Union(RInterface);end else begin if style=0 then Result:=Union(LInterface,RInterface,PInterface) else Result:=LInterface().Union(RInterface,PInterface);end;exit;end;
 if method=2 then begin if policy=0 then begin if style=0 then Result:=Intersect(LInterface,RInterface) else Result:=LInterface().Intersect(RInterface);end else begin if style=0 then Result:=Intersect(LInterface,RInterface,PInterface) else Result:=LInterface().Intersect(RInterface,PInterface);end;exit;end;
 if method=3 then begin if policy=0 then begin if style=0 then Result:=&Except(LInterface,RInterface) else Result:=LInterface().Except(RInterface);end else begin if style=0 then Result:=&Except(LInterface,RInterface,PInterface) else Result:=LInterface().Except(RInterface,PInterface);end;exit;end;
end;
procedure WalkInterface(style:integer);begin var q:=QInterface(style);Write('Q;');foreach var x in q do Write('Y',ITag(x),';');Write('DONE;');end;
begin var kind,nl,nr,no,style:integer;Readln(method,kind,nl,nr,policy,no,style);reject:=no<>0;var value:=new Item;var a:=new Item[Max(0,nl)];var b:=new Item[Max(0,nr)];for var n:=0 to a.Length-1 do a[n]:=if n mod 3=1 then nil else value;for var n:=0 to b.Length-1 do b[n]:=if n mod 2=0 then nil else value;
 if kind=0 then begin leftClass:=if nl<0 then nil else a;rightClass:=if nr<0 then nil else b;if policy=1 then policyClass:=nil else if policy=4 then policyClass:=EqualityComparer<Item>.Default else policyClass:=new Order;WalkClass(style);end else begin var ai:=new IEquatable<Item>[a.Length];var bi:=new IEquatable<Item>[b.Length];for var n:=0 to a.Length-1 do ai[n]:=a[n];for var n:=0 to b.Length-1 do bi[n]:=b[n];leftInterface:=if nl<0 then nil else ai;rightInterface:=if nr<0 then nil else bi;if policy=1 then policyInterface:=nil else if policy=4 then policyInterface:=EqualityComparer<IEquatable<Item>>.Default else policyInterface:=new InterfaceOrder;WalkInterface(style);end;
end.
