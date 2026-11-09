type IBox=interface
 property Value:integer read write;
 property Item[i:integer]:integer read write;default;
 property IsReadOnly:boolean read;
 property Fn:integer->integer read;
end;
Box=class(IBox)
private
 data:array of integer:=Arr(2,4);
 procedure SetValue(n:integer);begin data[0]:=n;end;
 procedure SetItem(i,n:integer);begin data[i]:=n;end;
 property IBox.Value:integer read data[0] write data[0]:=value;
 property IBox.Item[i:integer]:integer read data[i] write data[i]:=value;
 property IBox.IsReadOnly:boolean read boolean(False);
 property IBox.Fn:integer->integer read (n->data[0]+n);
public
 property Value:integer read 99;
end;
begin
 var c:=new Box;var box:IBox:=c;
 box.Value:=7;box[1]+=3;
 Writeln(c.Value,' ',box.Value,' ',box[1],' ',box.IsReadOnly);
 var f:=box.Fn;box[0]:=9;c:=nil;box:=nil;
 Writeln(f(3));
end.
