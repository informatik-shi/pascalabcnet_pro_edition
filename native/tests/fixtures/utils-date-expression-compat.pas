program UtilsDateExpressionSource;
uses Utils, System;
function Parsed(s:string): DateTime := DateTime(Utils.Eval(s));
procedure Show(s:string);
begin
  var d:=Parsed(s);
  Writeln(Utils.Eval(s).GetType.FullName,'|',d.ToString('O'),'|',d.Kind,'|',Utils.EvalStr(s))
end;
begin
  Show('#2024-02-29#');
  Show('#2/29/2024#');
  Show('#2/29/24#');
  Show('#2024-02-29 23:59:59.1234567#');
  Show('#2024-02-29T23:59:59.1234567#');
  Show('#February 29, 2024#');
  Show('#29 February 2024#');
  Show('#Feb. 29, 2024#');
  Show('#2024-02-29 1:02 PM#');
  Show('CONVERT(''2024-02-29'',System.DateTime)');
  Show('CONVERT(''February 29, 2024'',''System.DateTime'')');
  Writeln(Parsed('#12:30#').Date=DateTime.Today,' ',Parsed('#12:30#').ToString('HH:mm'));
  Writeln(Parsed('#02/28#').Year=DateTime.Today.Year,' ',Parsed('#02/28#').ToString('MM-dd'));
  var utc:=Parsed('#2024-02-29T00:00:00Z#');
  Writeln(utc.ToUniversalTime.ToString('O'),' ',utc.Kind);
  Writeln(EvalBool('#2024-02-29#=''2/29/2024'''),' ',
          EvalBool('#2024-02-29#<#2024-03-01#'),' ',
          EvalBool('#2024-02-29# IN (#2023-01-01#,''2/29/2024'')'));
  Writeln(EvalStr('CONVERT(#2024-02-29#,System.String)'));
  Writeln(EvalStr('#2024-02-29#+''!'''));
  Writeln(EvalBool('CONVERT(NULL,System.DateTime) IS NULL'));
  var ef:function(s:string):string:=Utils.EvalStr;
  Writeln(ef('#2024-02-29#'));
end.
