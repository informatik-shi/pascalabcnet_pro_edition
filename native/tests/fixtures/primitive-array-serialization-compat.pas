type IntArray = array of integer;
type CharArray = array of char;
type ByteArray = array of byte;

begin
  var numbers := Arr(1,-2,3);
  var value: object := numbers;
  assert(value=object(numbers));
  var alias := IntArray(value);
  alias[1] := 4;
  assert(numbers[1]=4);
  numbers[1] := -2;
  Serialize('serialized-array.dat', numbers);
  var from_file := IntArray(Deserialize('serialized-array.dat'));
  Writeln(from_file);

  var f: file;
  Reset(f,'serialized-array.dat');
  Writeln(f.ReadBytes(64));
  Close(f);
  Erase(f);

  Rewrite(f,'mixed-serialized-array.dat');
  f.Serialize(44);
  f.Serialize(Arr(byte(7),byte(255),byte(0)));
  f.Serialize(Arr('a',#55356,#57101));
  f.Serialize(new integer[0]);
  Close(f);
  Reset(f,'mixed-serialized-array.dat');
  var first := integer(f.Deserialize);
  var bytes := ByteArray(f.Deserialize);
  var chars := CharArray(f.Deserialize);
  var empty := IntArray(f.Deserialize);
  Writeln(first,' ',bytes);
  Writeln(Ord(chars[0]),' ',Ord(chars[1]),' ',Ord(chars[2]));
  Writeln(empty.Length,' ',Eof(f));
  Close(f);
  Erase(f);
end.
