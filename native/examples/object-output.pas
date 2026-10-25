uses Arrays;
type Student=record
  name:string;
  marks:array of integer;
end;
begin
  var students:=new Student[2];
  students[0].name:='Анна'; students[0].marks:=Arr(5,4);
  students[1].name:='Борис'; students[1].marks:=Arr(4,5);
  Writeln('Записи: ',students);
  WritelnArray(students,' / ');
  Writeln('Матрица: ',new integer[2,2]);
end.
