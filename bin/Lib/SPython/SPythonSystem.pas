{$HiddenIdents}
unit SPythonSystem;

// В SPython данной версии нет возможности использовать пространства имен
// {$reference '%GAC%\System.dll'}
// {$reference '%GAC%\mscorlib.dll'}
// {$reference '%GAC%\System.Core.dll'}
// {$reference '%GAC%\System.Numerics.dll'}

interface

// uses PABCSystem;

// Basic IO methods

function input(): string;

function input(s: string): string;

// Python-style read-only streams. Known literal modes keep concrete static
// types; a mode computed at run time uses PythonFile and PythonReadData.
type
  bytes = class(IEnumerable<integer>)
  private
    data: array of byte;
    function GetItem(index: integer): integer;
  public
    constructor Create(value: array of byte);
    property ByIndex[index: integer]: integer read GetItem; default;
    property Length: integer read data.Length;
    function to_array(): array of byte;
    function decode(encoding: string := 'utf-8'; errors: string := 'strict'): string;
    function hex(): string;
    function ToString(): string; override;
    function GetEnumerator(): IEnumerator<integer>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
  end;

  PythonBinaryFile = class(System.IDisposable, IEnumerable<bytes>)
  private
    stream: System.IO.FileStream;
    function GetClosed(): boolean;
    function Lines(): sequence of bytes;
  public
    constructor Create(path: string; mode: string; buffering: integer);
    function read(size: integer := -1): bytes;
    function readline(size: integer := -1): bytes;
    function readlines(hint: integer := -1): array of bytes;
    function seek(offset: integer; whence: integer := 0): integer;
    function tell(): integer;
    procedure close();
    procedure Dispose();
    function readable(): boolean;
    function seekable(): boolean;
    property closed: boolean read GetClosed;
    function GetEnumerator(): IEnumerator<bytes>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
  end;

  PythonTextFile = class(System.IDisposable, IEnumerable<string>)
  private
    stream: System.IO.FileStream;
    reader: System.IO.StreamReader;
    textEncoding: System.Text.Encoding;
    bytePosition: integer;
    newlineMode: string;
    function ReadCharacter(): integer;
    function GetClosed(): boolean;
    function Lines(): sequence of string;
  public
    constructor Create(path: string; mode: string; buffering: integer;
      encoding: string; errors: string; newline: string);
    function read(size: integer := -1): string;
    function readline(size: integer := -1): string;
    function readlines(hint: integer := -1): array of string;
    function seek(offset: integer; whence: integer := 0): integer;
    function tell(): integer;
    procedure close();
    procedure Dispose();
    function readable(): boolean;
    function seekable(): boolean;
    property closed: boolean read GetClosed;
    function GetEnumerator(): IEnumerator<string>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
  end;

  PythonReadData = class(IEnumerable<PythonReadData>)
  private
    rawValue: object;
    function GetLength(): integer;
    function GetItem(index: integer): PythonReadData;
    function AsInteger(): integer;
  public
    constructor Create(value: object);
    property value: object read rawValue;
    property Length: integer read GetLength;
    property ByIndex[index: integer]: PythonReadData read GetItem; default;
    function as_bytes(): bytes;
    function decode(encoding: string := 'utf-8'; errors: string := 'strict'): string;
    function hex(): string;
    function ToString(): string; override;
    function GetEnumerator(): IEnumerator<PythonReadData>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
    static function operator implicit(value: integer): PythonReadData := new PythonReadData(value);
    static function operator implicit(value: string): PythonReadData := new PythonReadData(value);
    static function operator implicit(value: PythonReadData): bytes := value.as_bytes();
    static function operator +(a, b: PythonReadData): PythonReadData;
    static function operator -(a, b: PythonReadData): PythonReadData;
    static function operator *(a, b: PythonReadData): PythonReadData;
    static function operator div(a, b: PythonReadData): PythonReadData;
    static function operator mod(a, b: PythonReadData): PythonReadData;
    static function operator =(a: PythonReadData; b: integer): boolean;
    static function operator =(a: PythonReadData; b: string): boolean;
    static function operator =(a, b: PythonReadData): boolean;
  end;

  PythonFile = class(System.IDisposable, IEnumerable<PythonReadData>)
  private
    binaryFile: PythonBinaryFile;
    textFile: PythonTextFile;
    function GetClosed(): boolean;
    function Lines(): sequence of PythonReadData;
  public
    constructor Create(path: string; mode: string; buffering: integer;
      encoding: string; errors: string; newline: string);
    function read(size: integer := -1): PythonReadData;
    function readline(size: integer := -1): PythonReadData;
    function readlines(hint: integer := -1): System.Collections.Generic.List<PythonReadData>;
    function seek(offset: integer; whence: integer := 0): integer;
    function tell(): integer;
    procedure close();
    procedure Dispose();
    function readable(): boolean;
    function seekable(): boolean;
    property closed: boolean read GetClosed;
    function GetEnumerator(): IEnumerator<PythonReadData>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
  end;

function open(path: string; mode: string := 'r'; buffering: integer := -1;
  encoding: string := nil; errors: string := nil; newline: string := nil): PythonFile;
function !open_text(path: string; mode: string := 'r'; buffering: integer := -1;
  encoding: string := nil; errors: string := nil; newline: string := nil): PythonTextFile;
function !open_binary(path: string; mode: string := 'rb'; buffering: integer := -1;
  encoding: string := nil; errors: string := nil; newline: string := nil): PythonBinaryFile;

///--
type kwargs_gen<T> = class
      public !kwargs: Dictionary<string, T>
        := new Dictionary<string, T>();
      
      constructor Create(
        keys: array of string;
        params values: array of T
        );
      begin
        for var i := 0 to keys.count() - 1 do
          !kwargs[keys[i]] := values[i];
      end;
      
      constructor Create(
        keys: array of char;
        params values: array of T
        );
      begin
        for var i := 0 to keys.count() - 1 do
          !kwargs[keys[i]] := values[i];
      end;
      
      constructor Create(); begin end;
    end;

function all(s: sequence of boolean): boolean;
function all(s: sequence of integer): boolean;
function any(s: sequence of boolean): boolean;
function any(s: sequence of integer): boolean;

function enumerate<T>(s: sequence of T; start: integer := 0): sequence of (integer,T);
function filter<T>(cond: T -> boolean; s: sequence of T): sequence of T;
function sorted<T>(s: sequence of T; reverse: boolean := false): sequence of T;
function sorted<T,T1>(s: sequence of T; key: T -> T1; reverse: boolean := false): sequence of T;

function int(val: string): integer;

function int(val: real): integer;

function int(obj: object): integer;

function int(b: boolean): integer;

function str(val: object): string;

function float(val: string): real;

function float(x: integer): real;

function float(x: object): real;

function bool(val: integer): boolean;

function round(val: real): integer;

function split(s: string): sequence of string;

function get_keys<K, V>(dct: Dictionary<K, V>): sequence of K;
function get_values<K, V>(dct: Dictionary<K, V>): sequence of V;

function &type(obj: object): string;

// Basic sequence functions

function range(s: integer; e: integer; step: integer): sequence of integer;

function range(e: integer): sequence of integer;

function range(s: integer; e: integer): sequence of integer;

function !format(obj: object; fmt: string): string;

function !format(i: integer; fmt: string): string;

function !format(val: real; fmt: string): string;

//------------------------------------
//     Standard Math functions
//------------------------------------

/// Возвращает абсолютное значение числа
function abs(x: integer): integer;
/// Возвращает абсолютное значение числа
function abs(x: real): real;

/// Возвращает x в степени y
function pow(x,y: real): real;
/// Возвращает x в целой степени n
function pow(x: real; n: integer): real;
/// Возвращает x в целой степени n
function pow(x: BigInteger; n: integer): BigInteger;

{$region STANDARD CONTAINERS}

type list<T> = class(IEnumerable<T>)
    private
      wrappee : PABCSystem.List<T>;
    
      constructor Create(l : PABCSystem.List<T>) := wrappee := l;
    
      function GetItem(ind : integer) : T := wrappee[ind];
      
      procedure SetItem(ind : integer; newItem : T) := wrappee[ind] := newItem;
    
    public
    
      static function operator implicit(l : PABCSystem.List<T>) : list<T> := new list<T>(l);
      
      constructor Create() := wrappee := new PABCSystem.List<T>();
      
      constructor Create(capacity : integer) := wrappee := new PABCSystem.List<T>(capacity);
    
      constructor Create(collection : sequence of T) := wrappee := new PABCSystem.List<T>(collection);
    
      property ByIndex[ind : integer] : T read GetItem write SetItem; default;
    
      ///--
      property !count : integer read wrappee.Count;
    
      procedure append(item : T) := wrappee.Add(item);
      
      procedure extend(seq : sequence of T) := wrappee.AddRange(seq);
      
      procedure clear() := wrappee.Clear();
      
      procedure insert(index : integer; item : T) := wrappee.Insert(index, item);
      
      procedure remove(item : T);
      begin
         if not wrappee.Remove(item) then
           raise new System.ArgumentException($'ValueError: {item} is not in list');
      end;
      
      function pop() : T;
      begin
        var lastIndex := wrappee.Count - 1;
        Result := wrappee[lastIndex];
        wrappee.RemoveAt(lastIndex);
      end;
      
      function pop(index : integer) : T;
      begin
        Result := wrappee[index];
        wrappee.RemoveAt(index);
      end;
    
      function index(item : T) : integer := index(item, 0, wrappee.Count);
      
      function index(item : T; start : integer) : integer := index(item, start, wrappee.Count);
      
      function index(item : T; start : integer; &end : integer) : integer;
      begin
        Result := wrappee.IndexOf(item, start, &end - start);
        if Result = -1 then
          raise new System.ArgumentException($'ValueError: {item} is not in list');
      end;
      
      function count(item : T) : integer;
      begin
        var comparer := System.Collections.Generic.EqualityComparer&<T>.Default;
        
        Result := 0;
        for var i := 0 to wrappee.Count - 1 do
        begin
          if comparer.Equals(wrappee[i], item) then
            Result += 1;
        end;
      end;
    
      procedure sort() := wrappee.Sort();
    
      procedure sort(reverse : boolean) := wrappee.OrderByDescending(x -> x);
      
      procedure sort<TKey>(key : T -> TKey) := wrappee.OrderBy(key);
      
      procedure sort<TKey>(key : T -> TKey; reverse : boolean) := wrappee.OrderByDescending(key);
      
      procedure reverse() := wrappee.Reverse();
    
      function copy() : list<T> := new list<T>(wrappee.ToList());
    
      function GetEnumerator() : IEnumerator<T> := wrappee.GetEnumerator();

      function System.Collections.IEnumerable.GetEnumerator() : System.Collections.IEnumerator := GetEnumerator();
    end;

type &set<T> = record(IEnumerable<T>)
  private
    wrappee : PABCSystem.HashSet<T>;
    
    constructor Create(h : PABCSystem.HashSet<T>) := wrappee := h;
    
  public
    
    constructor Create() := wrappee := new PABCSystem.HashSet<T>();
    
    constructor Create(collection : sequence of T) := wrappee := new PABCSystem.HashSet<T>(collection);
    
    constructor Create(collection : sequence of T; comparer : PABCSystem.IEqualityComparer<T>) := wrappee := new PABCSystem.HashSet<T>(collection, comparer);
    
    constructor Create(comparer : PABCSystem.IEqualityComparer<T>) := wrappee := new PABCSystem.HashSet<T>(comparer);
    
    ///--
    property !count : integer read wrappee.Count;
    
    procedure add(elem : T) := wrappee.Add(elem);
    
    procedure remove(elem : T);
    begin
      if not wrappee.Remove(elem) then
        raise new System.Collections.Generic.KeyNotFoundException('KeyError: ' + elem.ToString());
    end;
    
    procedure discard(elem : T) := wrappee.Remove(elem);
    
    function pop() : T;
    begin 
      if wrappee.Count = 0 then
        raise new System.Collections.Generic.KeyNotFoundException('KeyError: ''pop from an empty set''');
      
      Result := wrappee.First();
      wrappee.Remove(Result);
    end;
    
    procedure clear() := wrappee.Clear();
    
    function copy() : &set<T> := new &set<T>(wrappee.ToHashSet());
    
    static function operator in(elem: T; Self: &set<T>): boolean;
    begin
      Result := Self.wrappee.Contains(elem);
    end;
    
  private 
    function setOperation1(op : (HashSet<T>, sequence of T) -> (); params others : array of sequence of T) : &set<T>;
    begin
      Result.wrappee := wrappee.ToHashSet();
      foreach var seq in others do
        op(Result.wrappee, seq);
    end;
    
    procedure setOperation2(op : (HashSet<T>, sequence of T) -> (); params others : array of sequence of T);
    begin
      foreach var seq in others do
        op(wrappee, seq);
    end;
    
  public
    function intersection(params others : array of sequence of T) : &set<T> := setOperation1((h1, h2) -> h1.IntersectWith(h2), others);
    
    function union(params others : array of sequence of T) : &set<T> := setOperation1((h1, h2) -> h1.UnionWith(h2), others);
    
    function difference(params others : array of sequence of T) : &set<T> := setOperation1((h1, h2) -> h1.ExceptWith(h2), others);
    
    function symmetric_difference(params others : array of sequence of T) : &set<T> := setOperation1((h1, h2) -> h1.SymmetricExceptWith(h2), others);
    
    procedure update(params others : array of sequence of T) := setOperation2((h1, h2) -> h1.UnionWith(h2), others);
    
    procedure update(other : sequence of T) := wrappee.UnionWith(other);
    
    procedure intersection_update(params others : array of sequence of T) := setOperation2((h1, h2) -> h1.IntersectWith(h2), others);
    
    procedure intersection_update(other : sequence of T) := wrappee.IntersectWith(other);
    
    procedure difference_update(params others : array of sequence of T) := setOperation2((h1, h2) -> h1.ExceptWith(h2), others);
    
    procedure difference_update(other : sequence of T) := wrappee.ExceptWith(other);
    
    procedure symmetric_difference_update(params others : array of sequence of T) := setOperation2((h1, h2) -> h1.SymmetricExceptWith(h2), others);
    
    procedure symmetric_difference_update(other : sequence of T) := wrappee.SymmetricExceptWith(other);
    
    function isdisjoint(other : sequence of T) : boolean;
    
    function issubset(other : sequence of T) : boolean := wrappee.IsSubsetOf(other);
    
    function issuperset(other : sequence of T) : boolean := wrappee.IsSupersetOf(other);
    
    static function operator=(s1 : &set<T>; s2 : &set<T>) : boolean := s1.wrappee.SetEquals(s2.wrappee);
    
    static function operator<>(s1 : &set<T>; s2 : &set<T>) : boolean := not (s1 = s2);
    
    static function operator<=(s1 : &set<T>; s2 : &set<T>) : boolean := s1.issubset(s2);
    
    static function operator<(s1 : &set<T>; s2 : &set<T>) : boolean := (s1 <= s2) and (s1 <> s2);
    
    static function operator>=(s1 : &set<T>; s2 : &set<T>) : boolean := s1.issuperset(s2);
    
    static function operator>(s1 : &set<T>; s2 : &set<T>) : boolean := (s1 >= s2) and (s1 <> s2);
    
    static function operator or(s1 : &set<T>; s2 : &set<T>) : &set<T> := s1.union(s2);
    
    ///-
    function !orEqual(other : &set<T>) : &set<T>;
    begin
      update(other);
      Result := Self;
    end;
    
    static function operator and(s1 : &set<T>; s2 : &set<T>) : &set<T> := s1.intersection(s2);
    
    ///-
    function !andEqual(other : &set<T>) : &set<T>;
    begin
      intersection_update(other);
      Result := Self;
    end;
    
    static function operator-(s1 : &set<T>; s2 : &set<T>) : &set<T> := s1.difference(s2);
    
    static function operator -=(var s1 : &set<T>; s2 : &set<T>) : &set<T>;
    begin
      s1.difference_update(s2);
    end;
    
    static function operator xor(s1 : &set<T>; s2 : &set<T>) : &set<T> := s1.symmetric_difference(s2);
    
    ///-
    function !xorEqual(other : &set<T>) : &set<T>;
    begin
      symmetric_difference_update(other);
      Result := Self;
    end;
    
//    static function operator:=(var s1: &set<T>; s2: &set<T>): &set<T>;
//    begin
//      s1.wrappee := s2.wrappee.ToHashSet();
//    end;
    
    static function operator implicit(s : HashSet<T>) : &set<T> := new &set<T>(s);
    
    function GetEnumerator() : IEnumerator<T> := wrappee.GetEnumerator();

    function System.Collections.IEnumerable.GetEnumerator() : System.Collections.IEnumerator := GetEnumerator();
end;

type dict<K, V> = class(IEnumerable<PABCSystem.KeyValuePair<K, V>>)
  private
    wrappee : PABCSystem.Dictionary<K, V>;
    
    constructor Create(d : PABCSystem.Dictionary<K, V>) := wrappee := d;
    
    function GetValue(key : K) : V := wrappee[key];
    
    procedure SetValue(key : K; val : V) := wrappee[key] := val;
    
  public
    
    constructor Create() := wrappee := new PABCSystem.Dictionary<K, V>();
    
    constructor Create(capacity : integer) := wrappee := new PABCSystem.Dictionary<K, V>(capacity);
    
    constructor Create(comparer : PABCSystem.IEqualityComparer<K>) := wrappee := new PABCSystem.Dictionary<K, V>(comparer);
    
    constructor Create(capacity : integer; comparer : PABCSystem.IEqualityComparer<K>) := wrappee := new PABCSystem.Dictionary<K, V>(capacity, comparer);
  
    constructor Create(params pairs: array of (K, V));
    begin
      wrappee := new PABCSystem.Dictionary<K, V>();
      
      for var i := 0 to pairs.Length - 1 do
        wrappee[pairs[i].Item1] := pairs[i].Item2;
    end;
  
    constructor Create(seqOfPairs : sequence of (K, V));
    begin
      wrappee := new PABCSystem.Dictionary<K, V>();
      
      foreach var p in seqOfPairs do
        wrappee[p.Item1] := p.Item2;
    end;
  
    ///--
    property !count : integer read wrappee.Count;
  
    property ByKey[key : K] : V read GetValue write SetValue; default;
  
    static function operator in(key: K; Self: dict<K, V>): boolean;
    begin
      Result := Self.wrappee.ContainsKey(key);
    end;
  
    procedure clear() := wrappee.Clear();
    
    function copy() : dict<K, V> := new dict<K, V>(new Dictionary<K,V>(wrappee));
  
    static function fromkeys(sq : sequence of K; val : V := default(V)) : dict<K, V>;
    begin
      Result := new dict<K, V>();
      
      foreach var key in sq do
        Result[key] := val;
    end;
  
    function get(key : K; &default : V := default(V)) : V;
    begin
      var val : V;
      if wrappee.TryGetValue(key, val) then
        Result := val
      else
        Result := &default;
    end;
  
    function items() : sequence of KeyValuePair<K, V> := wrappee;
  
    function keys() := wrappee.Keys;
    
    function pop(key : K) : V;
    begin
      Result := wrappee[key];
      wrappee.Remove(key);
    end;
    
    function pop(key : K; &default : V) : V;
    begin
      var val : V;
      if wrappee.TryGetValue(key, val) then
      begin
        Result := val;
        wrappee.Remove(key);
      end
      else
        Result := &default;
    end;
    
    function popitem() : KeyValuePair<K, V>;
    begin
      Result := wrappee.Last();
      wrappee.Remove(Result.Key);
    end;
    
    function setdefault(key : K; &default : V := default(V)) : V;
    begin
      var val : V;
      if wrappee.TryGetValue(key, val) then
        Result := val
      else
      begin
        wrappee[key] := &default;
        Result := &default;
      end;
    end;
  
    procedure update(params pairs: array of (K, V));
    begin
      for var i := 0 to pairs.Length - 1 do
        wrappee[pairs[i].Item1] := pairs[i].Item2;
    end;
    
    procedure update(seqOfPairs : sequence of (K, V));
    begin
      foreach var p in seqOfPairs do
        wrappee[p.Item1] := p.Item2;
    end;
  
    function values() := wrappee.Values;
    
    static function operator or(d1 : dict<K, V>; d2 : dict<K, V>) : dict<K, V>;
    begin
      Result := new dict<K, V>(d1.wrappee);
      
      foreach var p in d2.wrappee do
        Result[p.Key] := p.Value;
    end;
  
    static function operator implicit(d : PABCSystem.Dictionary<K, V>) : dict<K, V> := new dict<K, V>(d);
  
    function GetEnumerator() : IEnumerator<KeyValuePair<K, V>> := wrappee.GetEnumerator();

    function System.Collections.IEnumerable.GetEnumerator() : System.Collections.IEnumerator := GetEnumerator();
end;

{$endregion STANDARD CONTAINERS}

//Standard functions with Lists

function len<T>(lst: list<T>): integer;
function len<T>(lst: System.Collections.Generic.List<T>): integer;
function len<T>(st: &set<T>): integer;
function len<K, V>(dct: dict<K, V>): integer;
function len<T>(arr: array of T): integer;
function len(s: string): integer;
function len(value: bytes): integer;
function len(value: PythonReadData): integer;

function sorted<T>(lst: list<T>): list<T>;

function sum(s: sequence of boolean): integer;
function sum(s: sequence of integer): integer;
function sum(s: sequence of real): real;

function !assign<T>(var a: T; b: T): T;

function !pow(x, n: integer): integer;

function !pow(x, n: biginteger): biginteger;

function !pow(x: integer; y: real): real;

function !pow(x: real; y: real): real;

function bigint(x: integer): biginteger;
function bigint(x: object): biginteger;

function !pow_recursion(x, n: integer): integer;

function !pow_recursion(x, n: biginteger): biginteger;

// TUPLES BEGIN
  
function !CreateTuple<T>(v: T): System.Tuple<T>;  

function !CreateTuple<T1, T2>(
    v1: T1; v2: T2
    ): System.Tuple<T1, T2>;

function !CreateTuple<T1, T2, T3>(
    v1: T1; v2: T2; v3: T3
    ): System.Tuple<T1, T2, T3>;

function !CreateTuple<T1, T2, T3, T4>(
    v1: T1; v2: T2; v3: T3; v4: T4
    ): System.Tuple<T1, T2, T3, T4>;
 
function !CreateTuple<T1, T2, T3, T4, T5>(
    v1: T1; v2: T2; v3: T3; v4: T4; v5: T5
    ): System.Tuple<T1, T2, T3, T4, T5>;

function !CreateTuple<T1, T2, T3, T4, T5, T6>(
    v1: T1; v2: T2; v3: T3; v4: T4; v5: T5; v6: T6
    ): System.Tuple<T1, T2, T3, T4, T5, T6>;

function !CreateTuple<T1, T2, T3, T4, T5, T6, T7>(
    v1: T1; v2: T2; v3: T3; v4: T4; v5: T5; v6: T6; v7: T7
    ): System.Tuple<T1, T2, T3, T4, T5, T6, T7>;

// TUPLES END

type 
    biginteger = PABCSystem.BigInteger;
    // tuple = System.Tuple;
    tuple<T> = System.Tuple<T>;
    !tuple2<T1, T2> = System.Tuple<T1, T2>;
    !tuple3<T1, T2, T3> = System.Tuple<T1, T2, T3>;
    !tuple4<T1, T2, T3, T4> = System.Tuple<T1, T2, T3, T4>;
    !tuple5<T1, T2, T3, T4, T5> = System.Tuple<T1, T2, T3, T4, T5>;
    !tuple6<T1, T2, T3, T4, T5, T6> = System.Tuple<T1, T2, T3, T4, T5, T6>;
    !tuple7<T1, T2, T3, T4, T5, T6, T7> = System.Tuple<T1, T2, T3, T4, T5, T6, T7>;
    
    ///--
    empty_list = class
    class function operator implicit<T>(x: empty_list): list<T>; 
    begin
      Result := new list<T>();
    end;
    end;
    
    ///--
    empty_set = class
    class function operator implicit<T>(x: empty_set): &set<T>; 
    begin
      Result := new &set<T>();
    end;
    end;
    
    ///--
    empty_dict = class
    class function operator implicit<K, V>(x: empty_dict): dict<K, V>; 
    begin
      Result := new dict<K, V>();
    end;
    end;


function !empty_list(): empty_list;
function !empty_dict(): empty_dict;


implementation

function GetPythonEncoding(name, errors: string): System.Text.Encoding;
begin
  if (name = nil) or (name = '') then
    Result := System.Text.Encoding.Default
  else if (name.ToLower() = 'utf-8') or (name.ToLower() = 'utf8') then
    Result := System.Text.Encoding.UTF8
  else if (name.ToLower() = 'latin-1') or (name.ToLower() = 'latin1') then
    Result := System.Text.Encoding.GetEncoding('iso-8859-1')
  else
    Result := System.Text.Encoding.GetEncoding(name);
  Result := System.Text.Encoding(Result.Clone());
  if (errors = nil) or (errors = 'strict') then
    Result.DecoderFallback := System.Text.DecoderFallback.ExceptionFallback
  else if errors = 'replace' then
    Result.DecoderFallback := System.Text.DecoderFallback.ReplacementFallback
  else if errors = 'ignore' then
    Result.DecoderFallback := new System.Text.DecoderReplacementFallback('')
  else
    raise new System.ArgumentException('unknown error handler: ' + errors);
end;

constructor bytes.Create(value: array of byte);
begin
  if value = nil then data := new byte[0] else data := value;
end;

function bytes.GetItem(index: integer): integer;
begin
  if index < 0 then index += data.Length;
  Result := data[index];
end;

function bytes.to_array(): array of byte := data;

function bytes.GetEnumerator(): IEnumerator<integer>;
begin
  var values := new System.Collections.Generic.List<integer>();
  foreach var value in data do values.Add(value);
  Result := values.GetEnumerator();
end;

function bytes.decode(encoding: string; errors: string): string :=
  GetPythonEncoding(encoding, errors).GetString(data);

function bytes.hex(): string := System.BitConverter.ToString(data).Replace('-', '').ToLowerInvariant();

function bytes.ToString(): string;
begin
  var b := new System.Text.StringBuilder('b''');
  foreach var x in data do
    if x = 39 then b.Append('\''')
    else if x = 92 then b.Append('\\')
    else if x = 10 then b.Append('\n')
    else if x = 13 then b.Append('\r')
    else if x = 9 then b.Append('\t')
    else if (x >= 32) and (x < 127) then b.Append(char(x))
    else b.Append('\x' + x.ToString('x2'));
  b.Append('''');
  Result := b.ToString();
end;

function CheckReadMode(mode: string; binary: boolean): string;
begin
  if (mode = nil) or (mode = '') then mode := 'r';
  if (mode <> 'r') and (mode <> 'rt') and (mode <> 'tr') and
     (mode <> 'rb') and (mode <> 'br') then
    raise new System.ArgumentException('unsupported file mode: ' + mode);
  if binary and (mode <> 'rb') and (mode <> 'br') then
    raise new System.ArgumentException('binary file mode must be rb');
  if not binary and ((mode = 'rb') or (mode = 'br')) then
    raise new System.ArgumentException('binary file mode requires a binary open call');
  Result := mode;
end;

constructor PythonBinaryFile.Create(path: string; mode: string; buffering: integer);
begin
  CheckReadMode(mode, true);
  if buffering < -1 then raise new System.ArgumentException('invalid buffering size');
  stream := new System.IO.FileStream(path, System.IO.FileMode.Open,
    System.IO.FileAccess.Read, System.IO.FileShare.ReadWrite);
end;

function PythonBinaryFile.GetClosed(): boolean := stream = nil;

function PythonBinaryFile.read(size: integer): bytes;
begin
  if closed then raise new System.ObjectDisposedException('file');
  if size < 0 then size := integer(stream.Length - stream.Position);
  var data := new byte[size];
  var count := 0;
  while count < size do
  begin
    var n := stream.Read(data, count, size - count);
    if n = 0 then break;
    count += n;
  end;
  if count <> size then System.Array.Resize(data, count);
  Result := new bytes(data);
end;

function PythonBinaryFile.readline(size: integer): bytes;
begin
  if closed then raise new System.ObjectDisposedException('file');
  var data := new System.Collections.Generic.List<byte>();
  while (size < 0) or (data.Count < size) do
  begin
    var n := stream.ReadByte();
    if n < 0 then break;
    data.Add(byte(n));
    if n = 10 then break;
  end;
  Result := new bytes(data.ToArray());
end;

function PythonBinaryFile.readlines(hint: integer): array of bytes;
begin
  var lines := new System.Collections.Generic.List<bytes>();
  var total := 0;
  while true do
  begin
    var line := readline();
    if line.Length = 0 then break;
    lines.Add(line);
    total += line.Length;
    if (hint > 0) and (total >= hint) then break;
  end;
  Result := lines.ToArray();
end;

function PythonBinaryFile.Lines(): sequence of bytes;
begin
  while true do
  begin
    var line := readline();
    if line.Length = 0 then break;
    yield line;
  end;
end;

function PythonBinaryFile.GetEnumerator(): IEnumerator<bytes> := Lines().GetEnumerator();

function PythonBinaryFile.seek(offset: integer; whence: integer): integer;
begin
  if closed then raise new System.ObjectDisposedException('file');
  if (whence < 0) or (whence > 2) then raise new System.ArgumentException('invalid whence');
  Result := integer(stream.Seek(offset, System.IO.SeekOrigin(whence)));
end;

function PythonBinaryFile.tell(): integer;
begin
  if closed then raise new System.ObjectDisposedException('file');
  Result := integer(stream.Position);
end;

procedure PythonBinaryFile.close();
begin
  if stream <> nil then
  begin
    stream.Dispose();
    stream := nil;
  end;
end;

procedure PythonBinaryFile.Dispose() := close();
function PythonBinaryFile.readable(): boolean := not closed;
function PythonBinaryFile.seekable(): boolean := not closed;

constructor PythonTextFile.Create(path: string; mode: string; buffering: integer;
  encoding: string; errors: string; newline: string);
begin
  CheckReadMode(mode, false);
  if buffering = 0 then raise new System.ArgumentException('cannot have unbuffered text I/O');
  if buffering < -1 then raise new System.ArgumentException('invalid buffering size');
  if (newline <> nil) and (newline <> '') and (newline <> #10) and
     (newline <> #13) and (newline <> #13#10) then
    raise new System.ArgumentException('illegal newline value');
  newlineMode := newline;
  textEncoding := GetPythonEncoding(encoding, errors);
  stream := new System.IO.FileStream(path, System.IO.FileMode.Open,
    System.IO.FileAccess.Read, System.IO.FileShare.ReadWrite);
  reader := new System.IO.StreamReader(stream, textEncoding, false);
  bytePosition := 0;
end;

function PythonTextFile.GetClosed(): boolean := reader = nil;

function PythonTextFile.ReadCharacter(): integer;
begin
  Result := reader.Read();
  if Result >= 0 then bytePosition += textEncoding.GetByteCount(char(Result).ToString());
  if (Result = 13) and (newlineMode = nil) then
  begin
    if reader.Peek() = 10 then
    begin
      reader.Read();
      bytePosition += textEncoding.GetByteCount(#10);
    end;
    Result := 10;
  end;
end;

function PythonTextFile.read(size: integer): string;
begin
  if closed then raise new System.ObjectDisposedException('file');
  var b := new System.Text.StringBuilder();
  while (size < 0) or (b.Length < size) do
  begin
    var c := ReadCharacter();
    if c < 0 then break;
    b.Append(char(c));
  end;
  Result := b.ToString();
end;

function PythonTextFile.readline(size: integer): string;
begin
  if closed then raise new System.ObjectDisposedException('file');
  var b := new System.Text.StringBuilder();
  while (size < 0) or (b.Length < size) do
  begin
    var c := ReadCharacter();
    if c < 0 then break;
    if newlineMode = nil then
    begin
      b.Append(char(c));
      if c = 10 then break;
      continue;
    end;
    b.Append(char(c));
    if newlineMode = '' then
    begin
      if c = 13 then
      begin
        if (reader.Peek() = 10) and ((size < 0) or (b.Length < size)) then
        begin
          b.Append(char(reader.Read()));
          bytePosition += textEncoding.GetByteCount(#10);
        end;
        break;
      end;
      if c = 10 then break;
    end
    else if newlineMode = #13#10 then
    begin
      if (c = 13) and (reader.Peek() = 10) and
         ((size < 0) or (b.Length < size)) then
      begin
        b.Append(char(reader.Read()));
        bytePosition += textEncoding.GetByteCount(#10);
        break;
      end;
    end
    else if (newlineMode = #10) and (c = 10) or
            (newlineMode = #13) and (c = 13) then break;
  end;
  Result := b.ToString();
end;

function PythonTextFile.readlines(hint: integer): array of string;
begin
  var lines := new System.Collections.Generic.List<string>();
  var total := 0;
  while true do
  begin
    var line := readline();
    if line = '' then break;
    lines.Add(line);
    total += line.Length;
    if (hint > 0) and (total >= hint) then break;
  end;
  Result := lines.ToArray();
end;

function PythonTextFile.Lines(): sequence of string;
begin
  while true do
  begin
    var line := readline();
    if line = '' then break;
    yield line;
  end;
end;

function PythonTextFile.GetEnumerator(): IEnumerator<string> := Lines().GetEnumerator();

function PythonTextFile.seek(offset: integer; whence: integer): integer;
begin
  if closed then raise new System.ObjectDisposedException('file');
  if (whence < 0) or (whence > 2) then raise new System.ArgumentException('invalid whence');
  reader.DiscardBufferedData();
  Result := integer(stream.Seek(offset, System.IO.SeekOrigin(whence)));
  bytePosition := Result;
end;

function PythonTextFile.tell(): integer;
begin
  if closed then raise new System.ObjectDisposedException('file');
  Result := bytePosition;
end;

procedure PythonTextFile.close();
begin
  if reader <> nil then
  begin
    reader.Dispose();
    reader := nil;
    stream := nil;
  end;
end;

procedure PythonTextFile.Dispose() := close();
function PythonTextFile.readable(): boolean := not closed;
function PythonTextFile.seekable(): boolean := not closed;

constructor PythonReadData.Create(value: object) := rawValue := value;

function PythonReadData.AsInteger(): integer;
begin
  if not (rawValue is integer) then
    raise new System.InvalidOperationException('value is not an integer');
  Result := integer(rawValue);
end;

function PythonReadData.GetLength(): integer;
begin
  if rawValue is bytes then Result := (rawValue as bytes).Length
  else Result := (rawValue as string).Length;
end;

function PythonReadData.GetItem(index: integer): PythonReadData;
begin
  if rawValue is bytes then Result := new PythonReadData((rawValue as bytes)[index])
  else
  begin
    var s := rawValue as string;
    if index < 0 then index += s.Length;
    Result := new PythonReadData(s[index + 1].ToString());
  end;
end;

function PythonReadData.as_bytes(): bytes;
begin
  if not (rawValue is bytes) then
    raise new System.ArgumentException('a bytes-like object is required');
  Result := rawValue as bytes;
end;
function PythonReadData.decode(encoding: string; errors: string): string := as_bytes().decode(encoding, errors);
function PythonReadData.hex(): string := as_bytes().hex();
function PythonReadData.ToString(): string := rawValue.ToString();

function PythonReadData.GetEnumerator(): IEnumerator<PythonReadData>;
begin
  var values := new System.Collections.Generic.List<PythonReadData>();
  if rawValue is bytes then
    foreach var item in (rawValue as bytes) do values.Add(new PythonReadData(item))
  else
    foreach var item in (rawValue as string) do values.Add(new PythonReadData(item.ToString()));
  Result := values.GetEnumerator();
end;

static function PythonReadData.operator +(a, b: PythonReadData): PythonReadData;
begin
  if (a.rawValue is string) and (b.rawValue is string) then
    Result := new PythonReadData(string(a.rawValue) + string(b.rawValue))
  else Result := new PythonReadData(a.AsInteger() + b.AsInteger());
end;

static function PythonReadData.operator -(a, b: PythonReadData): PythonReadData :=
  new PythonReadData(a.AsInteger() - b.AsInteger());
static function PythonReadData.operator *(a, b: PythonReadData): PythonReadData :=
  new PythonReadData(a.AsInteger() * b.AsInteger());
static function PythonReadData.operator div(a, b: PythonReadData): PythonReadData :=
  new PythonReadData(integer(System.Math.Floor(real(a.AsInteger()) / b.AsInteger())));
static function PythonReadData.operator mod(a, b: PythonReadData): PythonReadData :=
  new PythonReadData(a.AsInteger() - integer(System.Math.Floor(real(a.AsInteger()) / b.AsInteger())) * b.AsInteger());

static function PythonReadData.operator =(a: PythonReadData; b: integer): boolean :=
  System.Object.Equals(a.rawValue, b);
static function PythonReadData.operator =(a: PythonReadData; b: string): boolean :=
  System.Object.Equals(a.rawValue, b);
static function PythonReadData.operator =(a, b: PythonReadData): boolean :=
  System.Object.Equals(a.rawValue, b.rawValue);

constructor PythonFile.Create(path: string; mode: string; buffering: integer;
  encoding: string; errors: string; newline: string);
begin
  if (mode = 'rb') or (mode = 'br') then
    binaryFile := !open_binary(path, mode, buffering, encoding, errors, newline)
  else textFile := !open_text(path, mode, buffering, encoding, errors, newline);
end;

function PythonFile.GetClosed(): boolean :=
  if binaryFile <> nil then binaryFile.closed else textFile.closed;

function PythonFile.read(size: integer): PythonReadData :=
  if binaryFile <> nil then new PythonReadData(binaryFile.read(size))
  else new PythonReadData(textFile.read(size));

function PythonFile.readline(size: integer): PythonReadData :=
  if binaryFile <> nil then new PythonReadData(binaryFile.readline(size))
  else new PythonReadData(textFile.readline(size));

function PythonFile.readlines(hint: integer): System.Collections.Generic.List<PythonReadData>;
begin
  Result := new System.Collections.Generic.List<PythonReadData>();
  if binaryFile <> nil then
    foreach var line in binaryFile.readlines(hint) do Result.Add(new PythonReadData(line))
  else
    foreach var line in textFile.readlines(hint) do Result.Add(new PythonReadData(line));
end;

function PythonFile.Lines(): sequence of PythonReadData;
begin
  while true do
  begin
    var line := readline();
    if line.Length = 0 then break;
    yield line;
  end;
end;

function PythonFile.GetEnumerator(): IEnumerator<PythonReadData> := Lines().GetEnumerator();
function PythonFile.seek(offset: integer; whence: integer): integer :=
  if binaryFile <> nil then binaryFile.seek(offset, whence) else textFile.seek(offset, whence);
function PythonFile.tell(): integer :=
  if binaryFile <> nil then binaryFile.tell() else textFile.tell();
procedure PythonFile.close();
begin
  if binaryFile <> nil then binaryFile.close() else textFile.close();
end;
procedure PythonFile.Dispose() := close();
function PythonFile.readable(): boolean := not closed;
function PythonFile.seekable(): boolean := not closed;

function open(path: string; mode: string; buffering: integer;
  encoding: string; errors: string; newline: string): PythonFile :=
  new PythonFile(path, mode, buffering, encoding, errors, newline);

function !open_text(path: string; mode: string; buffering: integer;
  encoding: string; errors: string; newline: string): PythonTextFile :=
  new PythonTextFile(path, mode, buffering, encoding, errors, newline);

function !open_binary(path: string; mode: string; buffering: integer;
  encoding: string; errors: string; newline: string): PythonBinaryFile;
begin
  if (encoding <> nil) or (errors <> nil) or (newline <> nil) then
    raise new System.ArgumentException('binary mode does not take encoding, errors or newline');
  Result := new PythonBinaryFile(path, mode, buffering);
end;

function input(): string;
begin
  PABCSystem.Print();
  Result := PABCSystem.ReadlnString();
end;

function input(s: string): string;
begin
  PABCSystem.Print(s);
  Result := PABCSystem.ReadlnString();
end;

function enumerate<T>(s: sequence of T; start: integer): sequence of (integer,T)
  := s.Numerate(start);
  
function filter<T>(cond: T -> boolean; s: sequence of T): sequence of T
  := s.Where(cond);

function sorted<T>(s: sequence of T; reverse: boolean): sequence of T
  := reverse ? s.OrderDescending : s.Order;

function sorted<T,T1>(s: sequence of T; key: T -> T1; reverse: boolean): sequence of T
  := reverse ? s.OrderByDescending(key) : s.OrderBy(key);

function int(val: string): integer := integer.Parse(val);

function int(val: real): integer := round(val);

function int(b: boolean): integer;
begin
  if b then
    Result := 1
  else
    Result := 0;
end;

function &type(obj: object): string;
begin
    Result := TypeName(obj)
    .Replace('<', '[')
    .Replace('>', ']')
    .Replace('empty_list', 'list[anytype]')
    .Replace('empty_set', 'set[anytype]')
    .Replace('empty_dict', 'dict[anytype]')
    .Replace('integer', 'int')
    .Replace('string', 'str')
    .Replace('real', 'float')
    .Replace('boolean', 'bool')
    .Replace('System.Numerics.BigInteger', 'bigint');
end;

function int(obj: object): integer;
begin
  try
    Result := Convert.ToInt32(obj);
  except
    on System.InvalidCastException do Result := Convert.ToInt32(obj.ToString());
  end;
end;

function str(val: object): string := val.ToString(); 

function float(val: string): real := real.Parse(val);

function float(x: integer): real := PABCSystem.Floor(x);

function float(x: object): real;
begin
  try
    Result := Convert.ToDouble(x);
  except
    on System.InvalidCastException do Result := Convert.ToDouble(x.ToString());
  end;
end;

function bool(val: integer): boolean := Convert.ToBoolean(val);

function range(s: integer; e: integer; step: integer): sequence of integer;
begin
  Result := PABCSystem.Range(s, e - PABCSystem.Sign(step), step);
end;

function range(s: integer; e: integer): sequence of integer;
begin
  Result := PABCSystem.Range(s, e - 1);
end;

function range(e: integer): sequence of integer;
begin
  Result := PABCSystem.Range(0, e - 1);
end;

//------------------------------------
//     Standard Math functions
//------------------------------------

function abs(x: integer): integer := if x >= 0 then x else -x;
function abs(x: real): real := PABCSystem.Abs(x);

function pow(x,y: real): real := PABCSystem.Power(x,y);

function pow(x: real; n: integer): real := PABCSystem.Power(x,n);

function pow(x: BigInteger; n: integer): BigInteger := PABCSystem.Power(x,n);

function len<T>(lst: list<T>): integer := lst.!count;
function len<T>(lst: System.Collections.Generic.List<T>): integer := lst.Count;
function len<T>(st: &set<T>): integer := st.!count;
function len<K, V>(dct: dict<K, V>): integer := dct.!count;
function len<T>(arr: array of T): integer := arr.Length;
function len(s: string): integer := s.Length;
function len(value: bytes): integer := value.Length;
function len(value: PythonReadData): integer := value.Length;

function sorted<T>(lst: list<T>): list<T>;
begin
  var newList := lst.copy();
  newList.sort();
  Result := newList;
end;

function sum(s: sequence of boolean): integer := s.Select(x -> Convert.ToInt32(x)).Sum();
function sum(s: sequence of integer): integer := s.Sum();
function sum(s: sequence of real): real := s.Sum();

function !assign<T>(var a: T; b: T): T;
begin
  a := b;
  Result := a;
end;

function !pow(x, n: biginteger): biginteger;
begin
  if (n < 0) then
    raise new System.ArgumentException('возведение в степень не работает для целой отрицательной степени типа bigint.');
  Result := !pow_recursion(x, n);
end;

function !pow_recursion(x, n: biginteger): biginteger;
begin
  
  if (n = 0) then
    Result := 1
  else begin
    Result := !pow_recursion(x, n div 2);
    Result *= Result;
    if ((n mod 2) = 1) then
      Result *= x;
  end;
end;

function !pow(x, n: integer): integer;
begin
  if (n < 0) then
    raise new System.ArgumentException('возведение в степень не работает для целой отрицательной степени, используйте привидение к типу с плавающей точкой.');
  Result := !pow_recursion(x, n);
end;

function !pow(x: integer; y: real): real := Power(x, y);

function !pow(x: real; y: real): real := Power(x, y);

function !pow_recursion(x, n: integer): integer;
begin
  
  if (n = 0) then
    Result := 1
  else begin
    Result := !pow_recursion(x, n div 2);
    Result *= Result;
    if ((n mod 2) = 1) then
      Result *= x;
  end;
end;

function bigint(x: integer): biginteger;
begin
  Result := x;
end;

function bigint(x: object): biginteger := biginteger.Parse(x.ToString());

function &set<T>.isdisjoint(other : sequence of T) : boolean;
begin
  if other = nil then
    raise new System.ArgumentNullException('other', 'Null object is not iterable');
  
  var c1 := wrappee.Count;
  var c2 := other.Count();
  
  if (c1 = 0) or (c2 = 0) then
    Result := true
  else
  begin
    if c1 >= c2 then
      Result := other.All(elem -> not wrappee.Contains(elem))
    else
    begin
      var otherSet := new HashSet<T>(other);
      Result := wrappee.All(elem -> not otherSet.Contains(elem));
    end;
  end;
  
end;

function get_keys<K, V>(dct: Dictionary<K, V>):= dct.keys;
function get_values<K, V>(dct: Dictionary<K, V>):= dct.values;

// TUPLES BEGIN

function !CreateTuple<T>(v: T): System.Tuple<T> := System.Tuple.Create(v);

function !CreateTuple<T1, T2>(
    v1: T1; v2: T2
    ): System.Tuple<T1, T2> 
      := (v1, v2);

function !CreateTuple<T1, T2, T3>(
    v1: T1; v2: T2; v3: T3
    ): System.Tuple<T1, T2, T3> 
      := (v1, v2, v3);

function !CreateTuple<T1, T2, T3, T4>(
    v1: T1; v2: T2; v3: T3; v4: T4
    ): System.Tuple<T1, T2, T3, T4> 
      := (v1, v2, v3, v4);
 
function !CreateTuple<T1, T2, T3, T4, T5>(
    v1: T1; v2: T2; v3: T3; v4: T4; v5: T5
    ): System.Tuple<T1, T2, T3, T4, T5> 
      := (v1, v2, v3, v4, v5);

function !CreateTuple<T1, T2, T3, T4, T5, T6>(
    v1: T1; v2: T2; v3: T3; v4: T4; v5: T5; v6: T6
    ): System.Tuple<T1, T2, T3, T4, T5, T6> 
      := (v1, v2, v3, v4, v5, v6);

function !CreateTuple<T1, T2, T3, T4, T5, T6, T7>(
    v1: T1; v2: T2; v3: T3; v4: T4; v5: T5; v6: T6; v7: T7
    ): System.Tuple<T1, T2, T3, T4, T5, T6, T7> 
      := (v1, v2, v3, v4, v5, v6, v7);

// TUPLES END

function all(s: sequence of boolean): boolean;
begin
  Result := true;
  foreach var elem in s do
    if (not elem) then
    begin
      Result := false;
      break;
    end;
end;

function all(s: sequence of integer): boolean := all(s.Select(x -> Convert.ToBoolean(x)));

function any(s: sequence of boolean): boolean;
begin
  Result := false;
  foreach var elem in s do
    if (elem) then
    begin
      Result := true;
      break;
    end;
end;

function any(s: sequence of integer): boolean := any(s.Select(x -> Convert.ToBoolean(x)));

///-
function ToDictionary<T, U>(Self: sequence of System.Tuple<T, U>): dict<T, U>; extensionmethod;
begin
  Result := Self.ToDictionary(x->x[0],x->x[1]);
end;

function !empty_list(): empty_list := new empty_list();
function !empty_dict(): empty_dict := new empty_dict();


function round(val: real): integer := PABCSystem.round(val);

function split(s: string): sequence of string;
begin
  var temp := '';
  var i := 0;
  
  while i <= s.Length - 1 do
  begin
    if (i <= s.Length - 1) and (s.Substring(i, 1) = ' ') then
    begin
      yield temp;
      temp := '';
      i += 1;
    end
    else
    begin
      temp += s[i];
      i += 1;
    end;
  end;
  
  yield temp;
end;

function !format(i: integer; fmt: string): string;
begin
  if ((fmt.ToLower() = 'x') or (fmt = 'b') or (fmt = 'd')) then
  begin
      var HexChars := '0123456789abcdef';
      if (fmt = 'X') then
        HexChars := HexChars.ToUpper();
      var value := Cardinal(i);
      var radix := 10;
      if (fmt.ToLower() = 'x') then radix := 16;
      if (fmt = 'b') then radix := 2;
      if value = 0 then
      begin
        Result := '0';
        Exit;
      end;
    
      while value > 0 do
      begin
        var digit := value mod radix;
        Result := HexChars[digit + 1] + Result;
        value := value div radix;
      end;
      Exit;
  end;
  if (fmt[1] = '.') then
  begin
    Result := !format(i + 0.0, fmt);
    Exit;
  end;
  raise new System.ArgumentException('Неверный формат для целочисленного аргумента');
end;

function !format(val: real; fmt: string): string;
begin
  var digits: integer;
  if (fmt.Length >= 3) and (fmt[1] = '.') and (fmt.EndsWith('f')) then
  begin
    var numStr := fmt.Substring(1, fmt.Length - 2);
    if TryStrToInt(numStr, digits) then
    begin
      var intPart := Trunc(val);
      if (digits = 0) then
      begin
        Result := intPart.ToString;
        Exit;
      end;
      var frac := Abs(val - intPart);

      var fracPart := Round(frac * Power(10, digits));

      var fracStr := fracPart.ToString;
      while fracStr.Length < digits do
        fracStr := '0' + fracStr;

      Result := intPart.ToString + '.' + fracStr;
      Exit;
    end;
  end;

  raise new System.ArgumentException('Неверный формат для вещественного аргумента');
end;

function !format(obj: object; fmt: string): string;
begin
  Result := '';
  raise new System.ArgumentException('Формат не соответствует типу данных выражения в f-строке');
end;

end.
