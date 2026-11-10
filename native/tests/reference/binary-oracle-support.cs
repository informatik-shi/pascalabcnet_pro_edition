// Public API only. The modern runtime is an additional development comparison.
using System;using System.Text;using System.IO;using System.Collections.Generic;
static class BinaryOracle{
 static bool modern;
 public static void Setup(bool value){modern=value;if(!modern)return;var provider=Type.GetType("System.Text.CodePagesEncodingProvider, System.Text.Encoding.CodePages");typeof(Encoding).GetMethod("RegisterProvider").Invoke(null,new[]{provider.GetProperty("Instance").GetValue(null,null)});}
 public static Encoding Get(int page){return modern&&page==65000?new UTF7Encoding(false):Encoding.GetEncoding(page);}
 public static IEnumerable<int> Pages(string path){foreach(var line in File.ReadAllLines(path))if(line.Length>0&&line[0]!='#')yield return int.Parse(line.Split('\t')[0],System.Globalization.CultureInfo.InvariantCulture);}
}
