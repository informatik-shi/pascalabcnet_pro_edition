uses Arrays;
type
  FillInts=procedure(a:array of integer; value:integer);
  FillReals=procedure(a:array[,] of real; value:real);
  PrintInts=procedure(a:array of integer; delimiter:string);
  PrintStrings=procedure(a:array of string; delimiter:string);
  SortInts=procedure(a:array of integer);
  SortStrings=procedure(a:array of string);
  MatrixReals=procedure(a:array[,] of real; width:integer);
  MatrixInts=procedure(a:array[,] of integer; width:integer);
begin
  var op,n,nilFlag,form,width:integer;
  Read(op,n,nilFlag,form,width);
  var a:=new integer[n];
  var words:=new string[n];
  var m:=new real[n,2];
  var integers:=new integer[n,2];
  var values:=new string[6]('b','a','c','A',nil,'x');
  for var i:=0 to n-1 do begin
    a[i]:=n-i-2;
    words[i]:=values[i mod 6];
    for var j:=0 to 1 do begin
      m[i,j]:=1.5+i+j/4.0;
      integers[i,j]:=i*10+j;
    end;
  end;
  if nilFlag=1 then begin a:=nil;words:=nil;m:=nil;integers:=nil;end;
  var delimiter:string:='|';
  if width=5 then delimiter:=nil;
  if op=0 then begin
    if form=0 then FillByValue&<integer>(a,7)
    else if form=1 then Arrays.FillByValue&<integer>(a,7)
    else begin var f:FillInts:=Arrays.FillByValue&<integer>;f(a,7);end;
    Arrays.WriteArray&<integer>(a,'|');
  end
  else if op=1 then begin
    if form=0 then FillByValue&<real>(m,1.5)
    else if form=1 then Arrays.FillByValue&<real>(m,1.5)
    else begin var f:FillReals:=FillByValue&<real>;f(m,1.5);end;
    WriteMatrix&<real>(m);
  end
  else if op=2 then begin
    if form=0 then WriteArray&<integer>(a,delimiter)
    else if form=1 then Arrays.WriteArray&<integer>(a,delimiter)
    else begin var f:=Arrays.WriteArray&<integer>;f(a,delimiter);end;
  end
  else if op=3 then begin
    if form=0 then WritelnArray&<string>(words,delimiter)
    else if form=1 then Arrays.WritelnArray&<string>(words,delimiter)
    else begin var f:PrintStrings:=WritelnArray&<string>;f(words,delimiter);end;
  end
  else if op=4 then begin
    if form=0 then Sort&<string>(words)
    else if form=1 then Arrays.Sort&<string>(words)
    else begin var f:SortStrings:=Arrays.Sort&<string>;f(words);end;
    WritelnArray&<string>(words,'|');
  end
  else if op=5 then begin
    if form=0 then SortDescending&<integer>(a)
    else if form=1 then Arrays.SortDescending&<integer>(a)
    else begin var f:=Arrays.SortDescending&<integer>;f(a);end;
    WritelnArray&<integer>(a,'|');
  end
  else if op=6 then begin
    if form=0 then begin
      if width=0 then WriteMatrix&<real>(m)
      else WriteMatrix&<real>(m,width);
    end
    else if form=1 then Arrays.WriteMatrix&<real>(m,width)
    else begin var f:MatrixReals:=WriteMatrix&<real>;f(m,width);end;
  end
  else if op=7 then begin
    if form=0 then WriteMatrix&<integer>(integers,width)
    else if form=1 then Arrays.WriteMatrix&<integer>(integers,width)
    else begin var f:MatrixInts:=Arrays.WriteMatrix&<integer>;f(integers,width);end;
  end;
end.
