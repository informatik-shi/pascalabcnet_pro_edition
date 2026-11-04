type Day=(Mon,Tue,Wed);
begin
  Writeln('Границы: ',InRange(5,1,9),' / ',InRange(5,9,1));
  Writeln('Порядок границ: ',Between(5,9,1),' / ',Between(1,1,9,False));
  Writeln('Ограничение: ',Clamp(-2,0,9),' ',Clamp(5,0,9),' ',Clamp(20,0,9));
  Writeln('Методы: ',20.ClampTop(9),' ',(-2).ClampBottom(0));
  Writeln('Перечисление: ',Clamp(Wed,Mon,Tue),' / ',Between(Tue,Wed,Mon,False));
  var text:string:='é';
  Writeln('Строка: ',InRange(text,'a','z'),' / ',text.InRange('a','z'));
end.
