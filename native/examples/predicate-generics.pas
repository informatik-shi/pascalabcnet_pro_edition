function Match2(n: integer; text: string): boolean := n = text.Length;
function Match3(n: integer; text: string; expected: boolean): boolean := Match2(n,text) = expected;
begin
  Writeln(typeof(Predicate2<integer,string>).Name, ' ',
    typeof(Predicate2<integer,string>) = typeof(Func2<integer,string,boolean>));
  Writeln(typeof(Predicate3<integer,string,boolean>).Name, ' ',
    typeof(Predicate3<integer,string,boolean>) = typeof(Func3<integer,string,boolean,boolean>));
  var p: Predicate2<integer,string> := Match2;
  var q: Predicate3<integer,string,boolean> := Match3;
  var f: Func2<integer,string,boolean> := p;
  var g: Func3<integer,string,boolean,boolean> := q;
  Writeln(p.GetType.Namespace);
  Writeln(f(3,'abc'), ' ', g(3,'abc',true));
  var back: Predicate2<integer,string> := f;
  Writeln(back = p);
  var types := new HashSet<System.Type>(Arr(p.GetType,q.GetType,f.GetType,g.GetType));
  Writeln(types.Count);
end.
