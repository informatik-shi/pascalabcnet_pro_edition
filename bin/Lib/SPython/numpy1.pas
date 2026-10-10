unit numpy1;

// SPython facade for the bundled CPython NumPy. Arrays remain in the shared
// Python process and are passed to matplotlib and pandas by reference.
interface

uses System, System.Collections.Generic, pyplot1;

const pi = 3.14159265358979323846;
const e = 2.71828182845904523536;

function &array(data: object; dtype: string := nil): PlotObject;
function asarray(data: object; dtype: string := nil): PlotObject;
function arange(start: object; stop: object := nil; step: object := nil;
  dtype: string := nil): PlotObject;
function linspace(start, stop: real; num: integer := 50;
  endpoint: boolean := true; dtype: string := nil): PlotObject;
function zeros(shape: object; dtype: string := nil): PlotObject;
function ones(shape: object; dtype: string := nil): PlotObject;
function full(shape, fill_value: object; dtype: string := nil): PlotObject;
function eye(n: integer; m: integer := -1; dtype: string := nil): PlotObject;
function reshape(a: object; newshape: object): PlotObject;
function concatenate(arrays: object; axis: integer := 0): PlotObject;
function stack(arrays: object; axis: integer := 0): PlotObject;
function sin(x: object): PlotObject;
function cos(x: object): PlotObject;
function sqrt(x: object): PlotObject;
function exp(x: object): PlotObject;
function log(x: object): PlotObject;
function abs(x: object): PlotObject;
function sum(a: object; axis: object := nil): object;
function mean(a: object; axis: object := nil): object;
function min(a: object; axis: object := nil): object;
function max(a: object; axis: object := nil): object;
function std(a: object; axis: object := nil): object;

implementation

function Kw(name: string; value: object): Dictionary<string, object>;
begin
  Result := new Dictionary<string, object>;
  if value <> nil then Result.Add(name, value);
end;

function &array(data: object; dtype: string): PlotObject :=
  PythonCall('numpy', 'array', new object[](data), Kw('dtype', dtype)) as PlotObject;
function asarray(data: object; dtype: string): PlotObject :=
  PythonCall('numpy', 'asarray', new object[](data), Kw('dtype', dtype)) as PlotObject;
function arange(start, stop, step: object; dtype: string): PlotObject;
begin
  var args := new List<object>;
  args.Add(start);
  if stop <> nil then args.Add(stop);
  if step <> nil then args.Add(step);
  Result := PythonCall('numpy', 'arange', args.ToArray(), Kw('dtype', dtype)) as PlotObject;
end;
function linspace(start, stop: real; num: integer; endpoint: boolean; dtype: string): PlotObject;
begin
  var kwargs := Kw('dtype', dtype);
  kwargs.Add('endpoint', endpoint);
  Result := PythonCall('numpy', 'linspace', new object[](start, stop, num), kwargs) as PlotObject;
end;
function zeros(shape: object; dtype: string): PlotObject :=
  PythonCall('numpy', 'zeros', new object[](shape), Kw('dtype', dtype)) as PlotObject;
function ones(shape: object; dtype: string): PlotObject :=
  PythonCall('numpy', 'ones', new object[](shape), Kw('dtype', dtype)) as PlotObject;
function full(shape, fill_value: object; dtype: string): PlotObject :=
  PythonCall('numpy', 'full', new object[](shape, fill_value), Kw('dtype', dtype)) as PlotObject;
function eye(n, m: integer; dtype: string): PlotObject :=
  PythonCall('numpy', 'eye', if m < 0 then new object[](n) else new object[](n, m),
    Kw('dtype', dtype)) as PlotObject;
function reshape(a, newshape: object): PlotObject :=
  PythonCall('numpy', 'reshape', new object[](a, newshape), Kw('', nil)) as PlotObject;
function concatenate(arrays: object; axis: integer): PlotObject :=
  PythonCall('numpy', 'concatenate', new object[](arrays), Kw('axis', axis)) as PlotObject;
function stack(arrays: object; axis: integer): PlotObject :=
  PythonCall('numpy', 'stack', new object[](arrays), Kw('axis', axis)) as PlotObject;
function sin(x: object): PlotObject := PythonCall('numpy', 'sin', new object[](x), Kw('', nil)) as PlotObject;
function cos(x: object): PlotObject := PythonCall('numpy', 'cos', new object[](x), Kw('', nil)) as PlotObject;
function sqrt(x: object): PlotObject := PythonCall('numpy', 'sqrt', new object[](x), Kw('', nil)) as PlotObject;
function exp(x: object): PlotObject := PythonCall('numpy', 'exp', new object[](x), Kw('', nil)) as PlotObject;
function log(x: object): PlotObject := PythonCall('numpy', 'log', new object[](x), Kw('', nil)) as PlotObject;
function abs(x: object): PlotObject := PythonCall('numpy', 'abs', new object[](x), Kw('', nil)) as PlotObject;
function sum(a: object; axis: object): object :=
  PythonCall('numpy', 'sum', new object[](a), Kw('axis', axis));
function mean(a: object; axis: object): object :=
  PythonCall('numpy', 'mean', new object[](a), Kw('axis', axis));
function min(a: object; axis: object): object :=
  PythonCall('numpy', 'min', new object[](a), Kw('axis', axis));
function max(a: object; axis: object): object :=
  PythonCall('numpy', 'max', new object[](a), Kw('axis', axis));
function std(a: object; axis: object): object :=
  PythonCall('numpy', 'std', new object[](a), Kw('axis', axis));

end.
