uses School;

type Mapper = integer -> integer;

function DoubleIt(x: integer): integer;
begin
  Result := 2 * x;
end;

function TripleIt(x: integer): integer;
begin
  Result := 3 * x;
end;

function Small(x: integer): boolean;
begin
  Result := x < 3;
end;

function Add(a, b: integer): integer;
begin
  Result := a + b;
end;

function Both(a, b: boolean): boolean;
begin
  Result := a and b;
end;

begin
  var mapper: Mapper := DoubleIt;
  var saved := Seq(1, 2, 3).Select(mapper);
  mapper := TripleIt;
  Writeln('Сохранённая функция: ', saved.ToArray);
  var predicate: integer -> boolean := Small;
  Writeln('Отбор: ', Seq(1, 2, 3).Where(predicate).ToArray);
  var addPair: (integer, integer) -> integer := Add;
  Writeln('Суммы пар: ', Zip(Seq(1, 2), Seq(10, 20), addPair).ToArray);
  var truth: (boolean, boolean) -> boolean := Both;
  var table := TrueTable(truth);
  Writeln('Результат для True, True: ', table[3, 2]);
end.
