unit pyplot1;

// SPython facade for the real CPython Matplotlib. Python objects are kept in
// the bridge process; this avoids copying future NumPy/pandas objects on every
// plot call. The public signatures cover the common pyplot/Axes workflow.
interface

uses System, System.Collections, System.Collections.Generic, System.Diagnostics,
  System.Globalization, System.IO, System.Text, SPythonSystem;

type
  PlotObject = class(IEnumerable<PlotObject>)
  private
    id: integer;
    function GetLength(): integer;
    function GetItem(index: integer): PlotObject;
    function Items(): sequence of PlotObject;
  public
    constructor Create(value: integer);
    property Handle: integer read id;
    property Length: integer read GetLength;
    property ByIndex[index: integer]: PlotObject read GetItem; default;
    function GetEnumerator(): IEnumerator<PlotObject>;
    function System.Collections.IEnumerable.GetEnumerator(): System.Collections.IEnumerator := GetEnumerator();
    function plot(x, y: object; fmt: string := nil; color: string := nil;
      &label: string := nil; linewidth: real := real.NaN; linestyle: string := nil;
      marker: string := nil; alpha: real := real.NaN): PlotObject;
    function plot(y: object): PlotObject;
    function scatter(x, y: object; s: object := nil; c: object := nil;
      marker: string := nil; cmap: string := nil; &label: string := nil;
      alpha: real := real.NaN): PlotObject;
    function bar(x, height: object; width: real := real.NaN;
      color: string := nil; &label: string := nil): PlotObject;
    function hist(x: object; bins: integer := -1; color: string := nil;
      &label: string := nil; density: boolean := false): PlotObject;
    function imshow(x: object; cmap: string := nil; interpolation: string := nil): PlotObject;
    function set_title(text: string): PlotObject;
    function set_xlabel(text: string): PlotObject;
    function set_ylabel(text: string): PlotObject;
    function legend(): PlotObject;
    function grid(visible: boolean := true): PlotObject;
    function set_xlim(left, right: real): PlotObject;
    function set_ylim(bottom, top: real): PlotObject;
    function get_xlim(): PlotObject;
    function get_ylim(): PlotObject;
    function tight_layout(): PlotObject;
    function savefig(fname: string; dpi: real := real.NaN;
      bbox_inches: string := nil; transparent: boolean := false): PlotObject;
    function gca(): PlotObject;
    function ToString(): string; override;
  end;

// Shared entry points for future NumPy and pandas facades. Remote objects
// retain identity in the same Python process as pyplot.
function PythonCall(moduleName, name: string; args: array of object;
  kwargs: Dictionary<string, object>): object;
function PythonMethod(target: PlotObject; name: string; args: array of object;
  kwargs: Dictionary<string, object>): object;
function PythonAttribute(target: PlotObject; name: string): object;
function PythonIndex(target: PlotObject; index: integer): object;

function plot(x, y: object; fmt: string := nil; color: string := nil;
  &label: string := nil; linewidth: real := real.NaN; linestyle: string := nil;
  marker: string := nil; alpha: real := real.NaN): PlotObject;
function plot(y: object): PlotObject;
function scatter(x, y: object; s: object := nil; c: object := nil;
  marker: string := nil; cmap: string := nil; &label: string := nil;
  alpha: real := real.NaN): PlotObject;
function bar(x, height: object; width: real := real.NaN;
  color: string := nil; &label: string := nil): PlotObject;
function hist(x: object; bins: integer := -1; color: string := nil;
  &label: string := nil; density: boolean := false): PlotObject;
function imshow(x: object; cmap: string := nil; interpolation: string := nil): PlotObject;
function figure(num: integer := -1; figsize: object := nil; dpi: real := real.NaN): PlotObject;
function subplots(nrows: integer := 1; ncols: integer := 1;
  figsize: object := nil; sharex: boolean := false; sharey: boolean := false): sequence of PlotObject;
function subplot(nrows, ncols, index: integer): PlotObject;
function gca(): PlotObject;
function gcf(): PlotObject;
function title(text: string): PlotObject;
function xlabel(text: string): PlotObject;
function ylabel(text: string): PlotObject;
function legend(): PlotObject;
function grid(visible: boolean := true): PlotObject;
function xlim(left, right: real): PlotObject;
function ylim(bottom, top: real): PlotObject;
function tight_layout(): PlotObject;
procedure savefig(fname: string; dpi: real := real.NaN;
  bbox_inches: string := nil; transparent: boolean := false);
procedure show();
procedure close();
procedure close(fig: PlotObject);
procedure clf();
procedure cla();

implementation

var worker: Process;
var gate := new object;

function Quote(s: string): string;
begin
  Result := '"' + s.Replace('\', '\\').Replace('"', '\"')
    .Replace(#13, '\r').Replace(#10, '\n').Replace(#9, '\t') + '"';
  // .NET Framework's redirected stdin uses a system code page. ASCII-only
  // JSON keeps Cyrillic labels and paths intact on every Windows locale.
  var ascii := new StringBuilder;
  foreach var ch in Result do
    if (ord(ch) < 32) or (ord(ch) > 127) then
      ascii.Append('\u').Append(ord(ch).ToString('X4'))
    else ascii.Append(ch);
  Result := ascii.ToString();
end;

function Encode(value: object): string;
begin
  if value = nil then exit('null');
  if value is PyValue then exit(Encode((value as PyValue).value));
  if value is PlotObject then exit('{"ref":' + (value as PlotObject).Handle.ToString() + '}');
  if value is string then exit(Quote(string(value)));
  if value is char then exit(Quote(System.Convert.ToString(value)));
  if value is boolean then exit(if boolean(value) then 'true' else 'false');
  if value is integer or value is int64 or value is uint64 or
     value is byte or value is shortint then
    exit(System.Convert.ToString(value, CultureInfo.InvariantCulture));
  if value is real or value is single then
  begin
    var number := System.Convert.ToDouble(value, CultureInfo.InvariantCulture);
    if System.Double.IsNaN(number) then exit('{"float":"nan"}');
    if System.Double.IsPositiveInfinity(number) then exit('{"float":"inf"}');
    if System.Double.IsNegativeInfinity(number) then exit('{"float":"-inf"}');
    exit(number.ToString('R', CultureInfo.InvariantCulture));
  end;
  if value is IDictionary then
  begin
    var parts := new List<string>;
    var entries := IDictionary(value).GetEnumerator();
    while entries.MoveNext() do
      parts.Add(Quote(System.Convert.ToString(entries.Key)) + ':' + Encode(entries.Value));
    exit('{' + string.Join(',', parts) + '}');
  end;
  if value.GetType().IsGenericType and
     value.GetType().GetGenericTypeDefinition().Name.StartsWith('dict`') then
  begin
    var parts := new List<string>;
    foreach var item in IEnumerable(value) do
    begin
      var pairType := item.GetType();
      var key := pairType.GetProperty('Key').GetValue(item, nil);
      var entryValue := pairType.GetProperty('Value').GetValue(item, nil);
      parts.Add(Quote(System.Convert.ToString(key)) + ':' + Encode(entryValue));
    end;
    exit('{' + string.Join(',', parts) + '}');
  end;
  if value.GetType().IsGenericType and
     (value.GetType().FullName.StartsWith('System.Tuple`') or
      value.GetType().FullName.StartsWith('System.ValueTuple`')) then
  begin
    var parts := new List<string>;
    for var i := 1 to value.GetType().GetGenericArguments().Length do
    begin
      var fieldName := 'Item' + i.ToString();
      var propertyInfo := value.GetType().GetProperty(fieldName);
      var element := if propertyInfo <> nil then propertyInfo.GetValue(value, nil)
        else value.GetType().GetField(fieldName).GetValue(value);
      parts.Add(Encode(element));
    end;
    exit('[' + string.Join(',', parts) + ']');
  end;
  if value is IEnumerable then
  begin
    var parts := new List<string>;
    foreach var item in IEnumerable(value) do parts.Add(Encode(item));
    exit('[' + string.Join(',', parts) + ']');
  end;
  raise new System.ArgumentException('Matplotlib argument type is not supported: ' + value.GetType().FullName);
end;

function Options(params pairs: array of object): Dictionary<string, object>;
begin
  Result := new Dictionary<string, object>;
  for var i := 0 to pairs.Length div 2 - 1 do
  begin
    var value := pairs[2 * i + 1];
    if value = nil then continue;
    if value is real and System.Double.IsNaN(real(value)) then continue;
    Result.Add(System.Convert.ToString(pairs[2 * i]), value);
  end;
end;

function BridgeFile(): string;
begin
  Result := Environment.GetEnvironmentVariable('PABC_PYTHON_BRIDGE');
  if (Result <> nil) and System.IO.File.Exists(Result) then exit;
  var root := Environment.GetEnvironmentVariable('PABCNET_ROOT');
  if root <> nil then
  begin
    Result := System.IO.Path.Combine(root, 'bin', 'Lib', 'SPython', 'matplotlib_bridge.py');
    if System.IO.File.Exists(Result) then exit;
  end;
  Result := System.IO.Path.Combine(AppDomain.CurrentDomain.BaseDirectory, 'matplotlib_bridge.py');
  if System.IO.File.Exists(Result) then exit;
  raise new FileNotFoundException('Matplotlib bridge not found. Set PABC_PYTHON_BRIDGE.');
end;

procedure EnsureWorker();
begin
  if (worker <> nil) and not worker.HasExited then exit;
  var python := Environment.GetEnvironmentVariable('PABC_PYTHON_EXE');
  if string.IsNullOrEmpty(python) then python := 'python';
  var info := new ProcessStartInfo(python, '"' + BridgeFile() + '"');
  info.UseShellExecute := false;
  info.CreateNoWindow := true;
  info.RedirectStandardInput := true;
  info.RedirectStandardOutput := true;
  info.StandardOutputEncoding := Encoding.UTF8;
  info.WorkingDirectory := Environment.CurrentDirectory;
  worker := Process.Start(info);
  var ready := worker.StandardOutput.ReadLine();
  if ready = 'R' then exit;
  if (ready <> nil) and ready.StartsWith('E' + #9) then
    raise new System.Exception(Encoding.UTF8.GetString(System.Convert.FromBase64String(ready.Substring(2))));
  raise new System.Exception('Matplotlib bridge did not start. Check Python and matplotlib.');
end;

function Send(op, target, name: string; args: array of object;
  kwargs: Dictionary<string, object>; index: integer := 0): object;
begin
  System.Threading.Monitor.Enter(gate);
  try
    EnsureWorker();
    var payload := new StringBuilder;
    payload.Append('{"op":').Append(Quote(op)).Append(',"target":').Append(target);
    if name <> nil then payload.Append(',"name":').Append(Quote(name));
    if op = 'item' then payload.Append(',"index":').Append(index);
    if op = 'call' then
    begin
      payload.Append(',"args":[');
      for var i := 0 to args.Length - 1 do
      begin
        if i > 0 then payload.Append(',');
        payload.Append(Encode(args[i]));
      end;
      payload.Append('],"kwargs":').Append(Encode(kwargs));
    end;
    payload.Append('}');
    worker.StandardInput.WriteLine(payload.ToString());
    worker.StandardInput.Flush();
    var answer := worker.StandardOutput.ReadLine();
    if answer = nil then raise new System.Exception('Matplotlib bridge terminated unexpectedly.');
    var pieces := answer.Split(#9);
    case pieces[0] of
      'N': Result := nil;
      'B': Result := pieces[1] = '1';
      'I': Result := System.Int64.Parse(pieces[1], CultureInfo.InvariantCulture);
      'D': Result := System.Double.Parse(pieces[1], CultureInfo.InvariantCulture);
      'S': Result := Encoding.UTF8.GetString(System.Convert.FromBase64String(pieces[1]));
      'H': Result := new PlotObject(integer.Parse(pieces[1], CultureInfo.InvariantCulture));
      'E': raise new System.Exception(Encoding.UTF8.GetString(System.Convert.FromBase64String(pieces[1])));
      else raise new System.Exception('Invalid Matplotlib bridge response: ' + answer);
    end;
  finally
    System.Threading.Monitor.Exit(gate);
  end;
end;

function Call(target, name: string; args: array of object;
  kwargs: Dictionary<string, object>): object := Send('call', target, name, args, kwargs);

function PythonCall(moduleName, name: string; args: array of object;
  kwargs: Dictionary<string, object>): object := Call(Quote(moduleName), name, args, kwargs);

function PythonMethod(target: PlotObject; name: string; args: array of object;
  kwargs: Dictionary<string, object>): object :=
  Call(target.Handle.ToString(), name, args, kwargs);

function PythonAttribute(target: PlotObject; name: string): object :=
  Send('attr', target.Handle.ToString(), name, new object[0], nil);

function PythonIndex(target: PlotObject; index: integer): object :=
  Send('item', target.Handle.ToString(), nil, new object[0], nil, index);

function PlotCall(name: string; args: array of object;
  kwargs: Dictionary<string, object>): PlotObject :=
  Call('"pyplot"', name, args, kwargs) as PlotObject;

function ObjectCall(id: integer; name: string; args: array of object;
  kwargs: Dictionary<string, object>): PlotObject :=
  Call(id.ToString(), name, args, kwargs) as PlotObject;

constructor PlotObject.Create(value: integer);
begin id := value; end;

function PlotObject.GetLength(): integer :=
  System.Convert.ToInt32(Send('len', id.ToString(), nil, new object[0], nil));

function PlotObject.GetItem(index: integer): PlotObject :=
  Send('item', id.ToString(), nil, new object[0], nil, index) as PlotObject;

function PlotObject.Items(): sequence of PlotObject;
begin
  for var i := 0 to Length - 1 do yield ByIndex[i];
end;

function PlotObject.GetEnumerator(): IEnumerator<PlotObject> := Items().GetEnumerator();
function PlotObject.ToString(): string := '<matplotlib object ' + id.ToString() + '>';

function PlotObject.plot(x, y: object; fmt: string; color: string; &label: string;
  linewidth: real; linestyle: string; marker: string; alpha: real): PlotObject :=
  ObjectCall(id, 'plot', if fmt = nil then new object[](x, y) else new object[](x, y, fmt),
    Options('color', color, 'label', &label, 'linewidth', linewidth,
      'linestyle', linestyle, 'marker', marker, 'alpha', alpha));

function PlotObject.plot(y: object): PlotObject := ObjectCall(id, 'plot', new object[](y), Options());

function PlotObject.scatter(x, y: object; s, c: object; marker, cmap, &label: string;
  alpha: real): PlotObject := ObjectCall(id, 'scatter', new object[](x, y),
  Options('s', s, 'c', c, 'marker', marker, 'cmap', cmap, 'label', &label, 'alpha', alpha));

function PlotObject.bar(x, height: object; width: real; color, &label: string): PlotObject :=
  ObjectCall(id, 'bar', new object[](x, height), Options('width', width, 'color', color, 'label', &label));

function PlotObject.hist(x: object; bins: integer; color, &label: string;
  density: boolean): PlotObject := ObjectCall(id, 'hist', new object[](x),
  Options('bins', if bins < 0 then nil else object(bins), 'color', color,
    'label', &label, 'density', density));

function PlotObject.imshow(x: object; cmap, interpolation: string): PlotObject :=
  ObjectCall(id, 'imshow', new object[](x), Options('cmap', cmap, 'interpolation', interpolation));
function PlotObject.set_title(text: string): PlotObject := ObjectCall(id, 'set_title', new object[](text), Options());
function PlotObject.set_xlabel(text: string): PlotObject := ObjectCall(id, 'set_xlabel', new object[](text), Options());
function PlotObject.set_ylabel(text: string): PlotObject := ObjectCall(id, 'set_ylabel', new object[](text), Options());
function PlotObject.legend(): PlotObject := ObjectCall(id, 'legend', new object[0], Options());
function PlotObject.grid(visible: boolean): PlotObject := ObjectCall(id, 'grid', new object[](visible), Options());
function PlotObject.set_xlim(left, right: real): PlotObject := ObjectCall(id, 'set_xlim', new object[](left, right), Options());
function PlotObject.set_ylim(bottom, top: real): PlotObject := ObjectCall(id, 'set_ylim', new object[](bottom, top), Options());
function PlotObject.get_xlim(): PlotObject := ObjectCall(id, 'get_xlim', new object[0], Options());
function PlotObject.get_ylim(): PlotObject := ObjectCall(id, 'get_ylim', new object[0], Options());
function PlotObject.tight_layout(): PlotObject := ObjectCall(id, 'tight_layout', new object[0], Options());
function PlotObject.savefig(fname: string; dpi: real; bbox_inches: string;
  transparent: boolean): PlotObject := ObjectCall(id, 'savefig', new object[](fname),
  Options('dpi', dpi, 'bbox_inches', bbox_inches, 'transparent', transparent));
function PlotObject.gca(): PlotObject := ObjectCall(id, 'gca', new object[0], Options());

function plot(x, y: object; fmt, color, &label: string; linewidth: real;
  linestyle, marker: string; alpha: real): PlotObject :=
  PlotCall('plot', if fmt = nil then new object[](x, y) else new object[](x, y, fmt),
    Options('color', color, 'label', &label, 'linewidth', linewidth,
      'linestyle', linestyle, 'marker', marker, 'alpha', alpha));
function plot(y: object): PlotObject := PlotCall('plot', new object[](y), Options());
function scatter(x, y: object; s, c: object; marker, cmap, &label: string;
  alpha: real): PlotObject := PlotCall('scatter', new object[](x, y),
  Options('s', s, 'c', c, 'marker', marker, 'cmap', cmap, 'label', &label, 'alpha', alpha));
function bar(x, height: object; width: real; color, &label: string): PlotObject :=
  PlotCall('bar', new object[](x, height), Options('width', width, 'color', color, 'label', &label));
function hist(x: object; bins: integer; color, &label: string; density: boolean): PlotObject :=
  PlotCall('hist', new object[](x), Options('bins', if bins < 0 then nil else object(bins),
    'color', color, 'label', &label, 'density', density));
function imshow(x: object; cmap, interpolation: string): PlotObject :=
  PlotCall('imshow', new object[](x), Options('cmap', cmap, 'interpolation', interpolation));
function figure(num: integer; figsize: object; dpi: real): PlotObject :=
  PlotCall('figure', if num < 0 then new object[0] else new object[](num), Options('figsize', figsize, 'dpi', dpi));
function subplots(nrows, ncols: integer; figsize: object; sharex, sharey: boolean): sequence of PlotObject;
begin
  var items := PlotCall('subplots', new object[](nrows, ncols),
    Options('figsize', figsize, 'sharex', sharex, 'sharey', sharey));
  var values := new List<PlotObject>;
  for var i := 0 to items.Length - 1 do values.Add(items[i]);
  Result := values;
end;
function subplot(nrows, ncols, index: integer): PlotObject :=
  PlotCall('subplot', new object[](nrows, ncols, index), Options());
function gca(): PlotObject := PlotCall('gca', new object[0], Options());
function gcf(): PlotObject := PlotCall('gcf', new object[0], Options());
function title(text: string): PlotObject := PlotCall('title', new object[](text), Options());
function xlabel(text: string): PlotObject := PlotCall('xlabel', new object[](text), Options());
function ylabel(text: string): PlotObject := PlotCall('ylabel', new object[](text), Options());
function legend(): PlotObject := PlotCall('legend', new object[0], Options());
function grid(visible: boolean): PlotObject := PlotCall('grid', new object[](visible), Options());
function xlim(left, right: real): PlotObject := PlotCall('xlim', new object[](left, right), Options());
function ylim(bottom, top: real): PlotObject := PlotCall('ylim', new object[](bottom, top), Options());
function tight_layout(): PlotObject := PlotCall('tight_layout', new object[0], Options());
procedure savefig(fname: string; dpi: real; bbox_inches: string; transparent: boolean);
begin
  PlotCall('savefig', new object[](fname), Options('dpi', dpi, 'bbox_inches', bbox_inches,
    'transparent', transparent));
end;
procedure show(); begin PlotCall('show', new object[0], Options()); end;
procedure close(); begin PlotCall('close', new object[0], Options()); end;
procedure close(fig: PlotObject); begin PlotCall('close', new object[](fig), Options()); end;
procedure clf(); begin PlotCall('clf', new object[0], Options()); end;
procedure cla(); begin PlotCall('cla', new object[0], Options()); end;

end.
