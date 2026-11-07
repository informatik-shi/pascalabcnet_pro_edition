type Small=(N3,N2,N1,Z,P1,P2,P3);
var scenario,size,seed:integer;k,v:array of Small;ids:array of integer;
function Dump(a:array of Small):string;
begin if a=nil then begin Result:='nil';Exit;end;Result:='[';for var i:=0 to a.Length-1 do begin if i<>0 then Result+=',';Result+=IntToStr(integer(a[i]));end;Result+=']';end;
function IntDump(a:array of integer):string;
begin Result:='[';for var i:=0 to a.Length-1 do begin if i<>0 then Result+=',';Result+=IntToStr(a[i]);end;Result+=']';end;
begin
 Readln(scenario,size,seed);k:=new Small[size];v:=new Small[size];ids:=new integer[size];
 for var i:=0 to size-1 do begin k[i]:=Small((i*17+seed*13) mod 7);v[i]:=Small((i*3+seed) mod 7);ids[i]:=100+i;end;
 if scenario=1 then v:=k;if scenario=3 then v:=nil;
 if (scenario=0) or (scenario=1) or (scenario=3) then Array.Sort(k,v);
 if scenario=2 then Array.Sort(k,ids);
 if scenario=4 then Array.Sort(k);
 if scenario=5 then begin var l:=new List<Small>(size+7);l.AddRange(k);l.Sort;k:=l.ToArray;end;
 if scenario=6 then Array.Sort(k,v,if size>1 then 1 else 0,Max(0,size-2));
 Write(Dump(k),' ',if scenario=2 then IntDump(ids) else if (scenario=4) or (scenario=5) then 'nil' else Dump(v));
end.
