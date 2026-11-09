type Greeting=interface function Text:string;end;
Sample=class(Greeting,IEnumerable<integer>)
public
 function Text:string:='class';
private
 function Greeting.Text:string:='interface';
 function IEnumerable<integer>.GetEnumerator:IEnumerator<integer>:=Seq(4,5).GetEnumerator;
end;
begin
 var sample:=new Sample;
 var greeting:Greeting:=sample;
 Writeln(sample.Text,' ',greeting.Text);
 var numbers:IEnumerable<integer>:=sample;
 Writeln(numbers.ToArray);
end.
