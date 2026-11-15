type IntMatrix = array[,] of integer;
type RealMatrix = array[,] of real;
type CharMatrix = array[,] of char;

begin
  var matrix := new integer[2,3]((1,2,3),(4,-5,6));
  var value: object := matrix;
  var alias := IntMatrix(value);
  alias[0,0] := 9;
  assert(matrix[0,0]=9);
  alias[0,0] := 1;
  Serialize('serialized-matrix.dat', matrix);
  var loaded := IntMatrix(Deserialize('serialized-matrix.dat'));
  Writeln(loaded[1,1], ' ', loaded.GetLength(0), ' ', loaded.GetLength(1));
  var f: file;
  Reset(f,'serialized-matrix.dat');
  Writeln(f.ReadBytes(80));
  Close(f);

  Rewrite(f,'mixed-matrix-stream.dat');
  f.Serialize(new real[0,3]);
  f.Serialize(new char[1,2]((#1071,'b')));
  Close(f);
  Reset(f,'mixed-matrix-stream.dat');
  var empty := RealMatrix(f.Deserialize);
  var chars := CharMatrix(f.Deserialize);
  Writeln(empty.GetLength(0), ' ', empty.GetLength(1));
  Writeln(Ord(chars[0,0]), ' ', Ord(chars[0,1]), ' ', Eof(f));
  Close(f);
end.
