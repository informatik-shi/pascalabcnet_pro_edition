unit pandas1;

// SPython facade for the bundled CPython pandas. Tables and their NumPy
// arrays share the same remote-object table as pyplot.
interface

uses System, System.Collections.Generic, pyplot1;

function DataFrame(data: object; index: object := nil; columns: object := nil;
  dtype: string := nil): PlotObject;
function Series(data: object; index: object := nil; dtype: string := nil;
  name: string := nil): PlotObject;
function read_csv(path: string; sep: string := ','; header: object := 'infer';
  encoding: string := nil): PlotObject;
function concat(items: object; axis: integer := 0; ignore_index: boolean := false): PlotObject;
function merge(left, right: PlotObject; &on: object := nil; how: string := 'inner'): PlotObject;
function to_datetime(value: object; format: string := nil): object;

implementation

function Kw(): Dictionary<string, object> := new Dictionary<string, object>;

function DataFrame(data, index, columns: object; dtype: string): PlotObject;
begin
  var options := Kw();
  if index <> nil then options.Add('index', index);
  if columns <> nil then options.Add('columns', columns);
  if dtype <> nil then options.Add('dtype', dtype);
  Result := PythonCall('pandas', 'DataFrame', new object[](data), options) as PlotObject;
end;
function Series(data, index: object; dtype, name: string): PlotObject;
begin
  var options := Kw();
  if index <> nil then options.Add('index', index);
  if dtype <> nil then options.Add('dtype', dtype);
  if name <> nil then options.Add('name', name);
  Result := PythonCall('pandas', 'Series', new object[](data), options) as PlotObject;
end;
function read_csv(path, sep: string; header: object; encoding: string): PlotObject;
begin
  var options := Kw();
  options.Add('sep', sep);
  options.Add('header', header);
  if encoding <> nil then options.Add('encoding', encoding);
  Result := PythonCall('pandas', 'read_csv', new object[](path), options) as PlotObject;
end;
function concat(items: object; axis: integer; ignore_index: boolean): PlotObject;
begin
  var options := Kw();
  options.Add('axis', axis);
  options.Add('ignore_index', ignore_index);
  Result := PythonCall('pandas', 'concat', new object[](items), options) as PlotObject;
end;
function merge(left, right: PlotObject; &on: object; how: string): PlotObject;
begin
  var options := Kw();
  if &on <> nil then options.Add('on', &on);
  options.Add('how', how);
  Result := PythonCall('pandas', 'merge', new object[](left, right), options) as PlotObject;
end;
function to_datetime(value: object; format: string): object;
begin
  var options := Kw();
  if format <> nil then options.Add('format', format);
  Result := PythonCall('pandas', 'to_datetime', new object[](value), options);
end;

end.
