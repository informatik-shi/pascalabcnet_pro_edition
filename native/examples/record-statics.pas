type Ticket = record
  static nextNumber: integer := 100;
  static constructor := Inc(nextNumber);
  number: integer;
  constructor(n: integer);
  begin
    number := n;
  end;
  static function Issue: Ticket;
  begin
    Result := new Ticket(nextNumber);
    Inc(nextNumber);
  end;
end;

begin
  var blank := default(Ticket);
  var first := Ticket.Issue;
  var second := Ticket.Issue;
  Writeln(blank.number, ' ', first.number, ' ', second.number);
end.
