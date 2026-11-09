type P=KeyValuePair<string,integer>;
TraceDictionary=class(IDictionary<string,integer>)
private
 data:Dictionary<string,integer>:=new Dictionary<string,integer>;
 function Size:integer;begin Write('C');Result:=data.Count;end;
 function ReadOnly:boolean;begin Write('O');Result:=False;end;
 function GetKeys:ICollection<string>;begin Write('K');Result:=data.Keys;end;
 function GetValues:ICollection<integer>;begin Write('V');Result:=data.Values;end;
 function GetItem(key:string):integer;begin Write('G');Result:=data[key];end;
 procedure SetItem(key:string;value:integer);begin Write('S');data[key]:=value;end;
public
 constructor(size:integer);begin if size<>0 then begin data.Add('a',1);data.Add('b',2);end;end;
 property Count:integer read Size;
 property IsReadOnly:boolean read ReadOnly;
 property Keys:ICollection<string> read GetKeys;
 property Values:ICollection<integer> read GetValues;
 property Item[key:string]:integer read GetItem write SetItem;default;
 procedure Add(key:string;value:integer);overload;begin Write('A');data.Add(key,value);end;
 procedure Add(pair:P);overload;begin Write('P');var c:ICollection<P>:=data;c.Add(pair);end;
 function Remove(key:string):boolean;overload;begin Write('D');Result:=data.Remove(key);end;
 function Remove(pair:P):boolean;overload;begin Write('R');var c:ICollection<P>:=data;Result:=c.Remove(pair);end;
 procedure Clear;begin Write('L');data.Clear;end;
 function ContainsKey(key:string):boolean;begin Write('F');Result:=data.ContainsKey(key);end;
 function Contains(pair:P):boolean;begin Write('H');var c:ICollection<P>:=data;Result:=c.Contains(pair);end;
 procedure CopyTo(a:array of P;i:integer);begin Write('Y');var c:ICollection<P>:=data;c.CopyTo(a,i);end;
 function TryGetValue(key:string;var value:integer):boolean;begin Write('T');Result:=data.TryGetValue(key,value);end;
 function GetEnumerator:IEnumerator<P>;begin Write('E');Result:=data.AsEnumerable.GetEnumerator;end;
end;
procedure ShowItems(d:IDictionary<string,integer>);begin foreach var p in d do Write(p.Key,'=',p.Value,',');Writeln;end;
begin
 var kind,operation,size,isNil,keyKind,value,offset:integer;Read(kind,operation,size,isNil,keyKind,value,offset);
 var original:=new Dictionary<string,integer>;if size<>0 then begin original.Add('a',1);original.Add('b',2);end;
 var d:IDictionary<string,integer>;if kind=0 then d:=original else d:=new TraceDictionary(size);if isNil<>0 then d:=nil;
 var key:string;if keyKind=0 then key:='a' else if keyKind=1 then key:='x' else key:=nil;
 var pair:=KV&<string,integer>(key,value);
 if operation=0 then Writeln(d.Count)
 else if operation=1 then Writeln(d.IsReadOnly)
 else if operation=2 then begin d.Add(pair);ShowItems(d);end
 else if operation=3 then begin d.Clear;ShowItems(d);end
 else if operation=4 then Writeln(d.Contains(pair))
 else if operation=5 then begin var a:=new P[3];d.CopyTo(a,offset);Writeln(a);end
 else if operation=6 then begin Writeln(d.Remove(pair));ShowItems(d);end
 else if operation=7 then ShowItems(d)
 else if operation=8 then Writeln(d.Keys.Count,' ',d.Keys.IsReadOnly)
 else if operation=9 then Writeln(d.Values.Count,' ',d.Values.Contains(1))
 else if operation=10 then Writeln(d[key])
 else if operation=11 then begin d[key]:=value;ShowItems(d);end
 else if operation=12 then Writeln(d.ContainsKey(key))
 else if operation=13 then begin d.Add(key,value);ShowItems(d);end
 else if operation=14 then begin Writeln(d.Remove(key));ShowItems(d);end
 else if operation=15 then begin var result:=99;Writeln(d.TryGetValue(key,result),' ',result);end
 else if operation=16 then begin var c:ICollection<P>:=d;Writeln(c.Count,' ',c.Contains(pair));end
 else if operation=17 then begin var e:IEnumerable<P>:=d;Writeln(e.Count);end
 else if operation=18 then begin var e:IEnumerable<P>:=d;Writeln(e.ToArray);end
 else if operation=19 then begin d[key]+=value;ShowItems(d);end
 else if operation=20 then Writeln(d.Keys.Remove(key));
end.
