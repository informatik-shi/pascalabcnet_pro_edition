unit struct1;

interface

uses SPythonSystem;

type StructValue = class
private
  rawValue: object;
  function AsNumber(): System.Numerics.BigInteger;
  function AsFloat(): real;
  function IsFloat(): boolean;
public
  constructor Create(value: object);
  property value: object read rawValue;
  function ToString(): string; override;
  function decode(encoding: string := 'utf-8'; errors: string := 'strict'): string;
  function hex(): string;
  static function operator implicit(value: integer): StructValue := new StructValue(value);
  static function operator implicit(value: int64): StructValue := new StructValue(value);
  static function operator implicit(value: uint64): StructValue := new StructValue(value);
  static function operator implicit(value: real): StructValue := new StructValue(value);
  static function operator +(a, b: StructValue): StructValue;
  static function operator -(a, b: StructValue): StructValue;
  static function operator *(a, b: StructValue): StructValue;
  static function operator /(a, b: StructValue): StructValue;
  static function operator div(a, b: StructValue): StructValue;
  static function operator mod(a, b: StructValue): StructValue;
  static function operator -(a: StructValue): StructValue;
  static function operator =(a, b: StructValue): boolean;
  static function operator <(a, b: StructValue): boolean;
  static function operator >(a, b: StructValue): boolean;
  static function operator <=(a, b: StructValue): boolean;
  static function operator >=(a, b: StructValue): boolean;
end;

type StructObject = class
private
  storedFormat: string;
  storedSize: integer;
public
  constructor Create(format: string);
  property format: string read storedFormat;
  property size: integer read storedSize;
  function unpack(buffer: bytes): System.Collections.Generic.List<StructValue>;
  function unpack_from(buffer: bytes; offset: integer := 0): System.Collections.Generic.List<StructValue>;
  function iter_unpack(buffer: bytes): System.Collections.Generic.List<System.Collections.Generic.List<StructValue>>;
end;

function Struct(format: string): StructObject;
function calcsize(format: string): integer;
function unpack(format: string; buffer: bytes): System.Collections.Generic.List<StructValue>;
function unpack_from(format: string; buffer: bytes; offset: integer := 0): System.Collections.Generic.List<StructValue>;
function iter_unpack(format: string; buffer: bytes): System.Collections.Generic.List<System.Collections.Generic.List<StructValue>>;

implementation

constructor StructValue.Create(value: object) := rawValue := value;

function StructValue.ToString(): string := rawValue.ToString();

function StructValue.decode(encoding: string; errors: string): string :=
  (rawValue as bytes).decode(encoding, errors);

function StructValue.hex(): string := (rawValue as bytes).hex();

function StructValue.IsFloat(): boolean := rawValue is real;

function StructValue.AsNumber(): System.Numerics.BigInteger;
begin
  if rawValue is boolean then
    Result := if boolean(rawValue) then new System.Numerics.BigInteger(1)
      else new System.Numerics.BigInteger(0)
  else if rawValue is bytes then
    raise new System.InvalidOperationException('bytes value is not numeric')
  else Result := System.Numerics.BigInteger.Parse(rawValue.ToString());
end;

function StructValue.AsFloat(): real;
begin
  if rawValue is boolean then Result := if boolean(rawValue) then 1.0 else 0.0
  else if rawValue is bytes then
    raise new System.InvalidOperationException('bytes value is not numeric')
  else Result := System.Convert.ToDouble(rawValue.ToString());
end;

static function StructValue.operator +(a, b: StructValue): StructValue :=
  if a.IsFloat() or b.IsFloat() then new StructValue(a.AsFloat() + b.AsFloat())
  else new StructValue(a.AsNumber() + b.AsNumber());

static function StructValue.operator -(a, b: StructValue): StructValue :=
  if a.IsFloat() or b.IsFloat() then new StructValue(a.AsFloat() - b.AsFloat())
  else new StructValue(a.AsNumber() - b.AsNumber());

static function StructValue.operator *(a, b: StructValue): StructValue :=
  if a.IsFloat() or b.IsFloat() then new StructValue(a.AsFloat() * b.AsFloat())
  else new StructValue(a.AsNumber() * b.AsNumber());

static function StructValue.operator /(a, b: StructValue): StructValue :=
  new StructValue(a.AsFloat() / b.AsFloat());

static function StructValue.operator div(a, b: StructValue): StructValue;
begin
  if a.IsFloat() or b.IsFloat() then
    Result := new StructValue(System.Math.Floor(a.AsFloat() / b.AsFloat()))
  else
  begin
    var dividend := a.AsNumber();
    var divisor := b.AsNumber();
    var quotient := System.Numerics.BigInteger.Divide(dividend, divisor);
    var remainder := System.Numerics.BigInteger.Remainder(dividend, divisor);
    if (remainder <> 0) and ((dividend < 0) <> (divisor < 0)) then quotient -= 1;
    Result := new StructValue(quotient);
  end;
end;

static function StructValue.operator mod(a, b: StructValue): StructValue;
begin
  if a.IsFloat() or b.IsFloat() then
    Result := new StructValue(a.AsFloat() - System.Math.Floor(a.AsFloat() / b.AsFloat()) * b.AsFloat())
  else
  begin
    var divisor := b.AsNumber();
    var remainder := System.Numerics.BigInteger.Remainder(a.AsNumber(), divisor);
    if (remainder <> 0) and ((remainder < 0) <> (divisor < 0)) then remainder += divisor;
    Result := new StructValue(remainder);
  end;
end;

static function StructValue.operator -(a: StructValue): StructValue :=
  if a.IsFloat() then new StructValue(-a.AsFloat()) else new StructValue(-a.AsNumber());

static function StructValue.operator =(a, b: StructValue): boolean;
begin
  if (a.rawValue is bytes) or (b.rawValue is bytes) then
    Result := a.ToString() = b.ToString()
  else if a.IsFloat() or b.IsFloat() then Result := a.AsFloat() = b.AsFloat()
  else Result := a.AsNumber() = b.AsNumber();
end;

static function StructValue.operator <(a, b: StructValue): boolean :=
  if a.IsFloat() or b.IsFloat() then a.AsFloat() < b.AsFloat()
  else a.AsNumber() < b.AsNumber();

static function StructValue.operator >(a, b: StructValue): boolean :=
  if a.IsFloat() or b.IsFloat() then a.AsFloat() > b.AsFloat()
  else a.AsNumber() > b.AsNumber();

static function StructValue.operator <=(a, b: StructValue): boolean :=
  if a.IsFloat() or b.IsFloat() then a.AsFloat() <= b.AsFloat()
  else a.AsNumber() <= b.AsNumber();
static function StructValue.operator >=(a, b: StructValue): boolean :=
  if a.IsFloat() or b.IsFloat() then a.AsFloat() >= b.AsFloat()
  else a.AsNumber() >= b.AsNumber();

constructor StructObject.Create(format: string);
begin
  storedFormat := format;
  storedSize := calcsize(format);
end;

function StructObject.unpack(buffer: bytes): System.Collections.Generic.List<StructValue> := struct1.unpack(storedFormat, buffer);
function StructObject.unpack_from(buffer: bytes; offset: integer): System.Collections.Generic.List<StructValue> :=
  struct1.unpack_from(storedFormat, buffer, offset);
function StructObject.iter_unpack(buffer: bytes): System.Collections.Generic.List<System.Collections.Generic.List<StructValue>> :=
  struct1.iter_unpack(storedFormat, buffer);

function Struct(format: string): StructObject := new StructObject(format);

type FormatToken = record
  code: char;
  count: integer;
  size: integer;
  offset: integer;
end;

function FieldSize(code: char; native: boolean): integer;
begin
  case code of
    'x', 'c', 'b', 'B', '?', 's', 'p': Result := 1;
    'h', 'H', 'e': Result := 2;
    'i', 'I', 'l', 'L', 'f': Result := 4;
    'q', 'Q', 'd': Result := 8;
    'n', 'N', 'P':
      begin
        if not native then raise new System.ArgumentException('bad char in struct format');
        Result := System.IntPtr.Size;
      end;
    else raise new System.ArgumentException('bad char in struct format: ' + code);
  end;
end;

function ParseFormat(format: string; var bigEndian: boolean; var total: integer): array of FormatToken;
begin
  if format = nil then raise new System.ArgumentNullException('format');
  var native := true;
  bigEndian := not System.BitConverter.IsLittleEndian;
  var i := 0;
  if (format.Length > 0) and (format[1] in ['@', '=', '<', '>', '!']) then
  begin
    native := format[1] = '@';
    if format[1] = '<' then bigEndian := false;
    if format[1] in ['>', '!'] then bigEndian := true;
    i := 1;
  end;
  var tokens := new System.Collections.Generic.List<FormatToken>();
  total := 0;
  while i < format.Length do
  begin
    var ch := format[i + 1];
    if ch = ' ' then begin i += 1; continue end;
    var count := 0;
    var hasCount := false;
    while (i < format.Length) and (format[i + 1] in ['0'..'9']) do
    begin
      hasCount := true;
      if count > 100000000 then raise new System.ArgumentException('struct format is too large');
      count := count * 10 + integer(format[i + 1]) - integer('0');
      i += 1;
    end;
    if i >= format.Length then raise new System.ArgumentException('repeat count given without format specifier');
    if not hasCount then count := 1;
    ch := format[i + 1];
    i += 1;
    var size := FieldSize(ch, native);
    if native and not (ch in ['x', 's', 'p']) then
      total := (total + size - 1) div size * size;
    var item: FormatToken;
    item.code := ch;
    item.count := count;
    item.size := size;
    item.offset := total;
    tokens.Add(item);
    if ch in ['s', 'p', 'x'] then total := total + count
    else total := total + count * size;
  end;
  Result := tokens.ToArray();
end;

function calcsize(format: string): integer;
begin
  var endian: boolean;
  Result := 0;
  ParseFormat(format, endian, Result);
end;

function Slice(data: array of byte; start, count: integer): array of byte;
begin
  Result := new byte[count];
  System.Array.Copy(data, start, Result, 0, count);
end;

function ReadUnsigned(data: array of byte; start, count: integer; bigEndian: boolean): uint64;
begin
  Result := 0;
  for var j := 0 to count - 1 do
    if bigEndian then Result := (Result shl 8) or uint64(data[start + j])
    else Result := Result or (uint64(data[start + j]) shl (8 * j));
end;

function ReadFloat(data: array of byte; start, count: integer; bigEndian: boolean): object;
begin
  var b := Slice(data, start, count);
  if bigEndian = System.BitConverter.IsLittleEndian then System.Array.Reverse(b);
  if count = 4 then Result := double(System.BitConverter.ToSingle(b, 0))
  else Result := System.BitConverter.ToDouble(b, 0);
end;

function ReadHalf(value: uint64): double;
begin
  var sign := if (value and $8000) <> 0 then -1.0 else 1.0;
  var exponent := integer((value shr 10) and 31);
  var fraction := integer(value and 1023);
  if exponent = 0 then Result := sign * System.Math.Pow(2, -14) * fraction / 1024
  else if exponent = 31 then
    if fraction = 0 then Result := sign * double.PositiveInfinity
    else Result := double.NaN
  else Result := sign * System.Math.Pow(2, exponent - 15) * (1.0 + fraction / 1024.0);
end;

function Decode(format: string; data: array of byte; offset: integer): System.Collections.Generic.List<StructValue>;
begin
  var bigEndian: boolean;
  var size := 0;
  var tokens := ParseFormat(format, bigEndian, size);
  if (offset < 0) or (offset > data.Length) or (size > data.Length - offset) then
    raise new System.ArgumentException('unpack_from requires a buffer of at least ' + size + ' bytes');
  var values := new System.Collections.Generic.List<object>();
  foreach var item in tokens do
  begin
    var pos := offset + item.offset;
    if item.code = 'x' then continue;
    if item.code = 's' then
    begin
      values.Add(new bytes(Slice(data, pos, item.count)));
      continue;
    end;
    if item.code = 'p' then
    begin
      var n := 0;
      if item.count > 0 then n := System.Math.Min(integer(data[pos]), item.count - 1);
      values.Add(new bytes(Slice(data, pos + 1, n)));
      continue;
    end;
    for var j := 0 to item.count - 1 do
    begin
      var at := pos + j * item.size;
      case item.code of
        'c': values.Add(new bytes(Slice(data, at, 1)));
        '?': values.Add(data[at] <> 0);
        'f', 'd': values.Add(ReadFloat(data, at, item.size, bigEndian));
        'e': values.Add(ReadHalf(ReadUnsigned(data, at, 2, bigEndian)));
        'B', 'H', 'I', 'L', 'Q', 'N', 'P':
          begin
            var unsigned := ReadUnsigned(data, at, item.size, bigEndian);
            if unsigned <= integer.MaxValue then values.Add(integer(unsigned))
            else values.Add(unsigned);
          end;
        'b', 'h', 'i', 'l', 'q', 'n':
          begin
            var unsigned := ReadUnsigned(data, at, item.size, bigEndian);
            if item.size = 8 then
            begin
              var b := Slice(data, at, 8);
              if bigEndian = System.BitConverter.IsLittleEndian then System.Array.Reverse(b);
              var signed := System.BitConverter.ToInt64(b, 0);
              if (signed >= integer.MinValue) and (signed <= integer.MaxValue) then
                values.Add(integer(signed))
              else values.Add(signed);
            end
            else if (unsigned and (uint64(1) shl (item.size * 8 - 1))) <> 0 then
              values.Add(integer(int64(unsigned) - (int64(1) shl (item.size * 8))))
            else values.Add(integer(unsigned));
          end;
      end;
    end;
  end;
  Result := new System.Collections.Generic.List<StructValue>(
    values.Select(value -> new StructValue(value)));
end;

function unpack(format: string; buffer: bytes): System.Collections.Generic.List<StructValue>;
begin
  var size := calcsize(format);
  if buffer.Length <> size then
    raise new System.ArgumentException('unpack requires a buffer of ' + size + ' bytes');
  Result := Decode(format, buffer.to_array(), 0);
end;

function unpack_from(format: string; buffer: bytes; offset: integer): System.Collections.Generic.List<StructValue>;
begin
  var data := buffer.to_array();
  if offset < 0 then offset += data.Length;
  Result := Decode(format, data, offset);
end;

function iter_unpack(format: string; buffer: bytes): System.Collections.Generic.List<System.Collections.Generic.List<StructValue>>;
begin
  var size := calcsize(format);
  if size = 0 then raise new System.ArgumentException('cannot iteratively unpack with a struct of length 0');
  if buffer.Length mod size <> 0 then
    raise new System.ArgumentException('iterative unpacking requires a buffer of a multiple of ' + size + ' bytes');
  Result := new System.Collections.Generic.List<System.Collections.Generic.List<StructValue>>();
  for var i := 0 to buffer.Length div size - 1 do
    Result.Add(Decode(format, buffer.to_array(), i * size));
end;

end.
