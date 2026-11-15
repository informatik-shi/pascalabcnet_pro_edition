program OberonCallable;
uses Oberon00System;
type
  TLabel = class
  public
    function ToString: string; override;
  end;
function TLabel.ToString: string := 'label';
begin
  var show := Oberon00System.Print;
  var emit: procedure(x:object) := Oberon00System.Println;
  var emitPair: procedure(x,y:integer) := Oberon00System.Println;
  var line: procedure := Oberon00System.Println;
  var showBare: procedure(x:object) := Print;
  var pairBare: procedure(x,y:integer) := Println;
  show('[');
  emit(new TLabel);
  emitPair(12,-8);
  line;
  showBare('!');
  pairBare(3,4);
end.
