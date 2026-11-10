// Development-only table derivation from an independent public-API fixture.
import fs from 'node:fs';import path from 'node:path';import zlib from 'node:zlib';import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..'),archive=fs.readFileSync(path.join(root,'tests/fixtures/iso2022-reference.zip'));
let end=-1;for(let p=archive.length-22;p>=Math.max(0,archive.length-65557);--p)if(archive.readUInt32LE(p)===0x06054b50){end=p;break;}
if(end<0||archive.readUInt16LE(end+10)!==1)throw Error('Expected one reference ZIP member');const central=archive.readUInt32LE(end+16);if(archive.readUInt32LE(central)!==0x02014b50)throw Error('Invalid central directory');
const method=archive.readUInt16LE(central+10),compressed=archive.readUInt32LE(central+20),size=archive.readUInt32LE(central+24),local=archive.readUInt32LE(central+42),nameLength=archive.readUInt16LE(central+28);
if(archive.subarray(central+46,central+46+nameLength).toString()!=='iso2022-reference.bin'||size>256*1024*1024||archive.readUInt32LE(local)!==0x04034b50)throw Error('Invalid reference member');
const start=local+30+archive.readUInt16LE(local+26)+archive.readUInt16LE(local+28),payload=archive.subarray(start,start+compressed),data=method===8?zlib.inflateRawSync(payload):method===0?payload:null;if(!data||data.length!==size)throw Error('Invalid compressed reference');let offset=0;
const bytes=n=>{if(offset+n>data.length)throw Error('Truncated reference');const result=data.subarray(offset,offset+n);offset+=n;return result;},number=()=>bytes(4).readUInt32LE();
const text=()=>{let n=0,s=0,b;do{b=bytes(1)[0];n|=(b&127)<<s;s+=7;if(s>28)throw Error('Invalid length');}while(b&128);return bytes(n).toString('utf8');};
const result=()=>{const flags=bytes(1)[0],value=[];for(let n=0;n<(flags&127);++n)value.push(bytes(2).readUInt16LE());return{value,exact:!!(flags&128)};};
const runs=callback=>{const count=number();let pos=0;for(let k=0;k<count;++k){const length=number(),exact=bytes(1)[0],value=bytes(bytes(1)[0]);if(!length||pos+length>65536)throw Error('Invalid BMP run');if(callback)for(let n=pos;n<pos+length;++n)callback(n,value,exact);pos+=length;}if(pos!==65536)throw Error('Incomplete BMP runs');};
if(bytes(8).toString()!=='ABCISO01'||number()!==5)throw Error('Invalid ISO reference header');
const codecs=[];for(const cp of [50220,50221,50222,50225,52936]){if(number()!==cp||text()!=='System.Text.ISO2022Encoding')throw Error('Unexpected ISO codec');const encode=[],encodedExact=Array(8192).fill(0);
 runs((n,value,exact)=>{let code;if(value.length===1)code=value[0];else if(cp<50225&&value.subarray(0,3).equals(Buffer.from([27,36,66]))&&value.length===8&&value.subarray(5).equals(Buffer.from([27,40,66])))code=(value[3]<<8)|value[4];
 else if(cp===50221&&value.length===7&&value.subarray(0,3).equals(Buffer.from([27,40,73]))&&value.subarray(4).equals(Buffer.from([27,40,66])))code=0x1000|value[3];
 else if(cp===50222&&value.length===3&&value[0]===14&&value[2]===15)code=0x1000|value[1];
 else if(cp===50225&&value.length===8&&value.subarray(0,5).equals(Buffer.from([27,36,41,67,14]))&&value[7]===15)code=(value[5]<<8)|value[6];
 else if(cp===52936&&value.length===6&&value[0]===126&&value[1]===123&&value[4]===126&&value[5]===125)code=(value[2]<<8)|value[3];
 else if(cp===52936&&value.length===2&&value[0]===126&&value[1]===126)code=126;else throw Error('Unexpected canonical encoded symbol '+cp+' '+n+' '+value.toString('hex'));
 if(exact)encodedExact[n>>3]|=1<<(n&7);if(exact||code!==63)encode.push(n*65536+code);
 });
 const maps=[];for(let count=number();count>0;--count){const prefix=bytes(bytes(1)[0]),entries=[];for(let n=0;n<65536;++n)entries.push(result());maps.push({prefix,entries});}
 const raw=maps.at(-1).entries,base=[],baseExact=Array(32).fill(0),pair=[],pairExact=Array(8192).fill(0),replacement=cp<50225?0x30fb:63;
 for(let b=0;b<256;++b){let r;if(cp===52936){r=maps[0].entries[b];if(b===126)r={value:[0,126],exact:true};if(r.value.length!==2)throw Error('Unexpected HZ single derivation');base.push(r.value[1]);}else{r=raw[b];if(r.value.length!==1)throw Error('Unexpected raw single derivation');base.push(r.value[0]);}if(r.exact)baseExact[b>>3]|=1<<(b&7);}
 for(let n=0;n<65536;++n){const lead=n>>8;const allowed=cp<50225?![14,15,27].includes(lead):cp===50225?![9,10,14,15,27,32].includes(lead):lead>=32&&lead!==126;const r=raw[n];if(allowed&&r.value.length!==1)throw Error('Unexpected raw pair mapping');pair.push(allowed?r.value[0]:replacement);if(allowed&&r.exact)pairExact[n>>3]|=1<<(n&7);}
 const contexts=number();for(let n=0;n<contexts;++n){bytes(bytes(1)[0]*2);bytes(1);bytes(bytes(1)[0]);runs();}
 const states=number();for(let n=0;n<states;++n){bytes(bytes(1)[0]);result();for(let k=0;k<65536;++k)result();}
 codecs.push({cp,encode,encodedExact,base,baseExact,pair,pairExact,replacement,contexts,states});
}
if(offset!==data.length)throw Error('Trailing reference data');
const table=(name,type,values)=>`constexpr ${type} ${name}[]={\n`+Array.from({length:Math.ceil(values.length/12)},(_,n)=>'    '+values.slice(n*12,(n+1)*12).map(v=>'0x'+v.toString(16)).join(',')+',').join('\n')+'\n};\n';
let output='// Generated from independent ISO-2022/HZ public API reference.\n';for(const c of codecs){const p='iso_'+c.cp;output+=table(p+'_encode','std::uint32_t',c.encode)+table(p+'_encode_exact','std::uint8_t',c.encodedExact)+table(p+'_base','char16_t',c.base)+table(p+'_base_exact','std::uint8_t',c.baseExact)+table(p+'_pair','char16_t',c.pair)+table(p+'_pair_exact','std::uint8_t',c.pairExact);}
output+='constexpr IsoCodec iso_codecs[]={\n'+codecs.map(c=>`    {${c.cp},${c.replacement},iso_${c.cp}_encode,iso_${c.cp}_encode_exact,iso_${c.cp}_base,iso_${c.cp}_base_exact,iso_${c.cp}_pair,iso_${c.cp}_pair_exact},`).join('\n')+'\n};\n';
const target=path.join(root,'src/iso2022_data.inc');if(process.argv.includes('--check')){if(fs.readFileSync(target,'utf8').replaceAll('\r\n','\n')!==output)throw Error('Stale ISO static data');}else fs.writeFileSync(target,output);
console.log(JSON.stringify({codecs:5,generated_bytes:Buffer.byteLength(output),uncompressed_reference_bytes:size,states:codecs.map(c=>[c.cp,c.contexts,c.states])}));
