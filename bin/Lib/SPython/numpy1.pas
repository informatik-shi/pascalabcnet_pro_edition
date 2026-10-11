unit numpy1;

// Native NumPy-style numeric arrays for SPython. All operations run in .NET.
interface

uses System, System.Collections, System.Collections.Generic, System.Globalization,
  System.Text, SPythonSystem;

type
  NativeList = class(IEnumerable<object>)
  private
    items: array of object;
    function GetItem(index: integer): object;
  public
    constructor Create(values: array of object);
    property ByIndex[index: integer]: object read GetItem; default;
    property Length: integer read items.Length;
    function GetEnumerator(): IEnumerator<object>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
    function ToString(): string; override;
  end;

  ArrayShape = class(IEnumerable<integer>)
  private
    dims: array of integer;
    function GetItem(index: integer): integer;
  public
    constructor Create(values: array of integer);
    property ByIndex[index: integer]: integer read GetItem; default;
    property Length: integer read dims.Length;
    function GetEnumerator(): IEnumerator<integer>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
    function ToString(): string; override;
  end;

  NDArray = class(IEnumerable<object>)
  private
    data: array of real;
    dims: array of integer;
    integral: boolean;
    function GetShape(): ArrayShape;
    function GetSize(): integer;
    function GetNDim(): integer;
    function GetDType(): string;
    function GetItem(index: integer): object;
    function Elements(): sequence of object;
    function Nested(offset, axis: integer): object;
    function FormatAt(offset, axis: integer): string;
    function ValueAt(coords: array of integer): real;
    function Reduce(axis: object; operation: string): object;
  public
    constructor Create(values: array of real; dimensions: array of integer; isInteger: boolean);
    property shape: ArrayShape read GetShape;
    property size: integer read GetSize;
    property ndim: integer read GetNDim;
    property dtype: string read GetDType;
    property ByIndex[index: integer]: object read GetItem; default;
    function GetEnumerator(): IEnumerator<object>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
    function tolist(): object;
    function item(index: integer := 0): object;
    function reshape(params dimensions: array of integer): NDArray;
    function astype(dtype: string): NDArray;
    function sum(axis: object := nil): object;
    function mean(axis: object := nil): object;
    function min(axis: object := nil): object;
    function max(axis: object := nil): object;
    function std(axis: object := nil): object;
    function ToString(): string; override;
    static function operator +(a, b: NDArray): NDArray;
    static function operator -(a, b: NDArray): NDArray;
    static function operator *(a, b: NDArray): NDArray;
    static function operator /(a, b: NDArray): NDArray;
    static function operator +(a: NDArray; b: integer): NDArray;
    static function operator -(a: NDArray; b: integer): NDArray;
    static function operator *(a: NDArray; b: integer): NDArray;
    static function operator /(a: NDArray; b: integer): NDArray;
    static function operator +(a: NDArray; b: real): NDArray;
    static function operator -(a: NDArray; b: real): NDArray;
    static function operator *(a: NDArray; b: real): NDArray;
    static function operator /(a: NDArray; b: real): NDArray;
    static function operator +(a: integer; b: NDArray): NDArray;
    static function operator -(a: integer; b: NDArray): NDArray;
    static function operator *(a: integer; b: NDArray): NDArray;
    static function operator /(a: integer; b: NDArray): NDArray;
    static function operator +(a: real; b: NDArray): NDArray;
    static function operator -(a: real; b: NDArray): NDArray;
    static function operator *(a: real; b: NDArray): NDArray;
    static function operator /(a: real; b: NDArray): NDArray;
  end;

const pi = 3.14159265358979323846;
const e = 2.71828182845904523536;

function &array(value: object; dtype: string := nil): NDArray;
function asarray(value: object; dtype: string := nil): NDArray;
function arange(start: object; stop: object := nil; step: object := nil;
  dtype: string := nil): NDArray;
function linspace(start, stop: real; num: integer := 50;
  endpoint: boolean := true; dtype: string := nil): NDArray;
function zeros(shape: object; dtype: string := nil): NDArray;
function ones(shape: object; dtype: string := nil): NDArray;
function full(shape, fill_value: object; dtype: string := nil): NDArray;
function eye(n: integer; m: integer := -1; dtype: string := nil): NDArray;
function reshape(a: NDArray; newshape: object): NDArray;
function concatenate(items: object; axis: integer := 0): NDArray;
function stack(items: object; axis: integer := 0): NDArray;
function sin(x: object): NDArray;
function cos(x: object): NDArray;
function sqrt(x: object): NDArray;
function exp(x: object): NDArray;
function log(x: object): NDArray;
function abs(x: object): NDArray;
function sum(a: NDArray; axis: object := nil): object;
function mean(a: NDArray; axis: object := nil): object;
function min(a: NDArray; axis: object := nil): object;
function max(a: NDArray; axis: object := nil): object;
function std(a: NDArray; axis: object := nil): object;

implementation

function Product(values: array of integer): integer;
begin
  Result := 1;
  foreach var value in values do Result *= value;
end;

function NumberText(value: real; integral: boolean): string;
begin
  if integral then exit(System.Convert.ToInt64(value).ToString(CultureInfo.InvariantCulture));
  Result := value.ToString('G17', CultureInfo.InvariantCulture);
  if (value = System.Math.Truncate(value)) and not Result.Contains('E') then Result += '.0';
end;

constructor NativeList.Create(values: array of object); begin items := values; end;
function NativeList.GetItem(index: integer): object := items[index];
function NativeList.GetEnumerator(): IEnumerator<object> := (items as IEnumerable<object>).GetEnumerator();
function NativeList.ToString(): string;
begin
  var parts := new List<string>;
  foreach var value in items do parts.Add(if value = nil then 'None' else value.ToString());
  Result := '[' + string.Join(', ', parts) + ']';
end;

constructor ArrayShape.Create(values: array of integer); begin dims := values; end;
function ArrayShape.GetItem(index: integer): integer := dims[index];
function ArrayShape.GetEnumerator(): IEnumerator<integer> := (dims as IEnumerable<integer>).GetEnumerator();
function ArrayShape.ToString(): string;
begin
  var parts := new List<string>;
  foreach var d in dims do parts.Add(d.ToString());
  Result := '(' + string.Join(', ', parts) + (if dims.Length = 1 then ',' else '') + ')';
end;

constructor NDArray.Create(values: array of real; dimensions: array of integer; isInteger: boolean);
begin
  foreach var dimension in dimensions do
    if dimension < 0 then raise new ValueError('negative dimensions are not allowed');
  if Product(dimensions) <> values.Length then raise new ValueError('shape does not match array size');
  data := values.ToArray();
  if isInteger then for var i := 0 to data.Length - 1 do data[i] := System.Math.Truncate(data[i]);
  dims := dimensions.ToArray();
  integral := isInteger;
end;
function NDArray.GetShape(): ArrayShape := new ArrayShape(dims.ToArray());
function NDArray.GetSize(): integer := data.Length;
function NDArray.GetNDim(): integer := dims.Length;
function NDArray.GetDType(): string := if integral then 'int64' else 'float64';
function NDArray.GetItem(index: integer): object;
begin
  if ndim = 0 then raise new IndexError('too many indices for array');
  if index < 0 then index += dims[0];
  if (index < 0) or (index >= dims[0]) then raise new IndexError('index out of bounds');
  var tail := dims.Skip(1).ToArray();
  var width := Product(tail);
  if tail.Length = 0 then exit(item(index));
  var part := new real[width];
  System.Array.Copy(data, index * width, part, 0, width);
  Result := new NDArray(part, tail, integral);
end;
function NDArray.Elements(): sequence of object;
begin
  if ndim = 0 then yield item()
  else for var i := 0 to dims[0] - 1 do yield self[i];
end;
function NDArray.GetEnumerator(): IEnumerator<object> := Elements().GetEnumerator();
function NDArray.item(index: integer): object;
begin
  if index < 0 then index += size;
  if (index < 0) or (index >= size) then raise new IndexError('index out of bounds');
  if integral then Result := System.Convert.ToInt64(data[index])
  else Result := data[index];
end;
function NDArray.Nested(offset, axis: integer): object;
begin
  if axis = ndim then exit(item(offset));
  var width := 1;
  for var i := axis + 1 to ndim - 1 do width *= dims[i];
  var parts := new object[dims[axis]];
  for var i := 0 to parts.Length - 1 do parts[i] := Nested(offset + i * width, axis + 1);
  Result := new NativeList(parts);
end;
function NDArray.tolist(): object := Nested(0, 0);
function NDArray.FormatAt(offset, axis: integer): string;
begin
  if axis = ndim then exit(NumberText(data[offset], integral));
  var width := 1;
  for var i := axis + 1 to ndim - 1 do width *= dims[i];
  var parts := new List<string>;
  for var i := 0 to dims[axis] - 1 do parts.Add(FormatAt(offset + i * width, axis + 1));
  Result := '[' + string.Join(if axis = ndim - 1 then ' ' else #10 + ' ', parts) + ']';
end;
function NDArray.ToString(): string := FormatAt(0, 0);

function ShapeFrom(value: object): array of integer;
begin
  if value is integer then exit(new integer[](integer(value)));
  if value is ArrayShape then exit((value as ArrayShape).ToArray());
  if value is IEnumerable then
  begin
    var resultValues := new List<integer>;
    foreach var entry in IEnumerable(value) do resultValues.Add(System.Convert.ToInt32(entry));
    exit(resultValues.ToArray());
  end;
  raise new TypeError('shape must be an integer or a sequence');
end;
function NDArray.reshape(params dimensions: array of integer): NDArray;
begin
  var target := dimensions.ToArray();
  var missing := -1;
  var known := 1;
  for var i := 0 to target.Length - 1 do
    if target[i] = -1 then
    begin
      if missing >= 0 then raise new ValueError('only one unknown dimension is allowed');
      missing := i;
    end
    else
    begin
      if target[i] < 0 then raise new ValueError('negative dimensions are not allowed');
      known *= target[i];
    end;
  if missing >= 0 then
  begin
    if (known = 0) or (size mod known <> 0) then raise new ValueError('cannot reshape array');
    target[missing] := size div known;
  end;
  Result := new NDArray(data.ToArray(), target, integral);
end;
function NDArray.astype(dtype: string): NDArray;
begin
  var ints := dtype.StartsWith('int');
  if not ints and not dtype.StartsWith('float') then raise new TypeError('unsupported dtype: ' + dtype);
  var values := data.ToArray();
  if ints then for var i := 0 to values.Length - 1 do values[i] := System.Math.Truncate(values[i]);
  Result := new NDArray(values, dims.ToArray(), ints);
end;
function NDArray.ValueAt(coords: array of integer): real;
begin
  var offset := 0;
  for var i := 0 to ndim - 1 do offset := offset * dims[i] + coords[i];
  Result := data[offset];
end;

function Broadcast(a, b: NDArray; operation: string): NDArray;
begin
  var rank := System.Math.Max(a.ndim, b.ndim);
  var shape := new integer[rank];
  for var i := 0 to rank - 1 do
  begin
    var ai := i - rank + a.ndim;
    var bi := i - rank + b.ndim;
    var ad := if ai < 0 then 1 else a.dims[ai];
    var bd := if bi < 0 then 1 else b.dims[bi];
    if (ad <> bd) and (ad <> 1) and (bd <> 1) then
      raise new ValueError('operands could not be broadcast together');
    shape[i] := if ad = 1 then bd else ad;
  end;
  var output := new real[Product(shape)];
  var coords := new integer[rank];
  var ac := new integer[a.ndim];
  var bc := new integer[b.ndim];
  for var i := 0 to output.Length - 1 do
  begin
    var remainder := i;
    for var j := rank - 1 downto 0 do
    begin
      coords[j] := remainder mod shape[j];
      remainder := remainder div shape[j];
    end;
    for var j := 0 to a.ndim - 1 do ac[j] := if a.dims[j] = 1 then 0 else coords[rank - a.ndim + j];
    for var j := 0 to b.ndim - 1 do bc[j] := if b.dims[j] = 1 then 0 else coords[rank - b.ndim + j];
    var av := a.ValueAt(ac);
    var bv := b.ValueAt(bc);
    case operation of
      '+': output[i] := av + bv;
      '-': output[i] := av - bv;
      '*': output[i] := av * bv;
      '/': output[i] := av / bv;
    end;
  end;
  Result := new NDArray(output, shape, a.integral and b.integral and (operation <> '/'));
end;

function Scalar(value: real; ints: boolean): NDArray := new NDArray(new real[](value), new integer[0], ints);
static function NDArray.operator +(a, b: NDArray): NDArray := Broadcast(a, b, '+');
static function NDArray.operator -(a, b: NDArray): NDArray := Broadcast(a, b, '-');
static function NDArray.operator *(a, b: NDArray): NDArray := Broadcast(a, b, '*');
static function NDArray.operator /(a, b: NDArray): NDArray := Broadcast(a, b, '/');
static function NDArray.operator +(a: NDArray; b: integer): NDArray := a + Scalar(b, true);
static function NDArray.operator -(a: NDArray; b: integer): NDArray := a - Scalar(b, true);
static function NDArray.operator *(a: NDArray; b: integer): NDArray := a * Scalar(b, true);
static function NDArray.operator /(a: NDArray; b: integer): NDArray := a / Scalar(b, true);
static function NDArray.operator +(a: NDArray; b: real): NDArray := a + Scalar(b, false);
static function NDArray.operator -(a: NDArray; b: real): NDArray := a - Scalar(b, false);
static function NDArray.operator *(a: NDArray; b: real): NDArray := a * Scalar(b, false);
static function NDArray.operator /(a: NDArray; b: real): NDArray := a / Scalar(b, false);
static function NDArray.operator +(a: integer; b: NDArray): NDArray := Scalar(a, true) + b;
static function NDArray.operator -(a: integer; b: NDArray): NDArray := Scalar(a, true) - b;
static function NDArray.operator *(a: integer; b: NDArray): NDArray := Scalar(a, true) * b;
static function NDArray.operator /(a: integer; b: NDArray): NDArray := Scalar(a, true) / b;
static function NDArray.operator +(a: real; b: NDArray): NDArray := Scalar(a, false) + b;
static function NDArray.operator -(a: real; b: NDArray): NDArray := Scalar(a, false) - b;
static function NDArray.operator *(a: real; b: NDArray): NDArray := Scalar(a, false) * b;
static function NDArray.operator /(a: real; b: NDArray): NDArray := Scalar(a, false) / b;

function NDArray.Reduce(axis: object; operation: string): object;
begin
  if axis = nil then
  begin
    if size = 0 then
    begin
      if operation = 'sum' then exit(0);
      raise new ValueError('zero-size array to reduction operation');
    end;
    var total := 0.0;
    var low := data[0];
    var high := data[0];
    foreach var v in data do begin total += v; low := System.Math.Min(low, v); high := System.Math.Max(high, v); end;
    case operation of
      'sum': begin
        if integral then exit(System.Convert.ToInt64(total)) else exit(total);
      end;
      'mean': exit(total / size);
      'min': begin
        if integral then exit(System.Convert.ToInt64(low)) else exit(low);
      end;
      'max': begin
        if integral then exit(System.Convert.ToInt64(high)) else exit(high);
      end;
      'std': begin
        var variance := 0.0;
        foreach var v in data do variance += System.Math.Pow(v - total / size, 2);
        exit(System.Math.Sqrt(variance / size));
      end;
    end;
  end;
  var ax := System.Convert.ToInt32(axis);
  if ax < 0 then ax += ndim;
  if (ax < 0) or (ax >= ndim) then raise new ValueError('axis out of bounds');
  var outputShape := dims.Where((d, i) -> i <> ax).ToArray();
  var output := new real[Product(outputShape)];
  var coords := new integer[ndim];
  for var i := 0 to output.Length - 1 do
  begin
    var remainder := i;
    for var j := outputShape.Length - 1 downto 0 do
    begin
      coords[if j < ax then j else j + 1] := remainder mod outputShape[j];
      remainder := remainder div outputShape[j];
    end;
    var total := 0.0;
    var low := 0.0;
    var high := 0.0;
    for var j := 0 to dims[ax] - 1 do
    begin
      coords[ax] := j;
      var v := ValueAt(coords);
      total += v;
      if j = 0 then begin low := v; high := v; end
      else begin low := System.Math.Min(low, v); high := System.Math.Max(high, v); end;
    end;
    case operation of
      'sum': output[i] := total;
      'mean': output[i] := total / dims[ax];
      'min': output[i] := low;
      'max': output[i] := high;
      'std': begin
        var variance := 0.0;
        for var j := 0 to dims[ax] - 1 do
        begin coords[ax] := j; variance += System.Math.Pow(ValueAt(coords) - total / dims[ax], 2); end;
        output[i] := System.Math.Sqrt(variance / dims[ax]);
      end;
    end;
  end;
  var resultInts := integral and ((operation = 'sum') or (operation = 'min') or (operation = 'max'));
  if outputShape.Length = 0 then
  begin
    if resultInts then exit(System.Convert.ToInt64(output[0])) else exit(output[0]);
  end;
  Result := new NDArray(output, outputShape, resultInts);
end;
function NDArray.sum(axis: object): object := Reduce(axis, 'sum');
function NDArray.mean(axis: object): object := Reduce(axis, 'mean');
function NDArray.min(axis: object): object := Reduce(axis, 'min');
function NDArray.max(axis: object): object := Reduce(axis, 'max');
function NDArray.std(axis: object): object := Reduce(axis, 'std');

procedure Flatten(value: object; values: List<real>; var ints: boolean; var shape: array of integer);
begin
  if value is PyValue then value := (value as PyValue).value;
  if value is NDArray then
  begin
    var arr := value as NDArray;
    shape := arr.dims.ToArray();
    values.AddRange(arr.data);
    ints := ints and arr.integral;
    exit;
  end;
  if (value is IEnumerable) and not (value is string) then
  begin
    var entries := new List<object>;
    foreach var entry in IEnumerable(value) do entries.Add(entry);
    var child: array of integer := nil;
    foreach var entry in entries do
    begin
      var current: array of integer;
      Flatten(entry, values, ints, current);
      if child = nil then child := current
      else if not child.SequenceEqual(current) then raise new ValueError('ragged array');
    end;
    if child = nil then child := new integer[0];
    shape := new integer[child.Length + 1];
    shape[0] := entries.Count;
    System.Array.Copy(child, 0, shape, 1, child.Length);
    exit;
  end;
  if not (value is integer or value is int64 or value is real or value is single or value is boolean) then
    raise new TypeError('only numeric array elements are supported');
  ints := ints and not (value is real or value is single);
  values.Add(System.Convert.ToDouble(value, CultureInfo.InvariantCulture));
  shape := new integer[0];
end;
function &array(value: object; dtype: string): NDArray;
begin
  var values := new List<real>;
  var ints := true;
  var dims: array of integer;
  Flatten(value, values, ints, dims);
  if dtype <> nil then
  begin
    if not dtype.StartsWith('int') and not dtype.StartsWith('float') then raise new TypeError('unsupported dtype: ' + dtype);
    ints := dtype.StartsWith('int');
  end;
  var resultValues := values.ToArray();
  if ints then for var i := 0 to resultValues.Length - 1 do resultValues[i] := System.Math.Truncate(resultValues[i]);
  Result := new NDArray(resultValues, dims, ints);
end;
function asarray(value: object; dtype: string): NDArray := &array(value, dtype);
function arange(start, stop, step: object; dtype: string): NDArray;
begin
  var first := if stop = nil then 0.0 else System.Convert.ToDouble(start);
  var last := if stop = nil then System.Convert.ToDouble(start) else System.Convert.ToDouble(stop);
  var stride := if step = nil then 1.0 else System.Convert.ToDouble(step);
  if stride = 0 then raise new ValueError('step must be nonzero');
  var values := new List<real>;
  var current := first;
  while if stride > 0 then current < last else current > last do
  begin values.Add(current); current += stride; end;
  var ints := if dtype <> nil then dtype.StartsWith('int')
    else not (start is real or stop is real or step is real);
  Result := new NDArray(values.ToArray(), new integer[](values.Count), ints);
end;
function linspace(start, stop: real; num: integer; endpoint: boolean; dtype: string): NDArray;
begin
  if num < 0 then raise new ValueError('number of samples must be non-negative');
  var values := new real[num];
  var divisor := if endpoint then num - 1 else num;
  for var i := 0 to num - 1 do
    values[i] := if divisor = 0 then start else start + (stop - start) * i / divisor;
  if endpoint and (num > 1) then values[num - 1] := stop;
  Result := new NDArray(values, new integer[](num), (dtype <> nil) and dtype.StartsWith('int'));
end;
function full(shape, fill_value: object; dtype: string): NDArray;
begin
  var dims := ShapeFrom(shape);
  var values := new real[Product(dims)];
  var number := System.Convert.ToDouble(fill_value);
  for var i := 0 to values.Length - 1 do values[i] := number;
  var ints := if dtype <> nil then dtype.StartsWith('int')
    else fill_value is integer or fill_value is int64;
  Result := new NDArray(values, dims, ints);
end;
function zeros(shape: object; dtype: string): NDArray := full(shape, 0.0, dtype);
function ones(shape: object; dtype: string): NDArray := full(shape, 1.0, dtype);
function eye(n, m: integer; dtype: string): NDArray;
begin
  if m < 0 then m := n;
  Result := zeros(new integer[](n, m), dtype);
  for var i := 0 to System.Math.Min(n, m) - 1 do Result.data[i * m + i] := 1;
end;
function reshape(a: NDArray; newshape: object): NDArray := a.reshape(ShapeFrom(newshape));

function concatenate(items: object; axis: integer): NDArray;
begin
  var arrays := new List<NDArray>;
  foreach var item in IEnumerable(items) do arrays.Add(asarray(item));
  if arrays.Count = 0 then raise new ValueError('need at least one array to concatenate');
  var first := arrays[0];
  if first.ndim = 0 then raise new ValueError('zero-dimensional arrays cannot be concatenated');
  if axis < 0 then axis += first.ndim;
  if (axis < 0) or (axis >= first.ndim) then raise new ValueError('axis out of bounds');
  var target := first.dims.ToArray();
  target[axis] := 0;
  var ints := true;
  foreach var item in arrays do
  begin
    if item.ndim <> first.ndim then raise new ValueError('arrays must have the same number of dimensions');
    for var i := 0 to first.ndim - 1 do
      if (i <> axis) and (item.dims[i] <> first.dims[i]) then
        raise new ValueError('array dimensions must match except along concatenation axis');
    target[axis] += item.dims[axis];
    ints := ints and item.integral;
  end;
  var outer := 1;
  var inner := 1;
  for var i := 0 to axis - 1 do outer *= first.dims[i];
  for var i := axis + 1 to first.ndim - 1 do inner *= first.dims[i];
  var values := new real[Product(target)];
  var offset := 0;
  for var row := 0 to outer - 1 do
    foreach var item in arrays do
    begin
      var count := item.dims[axis] * inner;
      System.Array.Copy(item.data, row * count, values, offset, count);
      offset += count;
    end;
  Result := new NDArray(values, target, ints);
end;
function stack(items: object; axis: integer): NDArray;
begin
  var arrays := new List<NDArray>;
  foreach var item in IEnumerable(items) do arrays.Add(asarray(item));
  if arrays.Count = 0 then raise new ValueError('need at least one array to stack');
  var first := arrays[0];
  if axis < 0 then axis += first.ndim + 1;
  if (axis < 0) or (axis > first.ndim) then raise new ValueError('axis out of bounds');
  var expanded := new List<NDArray>;
  foreach var item in arrays do
  begin
    if not item.dims.SequenceEqual(first.dims) then raise new ValueError('all input arrays must have the same shape');
    var dimensions := new integer[item.ndim + 1];
    for var i := 0 to dimensions.Length - 1 do
      dimensions[i] := if i = axis then 1 else item.dims[if i < axis then i else i - 1];
    expanded.Add(new NDArray(item.data.ToArray(), dimensions, item.integral));
  end;
  Result := concatenate(expanded, axis);
end;

function Unary(x: object; operation: string): NDArray;
begin
  var input := asarray(x);
  var values := new real[input.size];
  for var i := 0 to values.Length - 1 do
  begin
    var v := input.data[i];
    case operation of
      'sin': values[i] := System.Math.Sin(v);
      'cos': values[i] := System.Math.Cos(v);
      'sqrt': values[i] := System.Math.Sqrt(v);
      'exp': values[i] := System.Math.Exp(v);
      'log': values[i] := System.Math.Log(v);
      'abs': values[i] := System.Math.Abs(v);
    end;
  end;
  Result := new NDArray(values, input.dims.ToArray(), input.integral and (operation = 'abs'));
end;
function sin(x: object): NDArray := Unary(x, 'sin');
function cos(x: object): NDArray := Unary(x, 'cos');
function sqrt(x: object): NDArray := Unary(x, 'sqrt');
function exp(x: object): NDArray := Unary(x, 'exp');
function log(x: object): NDArray := Unary(x, 'log');
function abs(x: object): NDArray := Unary(x, 'abs');
function sum(a: NDArray; axis: object): object := a.sum(axis);
function mean(a: NDArray; axis: object): object := a.mean(axis);
function min(a: NDArray; axis: object): object := a.min(axis);
function max(a: NDArray; axis: object): object := a.max(axis);
function std(a: NDArray; axis: object): object := a.std(axis);

end.
