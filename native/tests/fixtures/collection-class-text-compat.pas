program CollectionClassText;
uses Collections;
type
  MixedCaseItem = class
  public
    value: integer;
  end;
  NamedMethod = class
  public
    function ToString: string := 'named method';
  end;
  ExampleRecord = record
    value: integer;
  end;
  ExampleEnum = (ChoiceA,ChoiceB);
begin
  var first:=new MixedCaseItem;
  first.value:=93;
  var a:=new Stack<MixedCaseItem>;
  Writeln('[',a.ToString,']');
  a.Push(first);
  Writeln('[',a.ToString,'] [',first.ToString,']');
  var b:=new Queue<MixedCaseItem>;
  b.Enqueue(first);
  b.Enqueue(nil);
  Writeln('[',b.ToString,']');
  var hidden:=new NamedMethod;
  var s:=new Stack<NamedMethod>;
  s.Push(hidden);
  var q:=new Queue<NamedMethod>;
  q.Enqueue(hidden);
  Writeln('[',hidden.ToString,'] [',s.ToString,'] [',q.ToString,']');
  Writeln(typeof(MixedCaseItem).AssemblyQualifiedName);
  Writeln(typeof(ExampleRecord).FullName,' ',typeof(ExampleEnum).Namespace);
end.
