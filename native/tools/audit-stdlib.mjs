// Development-only source inventory; never installed with the application.
import fs from 'node:fs/promises';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const modules=['PABCSystem','Arrays','Collections','School','FilesOperations','IniFile'];
function lex(text){
 const tokens=[],directives=[];let i=0,line=1;const step=()=>{if(text[i]==='\n')++line;return text[i++];};
 while(i<text.length){
  if(/\s/u.test(text[i])){step();continue;}
  if(text.startsWith('//',i)){while(i<text.length && text[i]!=='\n')step();continue;}
  if(text[i]==='{' || text.startsWith('(*',i)){
   const start=i,at=line,brace=text[i]==='{';let depth=1;i+=brace?1:2;
   while(i<text.length && depth){if(brace && text[i]==='{'){++depth;step();}else if(brace && text[i]==='}'){--depth;step();}else if(!brace && text.startsWith('(*',i)){++depth;i+=2;}else if(!brace && text.startsWith('*)',i)){--depth;i+=2;}else step();}
   if(depth)throw Error('Unclosed source comment at '+at);if(text[start+1]==='$')directives.push({line:at,text:text.slice(start,i)});continue;
  }
  const start=i,at=line;
  if(text[i]==="'"){step();let closed=false;while(i<text.length){if(step()==="'"){if(text[i]==="'"){step();continue;}closed=true;break;}}if(!closed)throw Error('Unclosed string at '+at);tokens.push({kind:'string',word:text.slice(start,i),start,end:i,line:at});continue;}
  let escaped=false;if(text[i]==='&' && /[\p{L}_]/u.test(text[i+1]||'')){escaped=true;step();}
  if(/[\p{L}_]/u.test(text[i])){while(i<text.length && /[\p{L}\p{N}_]/u.test(text[i]))step();tokens.push({kind:'word',word:text.slice(start+(escaped?1:0),i).toLowerCase(),escaped,start,end:i,line:at});continue;}
  const pair=text.slice(i,i+2);if([':=','->','+=','-=','*=','/=','<>','<=','>=','..'].includes(pair)){i+=2;tokens.push({kind:'symbol',word:pair,start,end:i,line:at});}else{step();tokens.push({kind:'symbol',word:text[start],start,end:i,line:at});}
 }
 return {tokens,directives};
}
const controls=new Set(['begin','try','case','match']);
function bodyEnd(tokens,start){let depth=0;for(let i=start;i<tokens.length;++i){const t=tokens[i];if(t.kind!=='word'||t.escaped||tokens[i-1]?.word==='.')continue;if(controls.has(t.word))++depth;else if(t.word==='end' && --depth===0)return i+1;}throw Error('Unclosed inline routine body at '+tokens[start]?.line);}
function untilSemi(tokens,start,expression=false){let paren=0,square=0;for(let i=start;i<tokens.length;++i){const w=tokens[i].word;if(tokens[i].kind==='string')continue;if(controls.has(w) && tokens[i].kind==='word' && !tokens[i].escaped){i=bodyEnd(tokens,i)-1;continue;}if(w==='(')++paren;if(w===')')--paren;if(w==='[')++square;if(w===']')--square;if(!paren&&!square&&(w===';'||(!expression&&w===':='&&tokens[i-1]?.word!=='operator')))return i;}throw Error('Missing declaration terminator at '+tokens[start]?.line);}
function typeBefore(tokens,at){let eq=at-1;while(['auto','abstract','sealed','partial','static'].includes(tokens[eq]?.word))--eq;if(tokens[eq]?.word!=='=')return null;let name=eq-1;if(tokens[name]?.word==='>'){let d=1;--name;while(name>=0 && d){if(tokens[name].word==='>')++d;if(tokens[name].word==='<')--d;--name;}}if(tokens[name]?.kind!=='word')return null;return tokens.slice(name,eq).map(t=>t.word).join('');}
export function dependencies(text,sourceNames){
 const {tokens,directives}=lex(text),uses=[],external=[];let section='interface';
 for(let i=0;i<tokens.length;++i){const t=tokens[i];if(t.kind!=='word'||t.escaped||tokens[i-1]?.word==='.')continue;
  if(t.word==='implementation')section='implementation';
  if(t.word==='uses'){
   const end=untilSemi(tokens,i);
   for(let j=i+1;j<end;){const begin=j;while(j<end&&tokens[j].word!==',')++j;
    const group=tokens.slice(begin,j),inToken=group.findIndex(x=>x.word==='in'&&!x.escaped),head=inToken<0?group:group.slice(0,inToken),name=head.map(x=>x.word).join('');
    uses.push({name,source_unit:sourceNames.get(name)||null,section,line:tokens[begin].line,source_file:inToken<0?null:group.slice(inToken+1).map(x=>x.word).join('')});++j;
   }i=end;
  }else if(t.word==='external')external.push({line:t.line,library:tokens[i+1]?.kind==='string'?tokens[i+1].word:null});
 }
 return {uses,external,directives,scope:'Lexical references across all conditional branches. A source_unit match identifies a catalog source, not a resolved or unconditional dependency. PABCSystem language prelude and dependencies through aliases, inheritance, reflection or assembly references require semantic review.'};
}
export function inventory(text,name){
 const {tokens,directives}=lex(text),entries=[],declarations=[],imports=[],warnings=[];const scopes=[];let section=null;let start=tokens.findIndex(t=>t.word==='interface'&&!t.escaped);if(start<0)throw Error(name+': no interface section');let implementation=null;
 const declaration=(kind,begin,end,declaredName,visibility,owner='',extra={})=>declarations.push({kind,name:declaredName,owner,visibility,line:tokens[begin].line,end_line:tokens[end].line,source:text.slice(tokens[begin].start,tokens[end].end).trim(),...extra});
 for(let i=start+1;i<tokens.length;){const t=tokens[i],w=t.word;
  if(w==='['){let depth=1;++i;while(i<tokens.length&&depth){if(tokens[i].word==='[')++depth;if(tokens[i].word===']')--depth;++i;}if(depth)throw Error(name+': unclosed attribute at '+t.line);continue;}
  if(t.kind!=='word'||tokens[i-1]?.word==='.'){++i;continue;}
  if(!t.escaped && w==='implementation' && !scopes.length){implementation=t.line;break;}
  if(!t.escaped && w==='uses' && !scopes.length){const end=untilSemi(tokens,i);for(let j=i+1;j<end;){const begin=j;while(j<end&&tokens[j].word!==',')++j;imports.push({name:tokens.slice(begin,j).map(x=>x.word).join(''),line:tokens[begin].line});++j;}i=end+1;continue;}
  if(!t.escaped && ['type','const','var','threadvar'].includes(w)){if(scopes.length)scopes.at(-1).section=w;else section=w;++i;continue;}
  if((scopes.at(-1)?.section||section)==='type' && tokens[i-1]?.word!=='.' && !['class','record','interface','end','public','private','protected','internal','function','procedure','constructor','destructor','property','static'].includes(w)){
   let eq=i+1;if(tokens[eq]?.word==='<'){let depth=0;do{if(tokens[eq].word==='<')++depth;if(tokens[eq].word==='>')--depth;++eq;}while(eq<tokens.length&&depth);}
   if(tokens[eq]?.word==='='){
    let shape=eq+1;while(['auto','abstract','sealed','partial','static'].includes(tokens[shape]?.word))++shape;
    const kind=tokens[shape]?.word,structured=['class','record','interface'].includes(kind)&&tokens[shape+1]?.word!=='of';
    const declaredName=tokens.slice(i,eq).map(x=>x.word).join(''),visibility=scopes.at(-1)?.visibility||'module',owner=scopes.map(s=>s.name).join('.');
    if(structured){let end=shape;if(tokens[end+1]?.word==='('){let depth=0;do{++end;if(tokens[end].word==='(')++depth;if(tokens[end].word===')')--depth;}while(end+1<tokens.length&&depth);}declaration('type',i,end,declaredName,visibility,owner,{shape:kind,forward:tokens[end+1]?.word===';'});i=shape;continue;}
    else{const end=untilSemi(tokens,i,true);declaration('type',i,end,declaredName,visibility,owner,{shape:'alias-or-value'});i=end+1;continue;}
   }
  }
  if(!t.escaped && ['class','record','interface'].includes(w)){const owner=typeBefore(tokens,i);if(owner && tokens[i+1]?.word!==';'&&tokens[i+1]?.word!=='of'){scopes.push({name:owner,kind:w,visibility:w==='class'?'private':'public',section:'field'});if(tokens[i+1]?.word==='where'){i=untilSemi(tokens,i+1,true)+1;continue;}}++i;continue;}
  if(!t.escaped && ['public','private','protected','internal'].includes(w) && scopes.length){scopes.at(-1).visibility=w;++i;continue;}
  if(!t.escaped && w==='end'){if(scopes.length)scopes.pop();else warnings.push({line:t.line,reason:'Unmatched interface end; review extraction'});++i;continue;}
  if(!t.escaped && w==='begin'){i=bodyEnd(tokens,i);continue;}
  if(!t.escaped && ['function','procedure','constructor','destructor','property'].includes(w)){
   const next=tokens[i+1],anonymous=['constructor','destructor'].includes(w)&&['(',':=',';'].includes(next?.word);if(!next || (!anonymous&&next.kind!=='word')){++i;continue;}
   let end=untilSemi(tokens,i),after=end+1,body=tokens[end].word===':=';
   if(!body){while(['overload','override','virtual','abstract','extensionmethod','reintroduce','forward','external','where','inline','default'].includes(tokens[after]?.word)){const stop=untilSemi(tokens,after);end=stop;after=stop+1;if(tokens[stop].word===':='){body=true;break;}}}
   let begin=i;while(begin>0 && ['static','class','classprocedure','classfunction'].includes(tokens[begin-1].word))--begin;
   const signature=tokens.slice(begin,end+(body?0:1)).map(x=>x.word).join(' '),raw=text.slice(tokens[begin].start,tokens[end].start+(body?0:tokens[end].end-tokens[end].start)).trim();
   let head=i+1;if(tokens[head]?.word==='operator')head+=2;else while(head<end && !['(',':',';','[',':='].includes(tokens[head].word))++head;
   const owner=scopes.map(s=>s.name).join('.'),visibility=scopes.at(-1)?.visibility||'module';
   entries.push({id:crypto.createHash('sha256').update(name+'|'+owner+'|'+signature+'|'+t.line).digest('hex').slice(0,16),kind:w,owner,name:anonymous?'':tokens.slice(i+1,head).map(x=>x.word).join(''),visibility,line:tokens[begin].line,end_line:tokens[end].line,signature,source:raw,status:'unreviewed',implementation_evidence:[],verification_evidence:[]});
   if(body){i=untilSemi(tokens,after,true)+1;}else i=after;continue;
  }
  const activeSection=scopes.at(-1)?.section||section;
  if(['const','var','threadvar','field'].includes(activeSection) && !['static','class','public','private','protected','internal'].includes(w)){
   let endName=i+1;while(tokens[endName]?.word===','&&tokens[endName+1]?.kind==='word')endName+=2;
   if([':', '='].includes(tokens[endName]?.word)){
    const end=untilSemi(tokens,i,true),owner=scopes.map(s=>s.name).join('.'),visibility=scopes.at(-1)?.visibility||'module';
    for(let j=i;j<endName;j+=2)declaration(activeSection==='const'?'constant':activeSection==='field'?'field':'variable',i,end,tokens[j].word,visibility,owner,{thread_local:activeSection==='threadvar'});
    i=end+1;continue;
   }
  }
  ++i;
 }
 if(implementation===null)warnings.push({line:tokens.at(-1)?.line,reason:'No top-level implementation boundary found; review scopes'});
 const lifecycle=tokens.filter(t=>t.kind==='word'&&!t.escaped&&['initialization','finalization'].includes(t.word)&&t.line>implementation).map(t=>({kind:t.word,line:t.line}));
 return {name,source:'bin/Lib/'+name+'.pas',source_sha256:crypto.createHash('sha256').update(text.replaceAll('\r\n','\n')).digest('hex'),interface_line:tokens[start].line,implementation_line:implementation,scope:'Lexical interface declaration baseline, including hidden compiler helpers and all preprocessor branches. Not a language parser: inherited/imported APIs, initialization effects and semantic resolution require a separate review. This is not a completeness or progress percentage.',imports,lifecycle,directives:directives.filter(d=>implementation===null||d.line<implementation),warnings,declarations,entries};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
const result={schema:1,scope:'Source declaration baseline only; every entry starts unreviewed. No declaration name is treated as proof of implementation.',modules:[]};
for(const name of modules){const text=await fs.readFile(path.join(root,'bin/Lib',name+'.pas'),'utf8');result.modules.push(inventory(text,name));}
const expected={FilesOperations:2,Arrays:29};for(const [name,count] of Object.entries(expected)){const found=result.modules.find(m=>m.name===name);if(found.entries.length!==count || found.warnings.length)throw Error(name+': inventory needs review: '+found.entries.length+' entries, '+found.warnings.length+' warnings');}
const ini=result.modules.find(m=>m.name==='IniFile');if(ini.entries.filter(e=>e.visibility==='public').length!==9 || ini.warnings.length)throw Error('IniFile public member baseline needs review');
async function emit(name,value){const output=path.join(root,'native/audit',name),json=JSON.stringify(value,null,2)+'\n';if(process.argv.includes('--check')){if((await fs.readFile(output,'utf8')).replaceAll('\r\n','\n')!==json)throw Error(name+' is stale; regenerate and review source changes');}else{await fs.mkdir(path.dirname(output),{recursive:true});await fs.writeFile(output,json);}}
await emit('stdlib-interface-source.json',result);
const catalog=JSON.parse(await fs.readFile(path.join(root,'native/stdlib-status.json'),'utf8')),sourceNames=new Map(catalog.modules.map(m=>[m.name.toLowerCase(),m.name]));
const graph={schema:1,scope:'Source dependency baseline for every catalog module. This does not claim that a module or dependency is implemented.',modules:[]};
for(const m of catalog.modules){const text=await fs.readFile(path.join(root,m.source),'utf8');graph.modules.push({name:m.name,source:m.source,source_sha256:crypto.createHash('sha256').update(text.replaceAll('\r\n','\n')).digest('hex'),...dependencies(text,sourceNames)});}
await emit('stdlib-dependencies-source.json',graph);
for(const m of result.modules)console.log(m.name+': '+m.entries.length+' declarations; '+m.entries.filter(e=>['module','public','protected'].includes(e.visibility)).length+' exposed; '+m.warnings.length+' extraction warnings');
}
