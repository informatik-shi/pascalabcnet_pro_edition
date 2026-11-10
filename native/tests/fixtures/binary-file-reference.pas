type
  Shade=(Red,Green,Blue);
  Nested=record n:integer;c:char;end;
  PackedValue=record
    flag:boolean;b:byte;s:shortint;w:word;h:smallint;n:integer;u:longword;
    l:int64;v:uint64;x:single;y:real;c:char;color:Shade;inner:Nested;
  end;
begin
  var cp:=ReadInteger;
  var f:file of PackedValue;
  Rewrite(f,'native-packed-reference.tmp',Encoding.GetEncoding(cp));
  for var i:=0 to 8 do
  begin
    var p:PackedValue;
    p.flag:=i mod 2=1;p.b:=byte(i*31);p.s:=shortint(i-4);p.w:=word(65535-i*157);p.h:=smallint(-32768+i*971);
    p.n:=integer(-2147483648+int64(i)*1234567);p.u:=longword(uint64(4294967295)-uint64(i)*33331);
    p.l:=-9223372036854775807+int64(i)*123456789;p.v:=uint64(18446744073709551615)-uint64(i)*54321987;
    p.x:=single(i+0.25);p.y:=i+0.125;p.c:=Chr(i*997);p.color:=Shade(i mod 3);p.inner.n:=999-i;p.inner.c:=Chr(i*991+63);
    Write(f,p);
  end;
  Writeln(FileSize(f),' ',FilePos(f));
  Reset(f);
  for var i:=0 to 8 do
  begin
    var q:PackedValue;
    Read(f,q);
    assert(q.flag=(i mod 2=1));assert(q.b=byte(i*31));assert(q.s=shortint(i-4));assert(q.w=word(65535-i*157));assert(q.h=smallint(-32768+i*971));
    assert(q.n=integer(-2147483648+int64(i)*1234567));assert(q.u=longword(uint64(4294967295)-uint64(i)*33331));
    assert(q.l=-9223372036854775807+int64(i)*123456789);assert(q.v=uint64(18446744073709551615)-uint64(i)*54321987);
    assert(q.x=single(i+0.25));assert(q.y=i+0.125);assert(Ord(q.color)=i mod 3);assert(q.inner.n=999-i);
    Writeln(Ord(q.c),' ',Ord(q.inner.c));
  end;
  Writeln(Eof(f));
  Seek(f,10);Writeln(FilePos(f),' ',Eof(f));
  Close(f);
end.
