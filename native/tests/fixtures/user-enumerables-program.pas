type
  Cursor = class(IEnumerator<integer>)
  private
    tag: string;
    limit, flags, n: integer;
    function GetCurrent: integer;
    begin
      Write('C',tag,'(',n,');');
      if ((flags and 4)<>0) and (n=1) then begin var zero:=0;var failure:=1 div zero;end;
      Result:=n;
    end;
  public
    constructor Create(id:string;count,mode:integer);
    begin tag:=id;limit:=count;flags:=mode;end;
    function MoveNext: boolean;
    begin
      Write('M',tag,'(',n,');');
      if ((flags and 2)<>0) and (n=1) then begin var zero:=0;var failure:=1 div zero;end;
      n+=1;Result:=n<=limit;
    end;
    property Current:integer read GetCurrent;
    procedure Dispose;
    begin
      Write('D',tag,';');
      if (flags and 8)<>0 then begin var failure:=Arr(1)[2];end;
    end;
    procedure Reset;
    begin n:=0;end;
  end;
  Source = class(IEnumerable<integer>)
  private
    tag:string;
    limit,flags:integer;
  public
    constructor Create(id:string;count,mode:integer);
    begin tag:=id;limit:=count;flags:=mode;end;
    function GetEnumerator:IEnumerator<integer>;
    begin
      Write('G',tag,';');
      if (flags and 1)<>0 then begin var zero:=0;var failure:=1 div zero;end;
      Result:=new Cursor(tag,limit,flags);
    end;
  end;
var scenario,count,flags:integer;
procedure Body(n:integer;tag:string);
begin
  Write(tag,n,';');
  if (flags and 16)<>0 then begin var zero:=0;var failure:=1 div zero;end;
end;
function FirstReturn(source:IEnumerable<integer>):integer;
begin
  foreach var x in source do begin Body(x,'B');Exit(x);end;
  Result:=0;
end;
function Inner(x:integer):IEnumerable<integer>;
begin Body(x,'S');Result:=new Source('b',1,flags);end;
function FirstReturnBoth(first,second:IEnumerable<integer>):integer;
begin
  foreach var x in first do foreach var y in second do begin Body(x+y,'N');Exit(x+y);end;
  Result:=0;
end;
begin
  Read(scenario,count,flags);
  var source:IEnumerable<integer>:=new Source('a',count,flags);
  var other:IEnumerable<integer>:=new Source('b',count+1,flags);
  var equalSource:IEnumerable<integer>:=new Source('b',count,flags);
  var innerSource:IEnumerable<integer>:=new Source('b',1,flags);
  if scenario=0 then foreach var x in source do Body(x,'B')
  else if scenario=1 then foreach var x in source do begin Body(x,'B');break;end
  else if scenario=2 then foreach var x in source do begin Body(x,'B');if x=1 then continue;begin var answer:=x;Write('=',answer,';');end;end
  else if scenario=3 then begin var answer:=FirstReturn(source);Write('=',answer,';');end
  else if scenario=4 then begin var answer:=source.Count;Write('=',answer,';');end
  else if scenario=5 then begin var answer:=source.ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=6 then begin var answer:=source.Any;Write('=',answer,';');end
  else if scenario=7 then begin var answer:=source.First;Write('=',answer,';');end
  else if scenario=8 then begin var answer:=source.Last;Write('=',answer,';');end
  else if scenario=9 then begin var answer:=source.Single;Write('=',answer,';');end
  else if scenario=10 then begin var answer:=source.Sum;Write('=',answer,';');end
  else if scenario=11 then begin var answer:=source.Average;Write('=',answer,';');end
  else if scenario=12 then begin var answer:=source.Take(1).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=13 then begin var answer:=source.Skip(1).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=14 then begin var answer:=source.Reverse.ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=15 then begin var answer:=source.Select(x->begin Body(x,'S');Result:=x*2;end).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=16 then begin var answer:=source.Where(x->begin Body(x,'W');Result:=x mod 2=1;end).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=17 then begin var answer:=source.SelectMany(Inner).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=18 then begin var answer:=source.Zip(other,(x,y)->begin Body(x,'Z');Result:=x+y;end).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=19 then begin var answer:=source.SequenceEqual(equalSource);Write('=',answer,';');end
  else if scenario=20 then begin var e:=source.GetEnumerator;begin var answer:=e.Current;Write('=',answer,';');end;begin var answer:=e.MoveNext;Write('=',answer,';');end;e.Dispose;begin var answer:=e.MoveNext;Write('=',answer,';');end;begin var answer:=e.Current;Write('=',answer,';');end;e.Dispose;end
  else if scenario=21 then begin var answer:=source.FirstOrDefault;Write('=',answer,';');end
  else if scenario=22 then begin var answer:=source.LastOrDefault;Write('=',answer,';');end
  else if scenario=23 then begin var answer:=source.ElementAtOrDefault(1);Write('=',answer,';');end
  else if scenario=24 then begin var answer:=source.Take(0).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=25 then source.ForEach(x->Body(x,'A'))
  else if scenario=26 then begin var answer:=source.Select(x->x).Zip(other,(x,y)->begin Body(x,'Z');Result:=x+y;end).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=27 then begin var answer:=source.Concat(innerSource).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=28 then begin var answer:=source.TakeWhile(x->begin Body(x,'T');Result:=x<2;end).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=29 then begin var answer:=source.SkipWhile(x->begin Body(x,'T');Result:=x<2;end).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=30 then begin var answer:=source.Where(x->x mod 2=1).FirstOrDefault;Write('=',answer,';');end
  else if scenario=31 then begin var answer:=source.Take(1).Zip(other,(x,y)->begin Body(x,'Z');Result:=x+y;end).ToArray.JoinToString(',');Write('=',answer,';');end
  else if scenario=32 then begin var answer:=source.Select(x->x).SequenceEqual(equalSource);Write('=',answer,';');end
  else if scenario=33 then begin var answer:=source.Take(1).SequenceEqual(equalSource);Write('=',answer,';');end
  else if scenario=34 then foreach var x in source do foreach var y in innerSource do Body(x+y,'N')
  else if scenario=35 then begin var answer:=FirstReturnBoth(source,innerSource);Write('=',answer,';');end
  else if scenario=36 then begin var answer:=source.Any(x->begin Body(x,'P');Result:=x>=2;end);Write('=',answer,';');end
  else if scenario=37 then begin var answer:=source.All(x->begin Body(x,'P');Result:=x<2;end);Write('=',answer,';');end
  else if scenario=38 then begin var answer:=source.Count(x->begin Body(x,'P');Result:=x>=2;end);Write('=',answer,';');end
  else if scenario=39 then begin var answer:=source.ToDictionary(x->begin Body(x,'K');Result:=x;end).Count;Write('=',answer,';');end
  else if scenario=40 then begin var answer:=HSet(source).Count;Write('=',answer,';');end
  else if scenario=41 then begin var answer:=(new List<integer>(source)).Count;Write('=',answer,';');end
  else if scenario=42 then begin var answer:=source.ToArray.Length;Write('=',answer,';');end;
end.
