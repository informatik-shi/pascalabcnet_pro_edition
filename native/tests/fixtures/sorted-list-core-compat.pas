uses System;
begin
  var q := new SortedList<integer,string>;
  Println(q.Count,q.Capacity);
  foreach var x in Arr(4,3,2,1,0,-1) do
  begin
    q.Add(x, IntToStr(x));
    Println(q.Count,q.Capacity);
  end;
  var api: IDictionary<integer,string> := q;
  Println(api.Keys.JoinIntoString(','),api.Values.JoinIntoString(','));
  Println(q.IndexOfKey(3),q.IndexOfKey(11),q.IndexOfValue('3'),q.IndexOfValue('absent'));
  Println(q[3],3 in q,11 in q);
  q.RemoveAt(1);
  Println(api.Keys.JoinIntoString(','),q.Capacity);
  q.TrimExcess;
  Println(q.Capacity);
  var e := q.GetEnumerator;
  var f := e;
  Println(e.GetType.Name,e.MoveNext,e.Current.Key,f.Current.Key);
  Println(f.MoveNext,e.Current.Key,f.Current.Key);
  q.Capacity := 12;
  Println(e.MoveNext,e.Current.Key,q.Capacity);
  e.Dispose;
  Println(f.Current.Key,f.MoveNext,f.Current.Key);
  q.Clear;
  Println(q.Count,q.Capacity);
  q.TrimExcess;
  Println(q.Count,q.Capacity);
  var cmp := Comparer&<integer>.Create((a,b)-> b.CompareTo(a));
  var h: IDictionary<integer,string> := new Dictionary<integer,string>;
  h.Add(2,'two');
  h.Add(5,'five');
  h.Add(1,'one');
  var k := new SortedList<integer,string>(h,cmp);
  Println(k.Count,k.Capacity,k.Comparer.Compare(1,2));
  foreach var pair in k do
    Println(pair.Key,pair.Value);
  var l := new System.Collections.Generic.SortedList<integer,string>(17,cmp);
  Println(l.Count,l.Capacity);
  var m := new SortedList<integer,string>(cmp);
  Println(m.Count,m.Capacity);
  var n := new SortedList<integer,string>(7);
  Println(n.Count,n.Capacity);
  n.Capacity += 1;
  Println(n.Capacity);
  Println(k.GetType.Name,k.GetType.Namespace);
end.
