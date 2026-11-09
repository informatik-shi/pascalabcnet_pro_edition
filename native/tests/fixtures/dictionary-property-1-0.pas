var a,b,root:Dictionary<string,real>;
 receiverMode,keyMode,aliasMode,rhsMode,seed,receiverCalls,keyCalls:integer;
function Receiver:Dictionary<string,real>;
begin
 Inc(receiverCalls);Write('D',receiverCalls,';');
 if receiverMode=0 then Result:=root
 else if receiverMode=1 then Result:=if receiverCalls=1 then a else b
 else if receiverMode=2 then Result:=if receiverCalls=1 then b else a
 else if receiverMode=3 then Result:=if receiverCalls=1 then nil else a
 else if receiverMode=4 then Result:=if receiverCalls=1 then a else nil
 else Result:=nil;
end;
function Key:string;
begin
 Inc(keyCalls);Write('K',keyCalls,';');
 if keyMode=0 then Result:='a'
 else if keyMode=1 then Result:=if keyCalls=1 then 'a' else 'b'
 else if keyMode=2 then Result:=if keyCalls=1 then 'b' else 'a'
 else if keyMode=3 then begin if keyCalls=1 then Result:=nil else Result:='a';end
 else if keyMode=4 then begin if keyCalls=1 then Result:='a' else Result:=nil;end
 else if keyMode=5 then Result:=if keyCalls=1 then 'missing' else 'a'
 else if keyMode=6 then Result:=if keyCalls=1 then 'a' else 'missing'
 else Result:='missing';
end;
function Rhs:real;
begin
 Write('R;');
 if rhsMode=1 then root:=b else if rhsMode=2 then a.Clear else if rhsMode=3 then begin a['a']:=99.0; end;
 Result:=2.0;
end;
begin
 Read(receiverMode,keyMode,aliasMode,rhsMode,seed);
 a:=new Dictionary<string,real>;a.Add('a',2.5);a.Add('b',3.0);
 b:=new Dictionary<string,real>;b.Add('a',20.5);b.Add('b',30.0);
 if seed=1 then a['a']:=0.0;
 if aliasMode<>0 then b:=a;root:=a;
 Receiver()[Key()]+=Rhs();
 Writeln('|',a,'|',b,'|',root=b);
end.
