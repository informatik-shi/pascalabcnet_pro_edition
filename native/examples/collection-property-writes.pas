var dictionary:Dictionary<string,integer>;
    callbacks:List<integer->integer>;
    keyCalls,indexCalls:integer;
function NextKey:string;
begin
  Inc(keyCalls);
  Result:=if keyCalls=1 then 'left' else 'right';
end;
function NextIndex:integer;
begin
  Result:=indexCalls;
  Inc(indexCalls);
end;
function WithCallback(f:integer->integer):Dictionary<string,integer>;
begin
  callbacks.Add(f);
  Result:=dictionary;
end;
begin
  callbacks:=new List<integer->integer>;
  dictionary:=new Dictionary<string,integer>;
  dictionary.Add('left',10);
  dictionary.Add('right',20);
  dictionary[NextKey()]+=5;
  Writeln(dictionary['left'],' ',dictionary['right'],' ',keyCalls);

  var list:=Lst(1,2);
  list[NextIndex()]+=9;
  Writeln(list,' ',indexCalls);
  WithCallback(x->x+1)['left']+=1;
  Writeln(callbacks.Count,' ',callbacks[0]=callbacks[1],' ',dictionary['left']);
end.
