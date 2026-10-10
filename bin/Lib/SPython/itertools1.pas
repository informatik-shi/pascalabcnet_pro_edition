unit itertools1;

interface

uses System, System.Collections, System.Collections.Generic, SPythonSystem;

type
  // Python tuples keep their length and can be indexed and unpacked.
  PyTuple<T> = class(System.Collections.Generic.IReadOnlyCollection<T>)
  private
    values: array of T;
    function GetItem(index: integer): T;
  public
    constructor Create(items: array of T);
    property Length: integer read values.Length;
    property Count: integer read values.Length;
    property ByIndex[index: integer]: T read GetItem; default;
    function GetEnumerator(): IEnumerator<T>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
    function ToString(): string; override;
  end;

function product(): sequence of PyTuple<object>;
function product<T>(params iterables: array of sequence of T): sequence of PyTuple<T>;
function product(params iterables: array of string): sequence of PyTuple<string>;
function product(params iterables: array of System.Collections.IEnumerable): sequence of PyTuple<PyValue>;
function product_repeat(&repeat: integer): sequence of PyTuple<object>;
function product_repeat<T>(&repeat: integer; params iterables: array of sequence of T): sequence of PyTuple<T>;
function product_repeat(&repeat: integer; params iterables: array of string): sequence of PyTuple<string>;
function product_repeat(&repeat: integer; params iterables: array of System.Collections.IEnumerable): sequence of PyTuple<PyValue>;

function permutations<T>(iterable: sequence of T): sequence of PyTuple<T>;
function permutations<T>(iterable: sequence of T; r: integer): sequence of PyTuple<T>;
function permutations(iterable: string): sequence of PyTuple<string>;
function permutations(iterable: string; r: integer): sequence of PyTuple<string>;

function combinations<T>(iterable: sequence of T; r: integer): sequence of PyTuple<T>;
function combinations(iterable: string; r: integer): sequence of PyTuple<string>;

function chain(): sequence of object;
function chain<T>(params iterables: array of sequence of T): sequence of T;
function chain(params iterables: array of string): sequence of string;
function &repeat<T>(value: T): sequence of T;
function &repeat<T>(value: T; times: integer): sequence of T;

implementation

constructor PyTuple<T>.Create(items: array of T);
begin
  values := new T[items.Length];
  System.Array.Copy(items, values, items.Length);
end;

function PyTuple<T>.GetItem(index: integer): T;
begin
  if index < 0 then index += values.Length;
  if (index < 0) or (index >= values.Length) then
    raise new System.IndexOutOfRangeException('tuple index out of range');
  Result := values[index];
end;

function PyTuple<T>.GetEnumerator(): IEnumerator<T> :=
  (values as IEnumerable<T>).GetEnumerator();

function PythonRepr(value: object): string;
begin
  if value is PyValue then value := (value as PyValue).value;
  if value = nil then exit('None');
  if value is string then
  begin
    var original := string(value);
    var s := original.Replace('\', '\\').Replace(#10, '\n').Replace(#13, '\r').Replace(#9, '\t');
    if original.Contains('''') and not original.Contains('"') then exit('"' + s + '"');
    exit('''' + s.Replace('''', '\''') + '''');
  end;
  if value is boolean then exit(if boolean(value) then 'True' else 'False');
  if value is real then
  begin
    var number := real(value);
    if System.Double.IsNaN(number) then exit('nan');
    if System.Double.IsPositiveInfinity(number) then exit('inf');
    if System.Double.IsNegativeInfinity(number) then exit('-inf');
    var s := number.ToString('R', System.Globalization.CultureInfo.InvariantCulture);
    if not s.Contains('.') and not s.Contains('E') and not s.Contains('e') then s += '.0';
    exit(s);
  end;
  Result := value.ToString();
end;

function PyTuple<T>.ToString(): string;
begin
  var parts := new List<string>;
  foreach var value in values do parts.Add(PythonRepr(value));
  Result := '(' + System.String.Join(', ', parts.ToArray());
  if values.Length = 1 then Result += ',';
  Result += ')';
end;

// Every traversal consumes the same underlying cursor, even after an early break.
function ConsumeCursor<T>(cursor: IEnumerator<T>): sequence of T;
begin
  while cursor.MoveNext() do yield cursor.Current;
end;

function OneShot<T>(source: sequence of T): sequence of T :=
  ConsumeCursor(source.GetEnumerator());

function StringItems(value: string): sequence of string;
begin
  foreach var c in value do yield c.ToString();
end;

function ObjectItems(value: System.Collections.IEnumerable): sequence of PyValue;
begin
  var cursor := value.GetEnumerator();
  while cursor.MoveNext() do
  begin
    var item := cursor.Current;
    yield new PyValue(if item is char then char(item).ToString() else item);
  end;
end;

function MakePool<T>(source: sequence of T): array of T;
begin
  Result := source.ToArray();
end;

function ProductSequence<T>(pools: array of array of T): sequence of PyTuple<T>;
begin
  var count := pools.Length;
  var indexes := new integer[count];
  while true do
  begin
    var items := new T[count];
    for var i := 0 to count - 1 do
    begin
      if pools[i].Length = 0 then exit;
      items[i] := pools[i][indexes[i]];
    end;
    yield new PyTuple<T>(items);
    if count = 0 then exit;
    var position := count - 1;
    while position >= 0 do
    begin
      indexes[position] += 1;
      if indexes[position] < pools[position].Length then break;
      indexes[position] := 0;
      position -= 1;
    end;
    if position < 0 then exit;
  end;
end;

function MakeProduct<T>(sources: array of sequence of T; &repeat: integer): sequence of PyTuple<T>;
begin
  if &repeat < 0 then raise new System.ArgumentException('repeat argument cannot be negative');
  var pools: array of array of T;
  SetLength(pools, sources.Length);
  for var i := 0 to sources.Length - 1 do pools[i] := MakePool(sources[i]);
  var repeated: array of array of T;
  SetLength(repeated, pools.Length * &repeat);
  for var i := 1 to &repeat do
    for var j := 0 to pools.Length - 1 do
      repeated[(i - 1) * pools.Length + j] := pools[j];
  Result := OneShot(ProductSequence(repeated));
end;

function product(): sequence of PyTuple<object>;
begin
  var sources := new List<sequence of object>;
  Result := MakeProduct&<object>(sources.ToArray(), 1);
end;
function product<T>(params iterables: array of sequence of T): sequence of PyTuple<T> :=
  MakeProduct&<T>(iterables, 1);
function product(params iterables: array of string): sequence of PyTuple<string>;
begin
  var sources := new List<sequence of string>;
  foreach var iterable in iterables do sources.Add(StringItems(iterable));
  Result := MakeProduct&<string>(sources.ToArray(), 1);
end;
function product(params iterables: array of System.Collections.IEnumerable): sequence of PyTuple<PyValue>;
begin
  var sources := new List<sequence of PyValue>;
  foreach var iterable in iterables do sources.Add(ObjectItems(iterable));
  Result := MakeProduct&<PyValue>(sources.ToArray(), 1);
end;
function product_repeat(&repeat: integer): sequence of PyTuple<object>;
begin
  var sources := new List<sequence of object>;
  Result := MakeProduct&<object>(sources.ToArray(), &repeat);
end;
function product_repeat<T>(&repeat: integer; params iterables: array of sequence of T): sequence of PyTuple<T> :=
  MakeProduct&<T>(iterables, &repeat);
function product_repeat(&repeat: integer; params iterables: array of string): sequence of PyTuple<string>;
begin
  var sources := new List<sequence of string>;
  foreach var iterable in iterables do sources.Add(StringItems(iterable));
  Result := MakeProduct&<string>(sources.ToArray(), &repeat);
end;
function product_repeat(&repeat: integer; params iterables: array of System.Collections.IEnumerable): sequence of PyTuple<PyValue>;
begin
  var sources := new List<sequence of PyValue>;
  foreach var iterable in iterables do sources.Add(ObjectItems(iterable));
  Result := MakeProduct&<PyValue>(sources.ToArray(), &repeat);
end;

function PermutationSequence<T>(pool: array of T; r: integer): sequence of PyTuple<T>;
begin
  var n := pool.Length;
  if r > n then exit;
  var indexes := new integer[n];
  for var i := 0 to n - 1 do indexes[i] := i;
  var cycles := new integer[r];
  for var i := 0 to r - 1 do cycles[i] := n - i;
  while true do
  begin
    var items := new T[r];
    for var i := 0 to r - 1 do items[i] := pool[indexes[i]];
    yield new PyTuple<T>(items);
    var advanced := false;
    for var i := r - 1 downto 0 do
    begin
      cycles[i] -= 1;
      if cycles[i] = 0 then
      begin
        var old := indexes[i];
        for var j := i to n - 2 do indexes[j] := indexes[j + 1];
        indexes[n - 1] := old;
        cycles[i] := n - i;
      end
      else
      begin
        var j := n - cycles[i];
        var old := indexes[i];
        indexes[i] := indexes[j];
        indexes[j] := old;
        advanced := true;
        break;
      end;
    end;
    if not advanced then exit;
  end;
end;

function permutations<T>(iterable: sequence of T): sequence of PyTuple<T>;
begin
  var pool := MakePool(iterable);
  Result := OneShot(PermutationSequence(pool, pool.Length));
end;

function permutations<T>(iterable: sequence of T; r: integer): sequence of PyTuple<T>;
begin
  if r < 0 then raise new System.ArgumentException('r must be non-negative');
  Result := OneShot(PermutationSequence(MakePool(iterable), r));
end;

function permutations(iterable: string): sequence of PyTuple<string> :=
  permutations(StringItems(iterable));
function permutations(iterable: string; r: integer): sequence of PyTuple<string> :=
  permutations(StringItems(iterable), r);

function CombinationSequence<T>(pool: array of T; r: integer): sequence of PyTuple<T>;
begin
  var n := pool.Length;
  if r > n then exit;
  var indexes := new integer[r];
  for var i := 0 to r - 1 do indexes[i] := i;
  while true do
  begin
    var items := new T[r];
    for var i := 0 to r - 1 do items[i] := pool[indexes[i]];
    yield new PyTuple<T>(items);
    var position := r - 1;
    while (position >= 0) and (indexes[position] = position + n - r) do position -= 1;
    if position < 0 then exit;
    indexes[position] += 1;
    for var j := position + 1 to r - 1 do indexes[j] := indexes[j - 1] + 1;
  end;
end;

function combinations<T>(iterable: sequence of T; r: integer): sequence of PyTuple<T>;
begin
  if r < 0 then raise new System.ArgumentException('r must be non-negative');
  Result := OneShot(CombinationSequence(MakePool(iterable), r));
end;

function combinations(iterable: string; r: integer): sequence of PyTuple<string> :=
  combinations(StringItems(iterable), r);

function ChainSequence<T>(iterables: array of sequence of T): sequence of T;
begin
  foreach var iterable in iterables do
    foreach var value in iterable do yield value;
end;

function chain(): sequence of object;
begin
  var sources := new List<sequence of object>;
  Result := OneShot(ChainSequence(sources.ToArray()));
end;

function chain<T>(params iterables: array of sequence of T): sequence of T :=
  OneShot(ChainSequence(iterables));

function chain(params iterables: array of string): sequence of string;
begin
  var sources := new List<sequence of string>;
  foreach var iterable in iterables do sources.Add(StringItems(iterable));
  Result := OneShot(ChainSequence(sources.ToArray()));
end;

function RepeatSequence<T>(value: T; times: integer): sequence of T;
begin
  for var i := 1 to times do yield value;
end;

function RepeatForever<T>(value: T): sequence of T;
begin
  while true do yield value;
end;

function &repeat<T>(value: T): sequence of T :=
  OneShot(RepeatForever(value));
function &repeat<T>(value: T; times: integer): sequence of T :=
  OneShot(RepeatSequence(value, times));

end.
