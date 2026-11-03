The CP1251 reference binaries were produced independently with Windows PowerShell's system System.Text.Encoding.GetEncoding(1251), without invoking a Pascal compiler. They are test data and are not installed into the portable application.

cp1251-decode-reference.bin contains 256 UTF-16LE code units obtained by decoding byte values 0..255. cp1251-encode-reference.bin contains 65536 bytes: for each UTF-16 code unit 0..65535, a separate single-character array was passed to Encoding.GetBytes and its first byte recorded. Separate calls retain the source OrdWindows handling of isolated surrogates.

The published Windows Best Fit 1251 table was checked against both complete reference domains before generating the native data. Source: https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WindowsBestFit/bestfit1251.txt
