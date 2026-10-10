unit re1;

// Python re facade. Keep the public API in Pascal so the same PCU works in
// both the classic and portable .NET runtimes.
interface

uses System, System.Collections.Generic, System.Text,
  System.Text.RegularExpressions, SPythonSystem, itertools1;

const
  NOFLAG = 0;
  T = 1;
  &TEMPLATE = T;
  IGNORECASE = 2;
  I = IGNORECASE;
  LOCALE = 4;
  L = LOCALE;
  MULTILINE = 8;
  M = MULTILINE;
  DOTALL = 16;
  S = DOTALL;
  UNICODE = 32;
  U = UNICODE;
  VERBOSE = 64;
  X = VERBOSE;
  DEBUG = 128;
  ASCII = 256;
  A = ASCII;

type
  PatternError = class(System.ArgumentException);
  error = PatternError;

  RePattern = class;

  ReMatch = class
  private
    native: System.Text.RegularExpressions.Match;
    owner: RePattern;
    source: string;
    fromPos, toPos: integer;
    function GetLastIndex(): object;
    function GetLastGroup(): string;
    function GetRegs(): SPythonSystem.list<PyTuple<integer>>;
    function GetItem(index: integer): string;
  public
    constructor Create(m: System.Text.RegularExpressions.Match; p: RePattern;
      s: string; pos, endpos: integer);
    function group(index: integer := 0): string;
    function group(name: string): string;
    function group(first, second: object; params others: array of object): PyTuple<object>;
    function groups(&default: string := nil): PyTuple<object>;
    function groupdict(&default: string := nil): dict<string, object>;
    function start(index: integer := 0): integer;
    function start(name: string): integer;
    function &end(index: integer := 0): integer;
    function &end(name: string): integer;
    function span(index: integer := 0): PyTuple<integer>;
    function span(name: string): PyTuple<integer>;
    function expand(replacement: string): string;
    function ToString(): string; override;
    property pos: integer read fromPos;
    property endpos: integer read toPos;
    property &re: RePattern read owner;
    property lastindex: object read GetLastIndex;
    property lastgroup: string read GetLastGroup;
    property regs: SPythonSystem.list<PyTuple<integer>> read GetRegs;
    property ByIndex[index: integer]: string read GetItem; default;
    property &string: System.String read source;
  end;

  RePattern = class
  private
    native: Regex;
    fullNative: Regex;
    original: string;
    options: integer;
    names: dict<string, integer>;
    function GetGroups(): integer;
    function GetGroupIndex(): dict<string, integer>;
    function MakeMatch(m: System.Text.RegularExpressions.Match; text: string;
      pos, endpos: integer): ReMatch;
    function Scan(text: string; pos, endpos: integer): sequence of ReMatch;
  public
    constructor Create(pattern: string; flags: integer := 0);
    function search(text: string; pos: integer := 0; endpos: integer := -1): ReMatch;
    function &match(text: string; pos: integer := 0; endpos: integer := -1): ReMatch;
    function fullmatch(text: string; pos: integer := 0; endpos: integer := -1): ReMatch;
    function finditer(text: string; pos: integer := 0; endpos: integer := -1): sequence of ReMatch;
    function findall(text: string; pos: integer := 0; endpos: integer := -1): SPythonSystem.list<object>;
    function split(text: string; maxsplit: integer := 0): SPythonSystem.list<object>;
    function sub(repl: string; text: string; count: integer := 0): string;
    function sub(repl: ReMatch -> string; text: string; count: integer := 0): string;
    function sub(repl: PyValue -> PyValue; text: string; count: integer := 0): string;
    function subn(repl: string; text: string; count: integer := 0): PyTuple<object>;
    function subn(repl: ReMatch -> string; text: string; count: integer := 0): PyTuple<object>;
    function subn(repl: PyValue -> PyValue; text: string; count: integer := 0): PyTuple<object>;
    function ToString(): string; override;
    property pattern: string read original;
    property flags: integer read options;
    property groups: integer read GetGroups;
    property groupindex: dict<string, integer> read GetGroupIndex;
  end;

  ReBytesPattern = class;

  ReBytesMatch = class
  private
    inner: ReMatch;
    owner: ReBytesPattern;
    original: bytes;
    function GetLastIndex(): object;
    function GetLastGroup(): string;
    function GetRegs(): SPythonSystem.list<PyTuple<integer>>;
    function GetPos(): integer;
    function GetEndPos(): integer;
    function GetItem(index: integer): bytes;
  public
    constructor Create(m: ReMatch; p: ReBytesPattern; data: bytes);
    function group(index: integer := 0): bytes;
    function group(name: string): bytes;
    function group(first, second: object; params others: array of object): PyTuple<object>;
    function groups(&default: bytes := nil): PyTuple<object>;
    function groupdict(&default: bytes := nil): dict<string, object>;
    function start(index: integer := 0): integer;
    function start(name: string): integer;
    function &end(index: integer := 0): integer;
    function &end(name: string): integer;
    function span(index: integer := 0): PyTuple<integer>;
    function span(name: string): PyTuple<integer>;
    function expand(replacement: bytes): bytes;
    function ToString(): string; override;
    property pos: integer read GetPos;
    property endpos: integer read GetEndPos;
    property &re: ReBytesPattern read owner;
    property lastindex: object read GetLastIndex;
    property lastgroup: string read GetLastGroup;
    property regs: SPythonSystem.list<PyTuple<integer>> read GetRegs;
    property ByIndex[index: integer]: bytes read GetItem; default;
    property &string: bytes read original;
  end;

  ReBytesPattern = class
  private
    inner: RePattern;
    original: bytes;
    actualFlags: integer;
    function GetGroups(): integer;
    function GetGroupIndex(): dict<string, integer>;
    function Wrap(m: ReMatch; data: bytes): ReBytesMatch;
    function Scan(data: bytes; pos, endpos: integer): sequence of ReBytesMatch;
  public
    constructor Create(pattern: bytes; flags: integer := 0);
    function search(data: bytes; pos: integer := 0; endpos: integer := -1): ReBytesMatch;
    function &match(data: bytes; pos: integer := 0; endpos: integer := -1): ReBytesMatch;
    function fullmatch(data: bytes; pos: integer := 0; endpos: integer := -1): ReBytesMatch;
    function finditer(data: bytes; pos: integer := 0; endpos: integer := -1): sequence of ReBytesMatch;
    function findall(data: bytes; pos: integer := 0; endpos: integer := -1): SPythonSystem.list<object>;
    function split(data: bytes; maxsplit: integer := 0): SPythonSystem.list<object>;
    function sub(repl: bytes; data: bytes; count: integer := 0): bytes;
    function sub(repl: ReBytesMatch -> bytes; data: bytes; count: integer := 0): bytes;
    function sub(repl: PyValue -> PyValue; data: bytes; count: integer := 0): bytes;
    function subn(repl: bytes; data: bytes; count: integer := 0): PyTuple<object>;
    function subn(repl: ReBytesMatch -> bytes; data: bytes; count: integer := 0): PyTuple<object>;
    function subn(repl: PyValue -> PyValue; data: bytes; count: integer := 0): PyTuple<object>;
    function ToString(): string; override;
    property pattern: bytes read original;
    property flags: integer read actualFlags;
    property groups: integer read GetGroups;
    property groupindex: dict<string, integer> read GetGroupIndex;
  end;

  Pattern = RePattern;
  RegexFlag = integer;

function compile(pattern: string; flags: integer := 0): RePattern;
function compile(pattern: RePattern; flags: integer := 0): RePattern;
function compile(pattern: bytes; flags: integer := 0): ReBytesPattern;
function compile(pattern: ReBytesPattern; flags: integer := 0): ReBytesPattern;
function search(pattern, text: string; flags: integer := 0): ReMatch;
function &match(pattern, text: string; flags: integer := 0): ReMatch;
function fullmatch(pattern, text: string; flags: integer := 0): ReMatch;
function search(pattern: RePattern; text: string; flags: integer := 0): ReMatch;
function &match(pattern: RePattern; text: string; flags: integer := 0): ReMatch;
function fullmatch(pattern: RePattern; text: string; flags: integer := 0): ReMatch;
function search(pattern, text: bytes; flags: integer := 0): ReBytesMatch;
function &match(pattern, text: bytes; flags: integer := 0): ReBytesMatch;
function fullmatch(pattern, text: bytes; flags: integer := 0): ReBytesMatch;
function search(pattern: ReBytesPattern; text: bytes; flags: integer := 0): ReBytesMatch;
function &match(pattern: ReBytesPattern; text: bytes; flags: integer := 0): ReBytesMatch;
function fullmatch(pattern: ReBytesPattern; text: bytes; flags: integer := 0): ReBytesMatch;
function finditer(pattern, text: string; flags: integer := 0): sequence of ReMatch;
function findall(pattern, text: string; flags: integer := 0): SPythonSystem.list<object>;
function finditer(pattern: RePattern; text: string; flags: integer := 0): sequence of ReMatch;
function findall(pattern: RePattern; text: string; flags: integer := 0): SPythonSystem.list<object>;
function finditer(pattern, text: bytes; flags: integer := 0): sequence of ReBytesMatch;
function findall(pattern, text: bytes; flags: integer := 0): SPythonSystem.list<object>;
function finditer(pattern: ReBytesPattern; text: bytes; flags: integer := 0): sequence of ReBytesMatch;
function findall(pattern: ReBytesPattern; text: bytes; flags: integer := 0): SPythonSystem.list<object>;
function split(pattern, text: string; maxsplit: integer := 0; flags: integer := 0): SPythonSystem.list<object>;
function split(pattern: RePattern; text: string; maxsplit: integer := 0; flags: integer := 0): SPythonSystem.list<object>;
function split(pattern, text: bytes; maxsplit: integer := 0; flags: integer := 0): SPythonSystem.list<object>;
function split(pattern: ReBytesPattern; text: bytes; maxsplit: integer := 0; flags: integer := 0): SPythonSystem.list<object>;
function sub(pattern, repl, text: string; count: integer := 0; flags: integer := 0): string;
function sub(pattern: string; repl: ReMatch -> string; text: string;
  count: integer := 0; flags: integer := 0): string;
function sub(pattern: string; repl: PyValue -> PyValue; text: string;
  count: integer := 0; flags: integer := 0): string;
function subn(pattern, repl, text: string; count: integer := 0; flags: integer := 0): PyTuple<object>;
function subn(pattern: string; repl: ReMatch -> string; text: string;
  count: integer := 0; flags: integer := 0): PyTuple<object>;
function subn(pattern: string; repl: PyValue -> PyValue; text: string;
  count: integer := 0; flags: integer := 0): PyTuple<object>;
function sub(pattern: RePattern; repl, text: string; count: integer := 0; flags: integer := 0): string;
function subn(pattern: RePattern; repl, text: string; count: integer := 0; flags: integer := 0): PyTuple<object>;
function sub(pattern, repl, text: bytes; count: integer := 0; flags: integer := 0): bytes;
function sub(pattern: bytes; repl: ReBytesMatch -> bytes; text: bytes;
  count: integer := 0; flags: integer := 0): bytes;
function sub(pattern: bytes; repl: PyValue -> PyValue; text: bytes;
  count: integer := 0; flags: integer := 0): bytes;
function subn(pattern, repl, text: bytes; count: integer := 0; flags: integer := 0): PyTuple<object>;
function subn(pattern: bytes; repl: ReBytesMatch -> bytes; text: bytes;
  count: integer := 0; flags: integer := 0): PyTuple<object>;
function subn(pattern: bytes; repl: PyValue -> PyValue; text: bytes;
  count: integer := 0; flags: integer := 0): PyTuple<object>;
function sub(pattern: ReBytesPattern; repl, text: bytes; count: integer := 0; flags: integer := 0): bytes;
function subn(pattern: ReBytesPattern; repl, text: bytes; count: integer := 0; flags: integer := 0): PyTuple<object>;
function escape(pattern: string): string;
function escape(pattern: bytes): bytes;
procedure purge();

implementation

function PythonStringRepr(value: string): string;
begin
  var quote := #39;
  if value.Contains(#39) and not value.Contains(#34) then quote := #34;
  var output := new StringBuilder;
  output.Append(quote);
  foreach var c in value do
    case c of
      '\': output.Append('\\');
      #10: output.Append('\n');
      #13: output.Append('\r');
      #9: output.Append('\t');
      #7: output.Append('\x07');
      #8: output.Append('\x08');
      #12: output.Append('\x0c');
      else
        if c = quote then
        begin output.Append('\'); output.Append(c); end
        else if c < #32 then output.Append('\x' + integer(c).ToString('x2'))
        else output.Append(c);
    end;
  output.Append(quote);
  Result := output.ToString();
end;

function FlagRepr(flags: integer): string;
begin
  var names := new List<string>;
  if (flags and IGNORECASE) <> 0 then names.Add('re.IGNORECASE');
  if (flags and LOCALE) <> 0 then names.Add('re.LOCALE');
  if (flags and MULTILINE) <> 0 then names.Add('re.MULTILINE');
  if (flags and DOTALL) <> 0 then names.Add('re.DOTALL');
  if (flags and VERBOSE) <> 0 then names.Add('re.VERBOSE');
  if (flags and DEBUG) <> 0 then names.Add('re.DEBUG');
  if (flags and ASCII) <> 0 then names.Add('re.ASCII');
  Result := System.String.Join('|', names.ToArray());
end;

function Translate(pattern: string; flags: integer): string;
begin
  // .NET numbers unnamed groups before named groups. Name every unnamed
  // capture so all numbers follow Python's left-to-right numbering.
  pattern := Regex.Replace(pattern, '\(\?P<', '(?<');
  pattern := Regex.Replace(pattern, '\(\?P=([A-Za-z_]\w*)\)', '\k<$1>');
  var output := new StringBuilder;
  var inClass := false;
  var unnamed := 0;
  var i := 0;
  while i < pattern.Length do
  begin
    var c := pattern[i+1];
    if c = '\' then
    begin
      if i+1 < pattern.Length then
      begin
        var escaped := pattern[i+2];
        if escaped = 'Z' then output.Append('\z')
        else if (flags and ASCII) <> 0 then
          case escaped of
            'w': output.Append(if inClass then 'A-Za-z0-9_' else '[A-Za-z0-9_]');
            'W': output.Append(if inClass then '\W' else '[^A-Za-z0-9_]');
            'd': output.Append(if inClass then '0-9' else '[0-9]');
            'D': output.Append(if inClass then '\D' else '[^0-9]');
            's': output.Append(if inClass then ' \t\n\r\f\v' else '[ \t\n\r\f\v]');
            'S': output.Append(if inClass then '\S' else '[^ \t\n\r\f\v]');
            'b': output.Append(if inClass then '\x08' else
              '(?:(?<![A-Za-z0-9_])(?=[A-Za-z0-9_])|(?<=[A-Za-z0-9_])(?![A-Za-z0-9_]))');
            'B': output.Append(if inClass then '\B' else
              '(?:(?<=[A-Za-z0-9_])(?=[A-Za-z0-9_])|(?<![A-Za-z0-9_])(?![A-Za-z0-9_]))');
            else begin output.Append(c); output.Append(escaped); end;
          end
        else begin output.Append(c); output.Append(escaped); end;
        i += 2;
      end
      else begin output.Append(c); i += 1; end;
      continue;
    end;
    if c = '[' then inClass := true;
    if c = ']' then inClass := false;
    var capturing := (c = '(') and (not inClass);
    if capturing and (i+1 < pattern.Length) then
      capturing := pattern[i+2] <> '?';
    if capturing then
    begin
      unnamed += 1;
      output.Append('(?<_spython_re_group_');
      output.Append(unnamed);
      output.Append('>');
    end
    else output.Append(c);
    i += 1;
  end;
  Result := output.ToString();
end;

function NativeOptions(flags: integer): RegexOptions;
begin
  if (flags and (not (&TEMPLATE or IGNORECASE or MULTILINE or DOTALL or VERBOSE or
       ASCII or UNICODE or DEBUG or LOCALE))) <> 0 then
    raise new ValueError('invalid regular expression flags');
  if (flags and LOCALE) <> 0 then
    raise new ValueError('cannot use LOCALE flag with a str pattern');
  if ((flags and ASCII) <> 0) and ((flags and UNICODE) <> 0) then
    raise new ValueError('ASCII and UNICODE flags are incompatible');
  Result := RegexOptions.CultureInvariant;
  if (flags and IGNORECASE) <> 0 then Result := Result or RegexOptions.IgnoreCase;
  if (flags and MULTILINE) <> 0 then Result := Result or RegexOptions.Multiline;
  if (flags and DOTALL) <> 0 then Result := Result or RegexOptions.Singleline;
  if (flags and VERBOSE) <> 0 then Result := Result or RegexOptions.IgnorePatternWhitespace;
end;

constructor RePattern.Create(pattern: string; flags: integer);
begin
  if pattern = nil then raise new TypeError('first argument must be string or compiled pattern');
  original := pattern;
  var inline := Regex.Match(pattern, '^\(\?([aiLmsux]+)\)');
  if inline.Success then
  begin
    foreach var flagChar in inline.Groups[1].Value do
      case flagChar of
        'a': flags := flags or ASCII;
        'i': flags := flags or IGNORECASE;
        'L': flags := flags or LOCALE;
        'm': flags := flags or MULTILINE;
        's': flags := flags or DOTALL;
        'u': flags := flags or UNICODE;
        'x': flags := flags or VERBOSE;
      end;
    pattern := pattern.Substring(inline.Length);
  end;
  options := flags;
  if (flags and ASCII) = 0 then options := options or UNICODE;
  var nativeOptions := NativeOptions(flags);
  try
    var translated := Translate(pattern, flags);
    native := new Regex(translated, nativeOptions);
    fullNative := new Regex('\G(?:' + translated + ')\z', nativeOptions);
  except
    on e: System.ArgumentException do raise new PatternError(e.Message);
  end;
  names := new dict<string, integer>;
  foreach var groupName in native.GetGroupNames() do
  begin
    var parsed: integer;
    if not integer.TryParse(groupName, parsed) and
      not groupName.StartsWith('_spython_re_group_', StringComparison.Ordinal) then
      names[groupName] := native.GroupNumberFromName(groupName);
  end;
end;

function RePattern.ToString(): string;
begin
  Result := 're.compile(' + PythonStringRepr(original);
  var visible := options and not UNICODE;
  if visible <> 0 then Result += ', ' + FlagRepr(visible);
  Result += ')';
end;

function RePattern.GetGroups(): integer := native.GetGroupNumbers().Length - 1;
function RePattern.GetGroupIndex(): dict<string, integer> := names.copy();

function NormalizePos(value, size: integer): integer :=
  if value < 0 then 0 else if value > size then size else value;

function NormalizeEnd(value, size: integer): integer :=
  if value < 0 then size else if value > size then size else value;

function RePattern.MakeMatch(m: System.Text.RegularExpressions.Match; text: string;
  pos, endpos: integer): ReMatch :=
  if (m = nil) or (not m.Success) then nil else new ReMatch(m, Self, text, pos, endpos);

function RePattern.search(text: string; pos, endpos: integer): ReMatch;
begin
  if text = nil then raise new TypeError('expected string or bytes-like object');
  pos := NormalizePos(pos, text.Length);
  endpos := NormalizeEnd(endpos, text.Length);
  if pos > endpos then exit(nil);
  Result := MakeMatch(native.Match(text, pos, endpos - pos), text, pos, endpos);
end;

function RePattern.&match(text: string; pos, endpos: integer): ReMatch;
begin
  Result := search(text, pos, endpos);
  if (Result <> nil) and (Result.start() <> NormalizePos(pos, text.Length)) then Result := nil;
end;

function RePattern.fullmatch(text: string; pos, endpos: integer): ReMatch;
begin
  if text = nil then raise new TypeError('expected string or bytes-like object');
  pos := NormalizePos(pos, text.Length);
  endpos := NormalizeEnd(endpos, text.Length);
  if pos > endpos then exit(nil);
  var region := text.Substring(0, endpos);
  Result := MakeMatch(fullNative.Match(region, pos), text, pos, endpos);
end;

function RePattern.Scan(text: string; pos, endpos: integer): sequence of ReMatch;
begin
  if text = nil then raise new TypeError('expected string or bytes-like object');
  pos := NormalizePos(pos, text.Length);
  endpos := NormalizeEnd(endpos, text.Length);
  if pos > endpos then exit;
  var cursor := native.Match(text, pos, endpos - pos);
  while cursor.Success do
  begin
    yield new ReMatch(cursor, Self, text, pos, endpos);
    cursor := cursor.NextMatch();
  end;
end;

function ConsumeOnce<T>(cursor: IEnumerator<T>): sequence of T;
begin
  while cursor.MoveNext() do yield cursor.Current;
end;

function RePattern.finditer(text: string; pos, endpos: integer): sequence of ReMatch :=
  ConsumeOnce(Scan(text, pos, endpos).GetEnumerator());

function RePattern.findall(text: string; pos, endpos: integer): SPythonSystem.list<object>;
begin
  Result := new SPythonSystem.list<object>;
  foreach var m in Scan(text, pos, endpos) do
  begin
    if groups = 0 then Result.append(m.group())
    else if groups = 1 then Result.append(if m.group(1) = nil then '' else m.group(1))
    else
    begin
      var values := new object[groups];
      for var i := 1 to groups do values[i-1] := if m.group(i) = nil then '' else m.group(i);
      Result.append(new PyTuple<object>(values));
    end;
  end;
end;

function RePattern.split(text: string; maxsplit: integer): SPythonSystem.list<object>;
begin
  Result := new SPythonSystem.list<object>;
  var last := 0;
  var count := 0;
  foreach var m in Scan(text, 0, text.Length) do
  begin
    if (maxsplit > 0) and (count >= maxsplit) then break;
    Result.append(text.Substring(last, m.start()-last));
    for var i := 1 to groups do Result.append(m.group(i));
    last := m.&end();
    count += 1;
  end;
  Result.append(text.Substring(last));
end;

function ReplaceCore(p: RePattern; repl: ReMatch -> string;
  text: string; count: integer): PyTuple<object>;
begin
  var builder := new StringBuilder;
  var last := 0;
  var replacements := 0;
  foreach var m in p.finditer(text) do
  begin
    if (count > 0) and (replacements >= count) then break;
    builder.Append(text, last, m.start()-last);
    builder.Append(repl(m));
    last := m.&end();
    replacements += 1;
  end;
  builder.Append(text, last, text.Length-last);
  Result := new PyTuple<object>(new object[](builder.ToString(), replacements));
end;

function RePattern.sub(repl: string; text: string; count: integer): string :=
  string(ReplaceCore(Self, m -> m.expand(repl), text, count)[0]);
function RePattern.sub(repl: ReMatch -> string; text: string; count: integer): string :=
  string(ReplaceCore(Self, repl, text, count)[0]);
function RePattern.sub(repl: PyValue -> PyValue; text: string; count: integer): string :=
  string(ReplaceCore(Self, m -> repl(new PyValue(m)).ToString(), text, count)[0]);
function RePattern.subn(repl: string; text: string; count: integer): PyTuple<object> :=
  ReplaceCore(Self, m -> m.expand(repl), text, count);
function RePattern.subn(repl: ReMatch -> string; text: string; count: integer): PyTuple<object> :=
  ReplaceCore(Self, repl, text, count);
function RePattern.subn(repl: PyValue -> PyValue; text: string; count: integer): PyTuple<object> :=
  ReplaceCore(Self, m -> repl(new PyValue(m)).ToString(), text, count);

constructor ReMatch.Create(m: System.Text.RegularExpressions.Match; p: RePattern;
  s: System.String; pos, endpos: integer);
begin
  native := m; owner := p; source := s; fromPos := pos; toPos := endpos;
end;

function ReMatch.ToString(): System.String :=
  '<re.Match object; span=' + span().ToString() + ', match=' +
  PythonStringRepr(group()) + '>';

function ReMatch.group(index: integer): System.String;
begin
  if (index < 0) or (index > owner.groups) then
    raise new IndexError('no such group');
  var g := native.Groups[index];
  Result := if g.Success then g.Value else nil;
end;

function ReMatch.group(name: System.String): System.String;
begin
  if not (name in owner.names) then
    raise new IndexError('no such group');
  Result := group(owner.names[name]);
end;

function ReMatch.GetItem(index: integer): System.String := group(index);

function ReMatch.group(first, second: object; params others: array of object): PyTuple<object>;
begin
  var values := new object[2 + others.Length];
  values[0] := if first is integer then group(integer(first)) else group(System.String(first));
  values[1] := if second is integer then group(integer(second)) else group(System.String(second));
  for var i := 0 to others.Length-1 do
    values[i+2] := if others[i] is integer then group(integer(others[i])) else group(System.String(others[i]));
  Result := new PyTuple<object>(values);
end;

function ReMatch.groups(&default: System.String): PyTuple<object>;
begin
  var items := new object[owner.groups];
  for var i := 1 to owner.groups do
  begin
    var value := group(i);
    items[i-1] := if value = nil then &default else value;
  end;
  Result := new PyTuple<object>(items);
end;

function ReMatch.groupdict(&default: System.String): dict<System.String, object>;
begin
  Result := new dict<System.String, object>;
  foreach var pair in owner.names do
  begin
    var value := group(pair.Key);
    Result[pair.Key] := if value = nil then &default else value;
  end;
end;

function ReMatch.start(index: integer): integer;
begin
  if (index < 0) or (index > owner.groups) then
    raise new IndexError('no such group');
  Result := if native.Groups[index].Success then native.Groups[index].Index else -1;
end;
function ReMatch.start(name: System.String): integer;
begin
  if not (name in owner.names) then raise new IndexError('no such group');
  Result := start(owner.names[name]);
end;
function ReMatch.&end(index: integer): integer;
begin
  var i := start(index);
  Result := if i < 0 then -1 else i + native.Groups[index].Length;
end;
function ReMatch.&end(name: System.String): integer;
begin
  if not (name in owner.names) then raise new IndexError('no such group');
  Result := &end(owner.names[name]);
end;
function ReMatch.span(index: integer): PyTuple<integer> :=
  new PyTuple<integer>(new integer[](start(index), &end(index)));
function ReMatch.span(name: System.String): PyTuple<integer> :=
  new PyTuple<integer>(new integer[](start(name), &end(name)));

function ReMatch.GetLastIndex(): object;
begin
  Result := nil;
  var latest := -1;
  for var i := 1 to owner.groups do
    if native.Groups[i].Success and
      (native.Groups[i].Index + native.Groups[i].Length > latest) then
    begin
      latest := native.Groups[i].Index + native.Groups[i].Length;
      Result := i;
    end;
end;

function ReMatch.GetLastGroup(): System.String;
begin
  var i := GetLastIndex();
  Result := nil;
  if i = nil then exit;
  foreach var pair in owner.names do
    if pair.Value = integer(i) then exit(pair.Key);
end;

function ReMatch.GetRegs(): SPythonSystem.list<PyTuple<integer>>;
begin
  Result := new SPythonSystem.list<PyTuple<integer>>;
  for var i := 0 to owner.groups do Result.append(span(i));
end;

function ReMatch.expand(replacement: System.String): System.String;
begin
  var b := new StringBuilder;
  var i := 0;
  while i < replacement.Length do
  begin
    if replacement[i+1] <> '\' then
    begin b.Append(replacement[i+1]); i += 1; continue; end;
    i += 1;
    if i >= replacement.Length then raise new PatternError('bad escape (end of pattern)');
    var c := replacement[i+1];
    if c = 'g' then
    begin
      i += 1;
      if i >= replacement.Length then raise new PatternError('missing <');
      if replacement[i+1] <> '<' then raise new PatternError('missing <');
      var close := replacement.IndexOf('>', i+1);
      if close < 0 then raise new PatternError('missing >');
      var key := replacement.Substring(i+1, close-i-1);
      var number: integer;
      var value := if integer.TryParse(key, number) then group(number) else group(key);
      if value <> nil then b.Append(value);
      i := close+1;
      continue;
    end;
    if (c >= '0') and (c <= '9') then
    begin
      var first := i;
      while (i < replacement.Length) and (i-first < 2) do
      begin
        if (replacement[i+1] < '0') or (replacement[i+1] > '9') then break;
        i += 1;
      end;
      var number := integer.Parse(replacement.Substring(first, i-first));
      var value := group(number);
      if value <> nil then b.Append(value);
      continue;
    end;
    case c of
      'n': b.Append(#10);
      'r': b.Append(#13);
      't': b.Append(#9);
      'f': b.Append(#12);
      'v': b.Append(#11);
      'a': b.Append(#7);
      '\': b.Append('\');
      else if char.IsLetter(c) then raise new PatternError('bad escape \' + c)
        else b.Append(c);
    end;
    i += 1;
  end;
  Result := b.ToString();
end;

function LatinText(data: bytes): string;
begin
  if data = nil then raise new TypeError('expected a bytes-like object');
  Result := System.Text.Encoding.GetEncoding(28591).GetString(data.to_array());
end;

function LatinBytes(data: string): bytes :=
  if data = nil then nil else new bytes(System.Text.Encoding.GetEncoding(28591).GetBytes(data));

constructor ReBytesMatch.Create(m: ReMatch; p: ReBytesPattern; data: bytes);
begin inner := m; owner := p; original := data; end;
function ReBytesMatch.ToString(): System.String :=
  '<re.Match object; span=' + span().ToString() + ', match=' +
  group().ToString() + '>';
function ReBytesMatch.GetLastIndex(): object := inner.lastindex;
function ReBytesMatch.GetLastGroup(): System.String := inner.lastgroup;
function ReBytesMatch.GetRegs(): SPythonSystem.list<PyTuple<integer>> := inner.regs;
function ReBytesMatch.GetPos(): integer := inner.pos;
function ReBytesMatch.GetEndPos(): integer := inner.endpos;
function ReBytesMatch.group(index: integer): bytes := LatinBytes(inner.group(index));
function ReBytesMatch.group(name: System.String): bytes := LatinBytes(inner.group(name));
function ReBytesMatch.GetItem(index: integer): bytes := group(index);
function ReBytesMatch.group(first, second: object; params others: array of object): PyTuple<object>;
begin
  var values := new object[2 + others.Length];
  values[0] := if first is integer then group(integer(first)) else group(System.String(first));
  values[1] := if second is integer then group(integer(second)) else group(System.String(second));
  for var i := 0 to others.Length-1 do
    values[i+2] := if others[i] is integer then group(integer(others[i])) else group(System.String(others[i]));
  Result := new PyTuple<object>(values);
end;
function ReBytesMatch.groups(&default: bytes): PyTuple<object>;
begin
  var values := new object[owner.groups];
  for var i := 1 to owner.groups do
  begin
    var value := group(i);
    values[i-1] := if value = nil then &default else value;
  end;
  Result := new PyTuple<object>(values);
end;
function ReBytesMatch.groupdict(&default: bytes): dict<System.String, object>;
begin
  Result := new dict<System.String, object>;
  foreach var pair in owner.groupindex do
  begin
    var value := group(pair.Key);
    Result[pair.Key] := if value = nil then &default else value;
  end;
end;
function ReBytesMatch.start(index: integer): integer := inner.start(index);
function ReBytesMatch.start(name: System.String): integer := inner.start(name);
function ReBytesMatch.&end(index: integer): integer := inner.&end(index);
function ReBytesMatch.&end(name: System.String): integer := inner.&end(name);
function ReBytesMatch.span(index: integer): PyTuple<integer> := inner.span(index);
function ReBytesMatch.span(name: System.String): PyTuple<integer> := inner.span(name);
function ReBytesMatch.expand(replacement: bytes): bytes := LatinBytes(inner.expand(LatinText(replacement)));

constructor ReBytesPattern.Create(pattern: bytes; flags: integer);
begin
  if (flags and (UNICODE or LOCALE)) <> 0 then
    raise new ValueError('UNICODE and LOCALE flags are unsupported for bytes patterns');
  original := pattern;
  actualFlags := flags;
  inner := new RePattern(LatinText(pattern), flags or ASCII);
end;
function ReBytesPattern.ToString(): string;
begin
  Result := 're.compile(' + original.ToString();
  if actualFlags <> 0 then Result += ', ' + FlagRepr(actualFlags);
  Result += ')';
end;
function ReBytesPattern.GetGroups(): integer := inner.groups;
function ReBytesPattern.GetGroupIndex(): dict<System.String, integer> := inner.groupindex;
function ReBytesPattern.Wrap(m: ReMatch; data: bytes): ReBytesMatch :=
  if m = nil then nil else new ReBytesMatch(m, Self, data);
function ReBytesPattern.search(data: bytes; pos, endpos: integer): ReBytesMatch :=
  Wrap(inner.search(LatinText(data), pos, endpos), data);
function ReBytesPattern.&match(data: bytes; pos, endpos: integer): ReBytesMatch :=
  Wrap(inner.&match(LatinText(data), pos, endpos), data);
function ReBytesPattern.fullmatch(data: bytes; pos, endpos: integer): ReBytesMatch :=
  Wrap(inner.fullmatch(LatinText(data), pos, endpos), data);
function ReBytesPattern.Scan(data: bytes; pos, endpos: integer): sequence of ReBytesMatch;
begin
  foreach var m in inner.finditer(LatinText(data), pos, endpos) do
    yield new ReBytesMatch(m, Self, data);
end;
function ReBytesPattern.finditer(data: bytes; pos, endpos: integer): sequence of ReBytesMatch :=
  ConsumeOnce(Scan(data, pos, endpos).GetEnumerator());
function ReBytesPattern.findall(data: bytes; pos, endpos: integer): SPythonSystem.list<object>;
begin
  Result := new SPythonSystem.list<object>;
  foreach var m in Scan(data, pos, endpos) do
  begin
    if groups = 0 then Result.append(m.group())
    else if groups = 1 then Result.append(if m.group(1) = nil then LatinBytes('') else m.group(1))
    else
    begin
      var values := new object[groups];
      for var i := 1 to groups do values[i-1] := if m.group(i) = nil then LatinBytes('') else m.group(i);
      Result.append(new PyTuple<object>(values));
    end;
  end;
end;
function ReBytesPattern.split(data: bytes; maxsplit: integer): SPythonSystem.list<object>;
begin
  Result := new SPythonSystem.list<object>;
  var last := 0;
  var count := 0;
  var raw := data.to_array();
  foreach var m in Scan(data, 0, data.Length) do
  begin
    if (maxsplit > 0) and (count >= maxsplit) then break;
    var part := new byte[m.start()-last];
    System.Array.Copy(raw, last, part, 0, part.Length);
    Result.append(new bytes(part));
    for var i := 1 to groups do Result.append(m.group(i));
    last := m.&end();
    count += 1;
  end;
  var tail := new byte[data.Length-last];
  System.Array.Copy(raw, last, tail, 0, tail.Length);
  Result.append(new bytes(tail));
end;
function ReBytesPattern.subn(repl: bytes; data: bytes; count: integer): PyTuple<object>;
begin
  var pair := inner.subn(LatinText(repl), LatinText(data), count);
  Result := new PyTuple<object>(new object[](LatinBytes(string(pair[0])), pair[1]));
end;
function ReBytesPattern.subn(repl: ReBytesMatch -> bytes; data: bytes; count: integer): PyTuple<object>;
begin
  var pair := inner.subn(m -> LatinText(repl(new ReBytesMatch(m, Self, data))), LatinText(data), count);
  Result := new PyTuple<object>(new object[](LatinBytes(string(pair[0])), pair[1]));
end;
function ReBytesPattern.subn(repl: PyValue -> PyValue; data: bytes; count: integer): PyTuple<object>;
begin
  var pair := inner.subn(m -> LatinText(bytes(repl(new PyValue(new ReBytesMatch(m, Self, data))).value)), LatinText(data), count);
  Result := new PyTuple<object>(new object[](LatinBytes(string(pair[0])), pair[1]));
end;
function ReBytesPattern.sub(repl: bytes; data: bytes; count: integer): bytes :=
  bytes(subn(repl, data, count)[0]);
function ReBytesPattern.sub(repl: ReBytesMatch -> bytes; data: bytes; count: integer): bytes :=
  bytes(subn(repl, data, count)[0]);
function ReBytesPattern.sub(repl: PyValue -> PyValue; data: bytes; count: integer): bytes :=
  bytes(subn(repl, data, count)[0]);

function compile(pattern: string; flags: integer): RePattern := new RePattern(pattern, flags);
function compile(pattern: RePattern; flags: integer): RePattern;
begin
  if flags <> 0 then raise new ValueError('cannot process flags argument with a compiled pattern');
  Result := pattern;
end;
function compile(pattern: bytes; flags: integer): ReBytesPattern := new ReBytesPattern(pattern, flags);
function compile(pattern: ReBytesPattern; flags: integer): ReBytesPattern;
begin
  if flags <> 0 then raise new ValueError('cannot process flags argument with a compiled pattern');
  Result := pattern;
end;
function search(pattern, text: string; flags: integer): ReMatch := compile(pattern, flags).search(text);
function &match(pattern, text: string; flags: integer): ReMatch := compile(pattern, flags).&match(text);
function fullmatch(pattern, text: string; flags: integer): ReMatch := compile(pattern, flags).fullmatch(text);
function search(pattern: RePattern; text: string; flags: integer): ReMatch := compile(pattern, flags).search(text);
function &match(pattern: RePattern; text: string; flags: integer): ReMatch := compile(pattern, flags).&match(text);
function fullmatch(pattern: RePattern; text: string; flags: integer): ReMatch := compile(pattern, flags).fullmatch(text);
function search(pattern, text: bytes; flags: integer): ReBytesMatch := compile(pattern, flags).search(text);
function &match(pattern, text: bytes; flags: integer): ReBytesMatch := compile(pattern, flags).&match(text);
function fullmatch(pattern, text: bytes; flags: integer): ReBytesMatch := compile(pattern, flags).fullmatch(text);
function search(pattern: ReBytesPattern; text: bytes; flags: integer): ReBytesMatch := compile(pattern, flags).search(text);
function &match(pattern: ReBytesPattern; text: bytes; flags: integer): ReBytesMatch := compile(pattern, flags).&match(text);
function fullmatch(pattern: ReBytesPattern; text: bytes; flags: integer): ReBytesMatch := compile(pattern, flags).fullmatch(text);
function finditer(pattern, text: string; flags: integer): sequence of ReMatch := compile(pattern, flags).finditer(text);
function findall(pattern, text: string; flags: integer): SPythonSystem.list<object> := compile(pattern, flags).findall(text);
function finditer(pattern: RePattern; text: string; flags: integer): sequence of ReMatch := compile(pattern, flags).finditer(text);
function findall(pattern: RePattern; text: string; flags: integer): SPythonSystem.list<object> := compile(pattern, flags).findall(text);
function finditer(pattern, text: bytes; flags: integer): sequence of ReBytesMatch := compile(pattern, flags).finditer(text);
function findall(pattern, text: bytes; flags: integer): SPythonSystem.list<object> := compile(pattern, flags).findall(text);
function finditer(pattern: ReBytesPattern; text: bytes; flags: integer): sequence of ReBytesMatch := compile(pattern, flags).finditer(text);
function findall(pattern: ReBytesPattern; text: bytes; flags: integer): SPythonSystem.list<object> := compile(pattern, flags).findall(text);
function split(pattern, text: string; maxsplit, flags: integer): SPythonSystem.list<object> := compile(pattern, flags).split(text, maxsplit);
function split(pattern: RePattern; text: string; maxsplit, flags: integer): SPythonSystem.list<object> := compile(pattern, flags).split(text, maxsplit);
function split(pattern, text: bytes; maxsplit, flags: integer): SPythonSystem.list<object> := compile(pattern, flags).split(text, maxsplit);
function split(pattern: ReBytesPattern; text: bytes; maxsplit, flags: integer): SPythonSystem.list<object> := compile(pattern, flags).split(text, maxsplit);
function sub(pattern, repl, text: string; count, flags: integer): string := compile(pattern, flags).sub(repl, text, count);
function sub(pattern: string; repl: ReMatch -> string; text: string; count, flags: integer): string := compile(pattern, flags).sub(repl, text, count);
function sub(pattern: string; repl: PyValue -> PyValue; text: string; count, flags: integer): string := compile(pattern, flags).sub(repl, text, count);
function subn(pattern, repl, text: string; count, flags: integer): PyTuple<object> := compile(pattern, flags).subn(repl, text, count);
function subn(pattern: string; repl: ReMatch -> string; text: string; count, flags: integer): PyTuple<object> := compile(pattern, flags).subn(repl, text, count);
function subn(pattern: string; repl: PyValue -> PyValue; text: string; count, flags: integer): PyTuple<object> := compile(pattern, flags).subn(repl, text, count);
function sub(pattern: RePattern; repl, text: string; count, flags: integer): string := compile(pattern, flags).sub(repl, text, count);
function subn(pattern: RePattern; repl, text: string; count, flags: integer): PyTuple<object> := compile(pattern, flags).subn(repl, text, count);
function sub(pattern, repl, text: bytes; count, flags: integer): bytes := compile(pattern, flags).sub(repl, text, count);
function sub(pattern: bytes; repl: ReBytesMatch -> bytes; text: bytes; count, flags: integer): bytes := compile(pattern, flags).sub(repl, text, count);
function sub(pattern: bytes; repl: PyValue -> PyValue; text: bytes; count, flags: integer): bytes := compile(pattern, flags).sub(repl, text, count);
function subn(pattern, repl, text: bytes; count, flags: integer): PyTuple<object> := compile(pattern, flags).subn(repl, text, count);
function subn(pattern: bytes; repl: ReBytesMatch -> bytes; text: bytes; count, flags: integer): PyTuple<object> := compile(pattern, flags).subn(repl, text, count);
function subn(pattern: bytes; repl: PyValue -> PyValue; text: bytes; count, flags: integer): PyTuple<object> := compile(pattern, flags).subn(repl, text, count);
function sub(pattern: ReBytesPattern; repl, text: bytes; count, flags: integer): bytes := compile(pattern, flags).sub(repl, text, count);
function subn(pattern: ReBytesPattern; repl, text: bytes; count, flags: integer): PyTuple<object> := compile(pattern, flags).subn(repl, text, count);

function escape(pattern: string): string;
begin
  var b := new StringBuilder;
  foreach var c in pattern do
  begin
    if c in ['(', ')', '[', ']', '{', '}', '?', '*', '+', '-', '|', '^', '$', '\', '.', '&', '~', '#', ' ', #9, #10, #13, #11, #12] then
      b.Append('\');
    b.Append(c);
  end;
  Result := b.ToString();
end;

function escape(pattern: bytes): bytes := LatinBytes(escape(LatinText(pattern)));

procedure purge(); begin end;

end.
