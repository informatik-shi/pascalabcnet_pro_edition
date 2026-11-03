type
  Person = class
    fullName: string;
    age: integer;
    constructor(value: string; years: integer := 18);
    begin
      fullName := value;
      age := years;
    end;
    property Name: string read fullName;
  end;
begin
  var p := new Person('Анна');
  Writeln('Конструктор: ', p.Name, ' ', p.age);
  var q := new Person('Иван', 25);
  Writeln('Параметры: ', q.Name, ' ', q.age);
end.
