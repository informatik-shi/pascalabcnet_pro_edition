// Development-only static data generator. Never needed by the application.
import fs from 'node:fs';import path from 'node:path';import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..'),data=fs.readFileSync(path.join(root,'tests/fixtures/gb18030-reference.bin'));let offset=0;
const bytes=n=>{if(offset+n>data.length)throw Error('Truncated GB18030 reference');const value=data.subarray(offset,offset+n);offset+=n;return value;};
const number=()=>bytes(4).readUInt32LE(),text=()=>{let n=0,s=0,b;do{b=bytes(1)[0];n|=(b&127)<<s;s+=7;}while(b&128);return bytes(n).toString('utf8');};
if(bytes(8).toString()!=='ABCGB181'||number()!==54936||text()!=='System.Text.GB18030Encoding')throw Error('Unexpected GB18030 reference');
const encoded=bytes(65536*4);bytes(8192+512+32);const pairs=bytes(65536*5),two=[];
for(let lead=0x81;lead<=0xfe;++lead)for(let trail=0x40;trail<=0xfe;++trail){if(trail===0x7f)continue;const index=((lead<<8)|trail)*5;if(pairs[index+4]!==5)throw Error('Invalid exact two-byte entry');two.push(pairs.readUInt32LE(index));}
const count=number();if(count!==1587600)throw Error('Unexpected four-byte count');const four=bytes(count*4),bmp=[];
for(let n=0;n<count;++n){const value=four.readUInt32LE(n*4);if(n<=0x99fb){if(value>65535||value>=0xd800&&value<=0xdfff)throw Error('Invalid four-byte BMP entry');bmp.push(value);}else if(value!==(n>=0x2e248&&n<=0x12e247?n-0x2e248+0x10000:0xffffffff))throw Error('Unexpected supplementary/gap mapping');}
if(number()!==1048576)throw Error('Unexpected scalar count');bytes(1048576*4);
const cases=number();if(cases!==134400)throw Error('Unexpected streaming input count');for(let row=0;row<cases;++row){const length=bytes(1)[0];if(length<1||length>4)throw Error('Invalid stream input size');bytes(length);for(let cut=0;cut<=length;++cut){const size=bytes(1)[0]&127;if(size>16)throw Error('Invalid stream output size');bytes(size*2);}}
if(offset!==data.length)throw Error('Unexpected trailing reference data');
const hex=n=>'0x'+n.toString(16),table=(name,type,values)=>`constexpr ${type} ${name}[]={\n`+Array.from({length:Math.ceil(values.length/12)},(_,n)=>'    '+values.slice(n*12,(n+1)*12).map(hex).join(',')+',').join('\n')+'\n};\n';
const output='// Generated from independent Framework GB18030 reference.\n'+table('gb_encode_bmp','std::uint32_t',Array.from({length:65536},(_,n)=>encoded.readUInt32LE(n*4)))+table('gb_decode_two','char16_t',two)+table('gb_decode_four_bmp','char16_t',bmp);
const destination=path.join(root,'src/gb18030_data.inc');if(process.argv.includes('--check')){if(fs.readFileSync(destination,'utf8').replaceAll('\r\n','\n')!==output)throw Error('Stale GB18030 data');}else fs.writeFileSync(destination,output);
console.log(JSON.stringify({bmp_encodings:65536,two_byte_decodings:two.length,four_byte_bmp_decodings:bmp.length,generated_bytes:Buffer.byteLength(output)}));
