unit pandas1;

// Native .NET DataFrame and Series for SPython. No CPython calls.
interface

uses System, System.Collections, System.Collections.Generic, System.Globalization,
  System.IO, System.Text, numpy1, SPythonSystem;

type
  SeriesValue = class(IEnumerable<object>)
  private
    cells: array of object;
    labels: array of object;
    title: string;
    function GetItem(key: object): object;
    function GetDType(): string;
    function GetName(): string;
    function GetLength(): integer;
    function Items(): sequence of object;
    function Numeric(op: string): real;
  public
    constructor Create(data: object; index: object := nil; dtype: string := nil;
      name: string := nil);
    property ByIndex[key: object]: object read GetItem; default;
    property dtype: string read GetDType;
    property name: string read GetName;
    property Length: integer read GetLength;
    function GetEnumerator(): IEnumerator<object>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
    function tolist(): NativeList;
    function sum(): object;
    function mean(): real;
    function min(): object;
    function max(): object;
    function head(n: integer := 5): SeriesValue;
    function tail(n: integer := 5): SeriesValue;
    function ToString(): string; override;
    static function operator +(left: SeriesValue; right: integer): SeriesValue;
    static function operator -(left: SeriesValue; right: integer): SeriesValue;
    static function operator *(left: SeriesValue; right: integer): SeriesValue;
    static function operator /(left: SeriesValue; right: integer): SeriesValue;
  end;

  FrameRow = class
  private
    columns: array of object;
    values: array of object;
    function GetItem(key: object): object;
  public
    constructor Create(names, data: array of object);
    property ByIndex[key: object]: object read GetItem; default;
    function ToString(): string; override;
  end;

  FrameValue = class;
  RowLocator = class
  private
    frame: FrameValue;
    positional: boolean;
    function GetItem(key: object): FrameRow;
  public
    constructor Create(owner: FrameValue; byPosition: boolean);
    property ByIndex[key: object]: FrameRow read GetItem; default;
  end;

  FrameValue = class(IEnumerable<object>)
  private
    names: array of object;
    seriesByColumn: array of SeriesValue;
    rowLabels: array of object;
    function GetSeries(key: object): SeriesValue;
    function GetShape(): ArrayShape;
    function GetColumns(): NativeList;
    function GetILoc(): RowLocator;
    function GetLoc(): RowLocator;
    function NamesSequence(): sequence of object;
    function RowAt(position: integer): FrameRow;
    constructor Create(keys: array of object; data: array of SeriesValue; labels: array of object);
  public
    constructor Create(data: object; index: object := nil; selectedColumns: object := nil;
      dtype: string := nil);
    property ByIndex[key: object]: SeriesValue read GetSeries; default;
    property shape: ArrayShape read GetShape;
    property columns: NativeList read GetColumns;
    property iloc: RowLocator read GetILoc;
    property loc: RowLocator read GetLoc;
    function GetEnumerator(): IEnumerator<object>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
    function head(n: integer := 5): FrameValue;
    function tail(n: integer := 5): FrameValue;
    function to_numpy(): NDArray;
    procedure to_csv(path: string; index: boolean := true);
    function ToString(): string; override;
  end;

function DataFrame(data: object; index: object := nil; columns: object := nil;
  dtype: string := nil): FrameValue;
function Series(data: object; index: object := nil; dtype: string := nil;
  name: string := nil): SeriesValue;
function read_csv(path: string; sep: string := ','; header: object := 'infer';
  encoding: string := nil): FrameValue;
function concat(items: object; axis: integer := 0; ignore_index: boolean := false): FrameValue;
function merge(left, right: FrameValue; &on: object := nil; how: string := 'inner'): FrameValue;
function to_datetime(value: object; format: string := nil): object;

implementation

function ValuesOf(value: object): array of object;
begin
  if value is PyValue then value := (value as PyValue).value;
  if (value is IEnumerable) and not (value is string) then
  begin
    var items := new List<object>;
    foreach var item in IEnumerable(value) do items.Add(item);
    exit(items.ToArray());
  end;
  Result := new object[](value);
end;
function LabelsOf(value: object; count: integer): array of object;
begin
  if value <> nil then
  begin
    Result := ValuesOf(value);
    if Result.Length <> count then raise new ValueError('Length of values does not match length of index');
  end
  else
  begin
    Result := new object[count];
    for var i := 0 to count - 1 do Result[i] := i;
  end;
end;
function StringOf(value: object): string :=
  if value = nil then 'None' else System.Convert.ToString(value, CultureInfo.InvariantCulture);
function Number(value: object): real := System.Convert.ToDouble(value, CultureInfo.InvariantCulture);

constructor SeriesValue.Create(data, index: object; dtype, name: string);
begin
  cells := ValuesOf(data);
  labels := LabelsOf(index, cells.Length);
  title := name;
  if dtype <> nil then
    for var i := 0 to cells.Length - 1 do
      if cells[i] <> nil then
      begin
        if dtype.StartsWith('int') then cells[i] := System.Convert.ToInt64(cells[i])
        else if dtype.StartsWith('float') then cells[i] := Number(cells[i])
        else if dtype.StartsWith('str') then cells[i] := StringOf(cells[i]);
      end;
end;
function SeriesValue.GetItem(key: object): object;
begin
  for var i := 0 to labels.Length - 1 do
    if object.Equals(labels[i], key) then exit(cells[i]);
  raise new KeyNotFoundException('KeyError: ' + StringOf(key));
end;
function SeriesValue.GetDType(): string;
begin
  var hasFloat := false;
  var hasText := false;
  foreach var value in cells do
    if value <> nil then
    begin
      if value is string then hasText := true
      else if value is real or value is single then hasFloat := true;
    end;
  Result := if hasText then 'str' else if hasFloat then 'float64' else 'int64';
end;
function SeriesValue.GetName(): string := title;
function SeriesValue.GetLength(): integer := cells.Length;
function SeriesValue.Items(): sequence of object;
begin foreach var value in cells do yield value; end;
function SeriesValue.GetEnumerator(): IEnumerator<object> := Items().GetEnumerator();
function SeriesValue.tolist(): NativeList := new NativeList(cells.ToArray());
function SeriesValue.Numeric(op: string): real;
begin
  Result := if op = 'min' then real.PositiveInfinity else if op = 'max' then real.NegativeInfinity else 0.0;
  var validCount := 0;
  foreach var value in cells do
    if value <> nil then
    begin
      validCount += 1;
      case op of
        'sum', 'mean': Result += Number(value);
        'min': Result := System.Math.Min(Result, Number(value));
        'max': Result := System.Math.Max(Result, Number(value));
      end;
    end;
  if validCount = 0 then
  begin
    if op = 'sum' then exit(0);
    exit(real.NaN);
  end;
  if op = 'mean' then Result /= validCount;
end;
function SeriesValue.sum(): object;
begin
  if dtype = 'int64' then Result := System.Convert.ToInt64(Numeric('sum'))
  else Result := Numeric('sum');
end;
function SeriesValue.mean(): real := Numeric('mean');
function SeriesValue.min(): object;
begin
  var value := Numeric('min');
  if System.Double.IsNaN(value) then Result := value
  else if dtype = 'int64' then Result := System.Convert.ToInt64(value)
  else Result := value;
end;
function SeriesValue.max(): object;
begin
  var value := Numeric('max');
  if System.Double.IsNaN(value) then Result := value
  else if dtype = 'int64' then Result := System.Convert.ToInt64(value)
  else Result := value;
end;
function SeriesValue.head(n: integer): SeriesValue;
begin
  var count := System.Math.Max(0, System.Math.Min(n, Length));
  Result := new SeriesValue(cells.Take(count).ToArray(), labels.Take(count).ToArray(), nil, title);
end;
function SeriesValue.tail(n: integer): SeriesValue;
begin
  var count := System.Math.Max(0, System.Math.Min(n, Length));
  Result := new SeriesValue(cells.Skip(Length - count).ToArray(), labels.Skip(Length - count).ToArray(), nil, title);
end;
function SeriesValue.ToString(): string;
begin
  var lines := new List<string>;
  for var i := 0 to cells.Length - 1 do
    lines.Add(StringOf(labels[i]) + '    ' + StringOf(cells[i]));
  var suffix := if title = nil then '' else 'Name: ' + title + ', ';
  lines.Add(suffix + 'dtype: ' + dtype);
  Result := string.Join(#10, lines);
end;
function SeriesArithmetic(a: SeriesValue; b: integer; op: string): SeriesValue;
begin
  var values := new object[a.Length];
  for var i := 0 to values.Length - 1 do
  begin
    var n := Number(a.cells[i]);
    case op of
      '+': n += b;
      '-': n -= b;
      '*': n *= b;
      '/': n /= b;
    end;
    if (op <> '/') and (a.dtype = 'int64') then values[i] := System.Convert.ToInt64(n)
    else values[i] := n;
  end;
  Result := new SeriesValue(values, a.labels.ToArray(), nil, a.title);
end;
static function SeriesValue.operator +(left: SeriesValue; right: integer): SeriesValue := SeriesArithmetic(left, right, '+');
static function SeriesValue.operator -(left: SeriesValue; right: integer): SeriesValue := SeriesArithmetic(left, right, '-');
static function SeriesValue.operator *(left: SeriesValue; right: integer): SeriesValue := SeriesArithmetic(left, right, '*');
static function SeriesValue.operator /(left: SeriesValue; right: integer): SeriesValue := SeriesArithmetic(left, right, '/');

constructor FrameRow.Create(names, data: array of object);
begin columns := names; values := data; end;
function FrameRow.GetItem(key: object): object;
begin
  for var i := 0 to columns.Length - 1 do
    if object.Equals(columns[i], key) then exit(values[i]);
  raise new KeyNotFoundException('KeyError: ' + StringOf(key));
end;
function FrameRow.ToString(): string;
begin
  var lines := new List<string>;
  for var i := 0 to columns.Length - 1 do lines.Add(StringOf(columns[i]) + '    ' + StringOf(values[i]));
  Result := string.Join(#10, lines);
end;

constructor RowLocator.Create(owner: FrameValue; byPosition: boolean);
begin frame := owner; positional := byPosition; end;
function RowLocator.GetItem(key: object): FrameRow;
begin
  var position := -1;
  if positional then position := System.Convert.ToInt32(key)
  else
    for var i := 0 to frame.rowLabels.Length - 1 do
      if object.Equals(frame.rowLabels[i], key) then begin position := i; break; end;
  if positional and (position < 0) then position += frame.rowLabels.Length;
  if (position < 0) or (position >= frame.rowLabels.Length) then raise new IndexError('row index out of bounds');
  Result := frame.RowAt(position);
end;

constructor FrameValue.Create(keys: array of object; data: array of SeriesValue; labels: array of object);
begin names := keys; seriesByColumn := data; rowLabels := labels; end;
constructor FrameValue.Create(data, index, selectedColumns: object; dtype: string);
begin
  var keys := new List<object>;
  var values := new List<object>;
  if data is IDictionary then
  begin
    var entries := IDictionary(data).GetEnumerator();
    while entries.MoveNext() do
    begin keys.Add(entries.Key); values.Add(entries.Value); end;
  end
  else if data is IEnumerable then
  begin
    foreach var entry in IEnumerable(data) do
    begin
      var entryType := entry.GetType();
      var key := entryType.GetProperty('Key');
      var value := entryType.GetProperty('Value');
      if (key = nil) or (value = nil) then raise new TypeError('DataFrame expects a column dictionary');
      keys.Add(key.GetValue(entry, nil));
      values.Add(value.GetValue(entry, nil));
    end;
  end
  else raise new TypeError('DataFrame expects a column dictionary');
  var width := 0;
  var hasSequence := false;
  foreach var value in values do
    if (value is IEnumerable) and not (value is string) then
    begin
      hasSequence := true;
      width := System.Math.Max(width, ValuesOf(value).Length);
    end;
  if not hasSequence then width := if values.Count = 0 then 0 else 1;
  rowLabels := LabelsOf(index, width);
  names := keys.ToArray();
  seriesByColumn := new SeriesValue[keys.Count];
  for var i := 0 to keys.Count - 1 do
  begin
    var columnValues := ValuesOf(values[i]);
    if (columnValues.Length = 1) and not (values[i] is IEnumerable) then
    begin
      columnValues := new object[width];
      for var j := 0 to width - 1 do columnValues[j] := values[i];
    end;
    if columnValues.Length <> width then raise new ValueError('All arrays must be of the same length');
    seriesByColumn[i] := new SeriesValue(columnValues, rowLabels, dtype, StringOf(keys[i]));
  end;
  if selectedColumns <> nil then
  begin
    var requested := ValuesOf(selectedColumns);
    var reordered := new SeriesValue[requested.Length];
    for var i := 0 to requested.Length - 1 do reordered[i] := GetSeries(requested[i]);
    names := requested;
    seriesByColumn := reordered;
  end;
end;
function FrameValue.GetSeries(key: object): SeriesValue;
begin
  for var i := 0 to names.Length - 1 do if object.Equals(names[i], key) then exit(seriesByColumn[i]);
  raise new KeyNotFoundException('KeyError: ' + StringOf(key));
end;
function FrameValue.GetShape(): ArrayShape := new ArrayShape(new integer[](rowLabels.Length, names.Length));
function FrameValue.GetColumns(): NativeList := new NativeList(names.ToArray());
function FrameValue.GetILoc(): RowLocator := new RowLocator(self, true);
function FrameValue.GetLoc(): RowLocator := new RowLocator(self, false);
function FrameValue.NamesSequence(): sequence of object;
begin foreach var name in names do yield name; end;
function FrameValue.GetEnumerator(): IEnumerator<object> := NamesSequence().GetEnumerator();
function FrameValue.RowAt(position: integer): FrameRow;
begin
  var values := new object[names.Length];
  for var i := 0 to names.Length - 1 do values[i] := seriesByColumn[i].cells[position];
  Result := new FrameRow(names, values);
end;
function FrameValue.head(n: integer): FrameValue;
begin
  var count := System.Math.Max(0, System.Math.Min(n, rowLabels.Length));
  var selected := new SeriesValue[names.Length];
  for var i := 0 to names.Length - 1 do selected[i] := seriesByColumn[i].head(count);
  Result := new FrameValue(names.ToArray(), selected, rowLabels.Take(count).ToArray());
end;
function FrameValue.tail(n: integer): FrameValue;
begin
  var count := System.Math.Max(0, System.Math.Min(n, rowLabels.Length));
  var selected := new SeriesValue[names.Length];
  for var i := 0 to names.Length - 1 do selected[i] := seriesByColumn[i].tail(count);
  Result := new FrameValue(names.ToArray(), selected, rowLabels.Skip(rowLabels.Length - count).ToArray());
end;
function FrameValue.to_numpy(): NDArray;
begin
  var values := new real[rowLabels.Length * names.Length];
  var ints := true;
  for var r := 0 to rowLabels.Length - 1 do
    for var c := 0 to names.Length - 1 do
    begin
      values[r * names.Length + c] := Number(seriesByColumn[c].cells[r]);
      ints := ints and (seriesByColumn[c].dtype = 'int64');
    end;
  Result := new NDArray(values, new integer[](rowLabels.Length, names.Length), ints);
end;
function CsvField(value: object): string;
begin
  Result := if value = nil then '' else StringOf(value);
  if Result.Contains(',') or Result.Contains('"') or Result.Contains(#10) or Result.Contains(#13) then
    Result := '"' + Result.Replace('"', '""') + '"';
end;
procedure FrameValue.to_csv(path: string; index: boolean);
begin
  var text := new StringBuilder;
  var header := new List<string>;
  if index then header.Add('');
  foreach var name in names do header.Add(CsvField(name));
  text.AppendLine(string.Join(',', header));
  for var r := 0 to rowLabels.Length - 1 do
  begin
    var fields := new List<string>;
    if index then fields.Add(CsvField(rowLabels[r]));
    for var c := 0 to names.Length - 1 do fields.Add(CsvField(seriesByColumn[c].cells[r]));
    text.AppendLine(string.Join(',', fields));
  end;
  System.IO.File.WriteAllText(path, text.ToString(), new UTF8Encoding(false));
end;
function FrameValue.ToString(): string;
begin
  var lines := new List<string>;
  var header := new StringBuilder('   ');
  for var c := 0 to names.Length - 1 do header.Append(' ').Append(StringOf(names[c]));
  lines.Add(header.ToString().TrimEnd());
  for var r := 0 to rowLabels.Length - 1 do
  begin
    var line := new StringBuilder(StringOf(rowLabels[r]));
    for var c := 0 to names.Length - 1 do line.Append('  ').Append(StringOf(seriesByColumn[c].cells[r]));
    lines.Add(line.ToString());
  end;
  Result := string.Join(#10, lines);
end;

function DataFrame(data, index, columns: object; dtype: string): FrameValue :=
  new FrameValue(data, index, columns, dtype);
function Series(data, index: object; dtype, name: string): SeriesValue :=
  new SeriesValue(data, index, dtype, name);
function read_csv(path, sep: string; header: object; encoding: string): FrameValue;
begin
  if sep.Length <> 1 then raise new ValueError('separator must be one character');
  var source := System.IO.File.ReadAllText(path,
    if encoding = nil then System.Text.Encoding.UTF8 else System.Text.Encoding.GetEncoding(encoding));
  var rows := new List<array of string>;
  var fields := new List<string>;
  var cell := new StringBuilder;
  var quoted := false;
  var position := 1;
  while position <= source.Length do
  begin
    var ch := source[position];
    if quoted then
    begin
      if ch = '"' then
      begin
        if (position + 1 <= source.Length) and (source[position + 1] = '"') then
        begin cell.Append('"'); position += 1; end
        else quoted := false;
      end
      else cell.Append(ch);
    end
    else if ch = '"' then quoted := true
    else if ch = sep[1] then
    begin fields.Add(cell.ToString()); cell.Clear(); end
    else if (ch = #10) or (ch = #13) then
    begin
      if (ch = #13) and (position + 1 <= source.Length) and (source[position + 1] = #10) then position += 1;
      fields.Add(cell.ToString()); cell.Clear();
      rows.Add(fields.ToArray()); fields.Clear();
    end
    else cell.Append(ch);
    position += 1;
  end;
  if quoted then raise new ValueError('unclosed quote in CSV');
  if (cell.Length > 0) or (fields.Count > 0) then
  begin fields.Add(cell.ToString()); rows.Add(fields.ToArray()); end;
  if rows.Count = 0 then raise new ValueError('No columns to parse from file');
  var firstDataRow := if header = nil then 0 else 1;
  var keys := new object[rows[0].Length];
  for var c := 0 to keys.Length - 1 do
    if header = nil then keys[c] := c else keys[c] := rows[0][c];
  var pairs := new Dictionary<object, object>;
  for var c := 0 to keys.Length - 1 do
  begin
    var column := new object[rows.Count - firstDataRow];
    for var r := firstDataRow to rows.Count - 1 do
    begin
      if rows[r].Length <> keys.Length then raise new ValueError('inconsistent number of CSV fields');
      var raw := rows[r][c];
      if raw = '' then column[r - firstDataRow] := nil
      else
      begin
        var whole: int64;
        var fraction: real;
        if int64.TryParse(raw, NumberStyles.Integer, CultureInfo.InvariantCulture, whole) then
          column[r - firstDataRow] := whole
        else if real.TryParse(raw, NumberStyles.Float, CultureInfo.InvariantCulture, fraction) then
          column[r - firstDataRow] := fraction
        else column[r - firstDataRow] := raw;
      end;
    end;
    pairs.Add(keys[c], column);
  end;
  Result := new FrameValue(pairs);
end;
function concat(items: object; axis: integer; ignore_index: boolean): FrameValue;
begin
  if axis <> 0 then raise new ValueError('only axis=0 is implemented');
  var frames := new List<FrameValue>;
  foreach var item in IEnumerable(items) do frames.Add(item as FrameValue);
  if frames.Count = 0 then raise new ValueError('No objects to concatenate');
  var first := frames[0];
  var values := new Dictionary<object, object>;
  foreach var name in first.names do
  begin
    var combined := new List<object>;
    foreach var frame in frames do combined.AddRange(frame[name].cells);
    values.Add(name, combined.ToArray());
  end;
  Result := new FrameValue(values);
  if not ignore_index then
  begin
    var combinedLabels := new List<object>;
    foreach var frame in frames do combinedLabels.AddRange(frame.rowLabels);
    Result.rowLabels := combinedLabels.ToArray();
    foreach var column in Result.seriesByColumn do column.labels := Result.rowLabels;
  end;
end;
function merge(left, right: FrameValue; &on: object; how: string): FrameValue;
begin
  if &on = nil then raise new ValueError('merge requires on');
  if how <> 'inner' then raise new ValueError('only inner merge is implemented');
  var values := new Dictionary<object, object>;
  foreach var name in left.names do values.Add(name, new List<object>);
  foreach var name in right.names do
    if not object.Equals(name, &on) then values.Add(name, new List<object>);
  for var i := 0 to left.rowLabels.Length - 1 do
    for var j := 0 to right.rowLabels.Length - 1 do
      if object.Equals(left[&on].cells[i], right[&on].cells[j]) then
      begin
        foreach var name in left.names do (values[name] as List<object>).Add(left[name].cells[i]);
        foreach var name in right.names do
          if not object.Equals(name, &on) then (values[name] as List<object>).Add(right[name].cells[j]);
      end;
  Result := new FrameValue(values);
end;
function to_datetime(value: object; format: string): object;
begin
  if (value is IEnumerable) and not (value is string) then
  begin
    var items := ValuesOf(value);
    var dates := new object[items.Length];
    for var i := 0 to items.Length - 1 do
      dates[i] := if format = nil then System.DateTime.Parse(StringOf(items[i]), CultureInfo.InvariantCulture)
        else System.DateTime.ParseExact(StringOf(items[i]), format, CultureInfo.InvariantCulture);
    exit(new NativeList(dates));
  end;
  Result := if format = nil then System.DateTime.Parse(StringOf(value), CultureInfo.InvariantCulture)
    else System.DateTime.ParseExact(StringOf(value), format, CultureInfo.InvariantCulture);
end;

end.
