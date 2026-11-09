uses School;
type TraceStrings=class(IList<string>)
private
 label:string;
 data:List<string>;
 function Size:integer;begin Write(label,'C;');Result:=data.Count;end;
 function ReadOnly:boolean:=False;
 function GetItem(i:integer):string;begin Write(label,'G',i,';');Result:=data[i];end;
 procedure SetItem(i:integer;value:string);begin data[i]:=value;end;
public
 constructor(name:string;items:array of string);begin label:=name;data:=new List<string>(items);end;
 property Count:integer read Size;
 property IsReadOnly:boolean read ReadOnly;
 property Item[i:integer]:string read GetItem write SetItem; default;
 procedure Add(x:string);begin data.Add(x);end;
 procedure Clear;begin data.Clear;end;
 function Contains(x:string):boolean:=data.Contains(x);
 procedure CopyTo(a:array of string;i:integer);begin data.CopyTo(a,i);end;
 function Remove(x:string):boolean:=data.Remove(x);
 function IndexOf(x:string):integer:=data.IndexOf(x);
 procedure Insert(i:integer;x:string);begin data.Insert(i,x);end;
 procedure RemoveAt(i:integer);begin data.RemoveAt(i);end;
 function GetEnumerator:IEnumerator<string>;begin Write(label,'E;');Result:=data.AsEnumerable.GetEnumerator;end;
end;
begin
 var sk,tk,sd,td,sn,tn,textNil:integer;Read(sk,tk,sd,td,sn,tn,textNil);
 var sourceData:=new string[2];sourceData[0]:='a';sourceData[1]:='b';
 if sd=1 then sourceData[1]:='' else if sd=2 then sourceData[0]:=nil else if sd=3 then sourceData:=new string[0];
 var targetData:=new string[2];targetData[0]:='b';targetData[1]:='x';
 if td=1 then begin targetData:=new string[1];targetData[0]:='z';end else if td=2 then targetData:=new string[0] else if td=3 then targetData[0]:=nil;
 var source,target:IList<string>;
 if sk=0 then source:=sourceData else if sk=1 then source:=new List<string>(sourceData) else source:=new TraceStrings('S',sourceData);
 if tk=0 then target:=targetData else if tk=1 then target:=new List<string>(targetData) else target:=new TraceStrings('T',targetData);
 if sn<>0 then source:=nil;if tn<>0 then target:=nil;
 var text:string:='aabbaba';if textNil<>0 then text:=nil;
 Writeln(ReplaceMany(text,source,target));
end.
