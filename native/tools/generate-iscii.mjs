// Development-only static data derivation from independent public-API outcomes.
import fs from 'node:fs';import path from 'node:path';import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..'),data=fs.readFileSync(path.join(root,'tests/fixtures/iscii-reference.bin'));let offset=0;
const bytes=n=>{if(offset+n>data.length)throw Error('Truncated ISCII reference');const value=data.subarray(offset,offset+n);offset+=n;return value;},number=()=>bytes(4).readUInt32LE();
const text=()=>{let n=0,s=0,b;do{b=bytes(1)[0];n|=(b&127)<<s;s+=7;if(s>28)throw Error('Invalid text length');}while(b&128);return bytes(n).toString('utf8');};
const runs=callback=>{const count=number();let position=0;for(let row=0;row<count;++row){const length=number(),exact=bytes(1)[0],value=bytes(bytes(1)[0]);if(!length||position+length>65536)throw Error('Invalid run');if(callback)for(let n=position;n<position+length;++n)callback(n,exact,value);position+=length;}if(position!==65536)throw Error('Incomplete BMP runs');};
if(bytes(8).toString()!=='ABCISC01'||number()!==10)throw Error('Invalid ISCII header');const codecs=[],mapping=Array(0x0d6f-0x0901+1).fill(0);
for(let index=0;index<10;++index){const cp=number(),runtime=text();if(cp!==57002+index||runtime!=='System.Text.ISCIIEncoding')throw Error('Unexpected ISCII codec');
 runs((n,exact,value)=>{
  if(!index&&exact&&n>=160){let script=2,start=0,end=value.length;if(value[0]===0xef){script=value[1]-0x40;start=2;if(value[end-2]!==0xef||value[end-1]!==0x42)throw Error('Missing default-font reset');end-=2;}if(n<0x0901||n>0x0d6f||end-start<1||end-start>2||script<2||script>11)throw Error('Unexpected Indic map');mapping[n-0x0901]=(script<<16)|(value[start]<<8)|(end-start===2?value[start+1]:0);}
  let expected;if(n<160)expected=Buffer.from([n]);else{const entry=mapping[n-0x0901]??0;if(!entry){if(exact)throw Error('Unrecorded exact mapping');expected=Buffer.from([63]);}else{const script=entry>>>16,content=[(entry>>>8)&255];if(entry&255)content.push(entry&255);expected=Buffer.from(script===index+2?content:[0xef,script+0x40,...content,0xef,index+0x42]);if(!exact)throw Error('Exact-map classification mismatch');}}
  if(!value.equals(expected))throw Error('ISCII canonical map mismatch '+cp+' '+n);
 });
 const singles=bytes(512),exact=bytes(32),modes=bytes(256),alternate=bytes(512);if([...modes].some(n=>n>4))throw Error('Unknown pending mode');
 for(let n=0;n<65536;++n){const flags=bytes(1)[0],count=flags&127;if(count>2)throw Error('Invalid decoded pair size');bytes(count*2);}
 const contexts=number();if(contexts!==20)throw Error('Unexpected encoder contexts');for(let n=0;n<contexts;++n){bytes(bytes(1)[0]*2);bytes(bytes(1)[0]);runs();}
 codecs.push({cp,singles,exact,modes,alternate});
}
if(offset!==data.length)throw Error('Trailing ISCII reference data');
const table=(name,type,values)=>`constexpr ${type} ${name}[]={\n`+Array.from({length:Math.ceil(values.length/12)},(_,n)=>'    '+values.slice(n*12,(n+1)*12).map(v=>'0x'+v.toString(16)).join(',')+',').join('\n')+'\n};\n';
let output='// Generated from public Framework ISCII reference outcomes.\n'+table('iscii_encode_map','std::uint32_t',mapping);
for(const c of codecs){const prefix='iscii_'+c.cp;for(const [suffix,type,values] of [['_base','char16_t',Array.from({length:256},(_,n)=>c.singles.readUInt16LE(n*2))],['_exact','std::uint8_t',[...c.exact]],['_mode','std::uint8_t',[...c.modes]],['_alternate','char16_t',Array.from({length:256},(_,n)=>c.alternate.readUInt16LE(n*2))]])output+=table(prefix+suffix,type,values);}
output+='constexpr IsciiCodec iscii_codecs[]={\n'+codecs.map(c=>`    {iscii_${c.cp}_base,iscii_${c.cp}_exact,iscii_${c.cp}_mode,iscii_${c.cp}_alternate},`).join('\n')+'\n};\n';
const target=path.join(root,'src/iscii_data.inc');if(process.argv.includes('--check')){if(fs.readFileSync(target,'utf8').replaceAll('\r\n','\n')!==output)throw Error('Stale ISCII static data');}else fs.writeFileSync(target,output);
console.log(JSON.stringify({codecs:10,indic_mappings:mapping.filter(Boolean).length,generated_bytes:Buffer.byteLength(output)}));
