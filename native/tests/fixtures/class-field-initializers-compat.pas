type
  Notebook = class
  private
    title: string := 'Черновик';
    lines: List<string> := new List<string>;
  public
    constructor Create(name: string);
    procedure Add(value: string);
    function Summary: string;
  end;

constructor Notebook.Create(name: string);
begin
  title := name;
end;

procedure Notebook.Add(value: string);
begin
  lines.Add(value);
end;

function Notebook.Summary: string;
begin
  Result := title + ': ' + lines.JoinToString(', ');
end;

begin
  var first := new Notebook('Первый');
  var second := new Notebook('Второй');
  first.Add('заметка');
  Writeln(first.Summary);
  Writeln(second.Summary);
end.
