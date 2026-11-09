var a,b,root:Dictionary<string,array of integer>;
 receiverMode,keyMode,aliasMode,rhsMode,seed,receiverCalls,keyCalls:integer;
function Receiver:Dictionary<string,array of integer>;
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
function Rhs:integer;
begin
 Write('R;');
 if rhsMode=1 then root:=b else if rhsMode=2 then a.Clear else if rhsMode=3 then begin a['a'][0]:=99; end;
 Result:=2;
end;
begin
 Read(receiverMode,keyMode,aliasMode,rhsMode,seed);
 a:=new Dictionary<string,array of integer>;a.Add('a',Arr(1,2));a.Add('b',Arr(3,4));
 b:=new Dictionary<string,array of integer>;b.Add('a',Arr(5,6));b.Add('b',Arr(7,8));
 if seed=1 then a['a']:=nil;
 if aliasMode<>0 then b:=a;root:=a;
 Receiver()[Key()]*=Rhs();
 Writeln('|',a,'|',b,'|',root=b);
end.
