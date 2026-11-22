program CountriesLoaderValuesOracle;
uses Countries, System;
type CountryArray=array of Country;
function ЗаполнитьМассивСтран:CountryArray;
begin
  Result:=Arr(new Country('Local','City',1,'Continent'));
end;
function ReadCount(loader:()->CountryArray):integer := loader().Length;
begin
  var local:()->CountryArray:=ЗаполнитьМассивСтран;
  var first:()->CountryArray:=Countries.ЗаполнитьМассивСтран;
  var second:System.Func<CountryArray>:=Countries.ЗаполнитьМассивСтран;
  Writeln(local().Length,'|',first().Length,'|',second().Length);
  var back:()->CountryArray:=second;
  var firstAgain:()->CountryArray:=Countries.ЗаполнитьМассивСтран;
  var secondAgain:System.Func<CountryArray>:=Countries.ЗаполнитьМассивСтран;
  var alias:=first;
  Writeln(first=alias);
  Writeln(first=firstAgain,'|',second=secondAgain,'|',first=back);
  Writeln(ReadCount(Countries.ЗаполнитьМассивСтран));
  var rows:=Countries.ЗаполнитьМассивСтран;
  Writeln(rows.Length);
end.
