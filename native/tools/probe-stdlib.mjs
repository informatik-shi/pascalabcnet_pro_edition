// Development-only audit. Invokes the native checker, never the original compiler.
import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {createHash} from 'node:crypto';
const exec=promisify(execFile);
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const args=process.argv.slice(2);
const runnerOption=args.indexOf('--runner');
if(runnerOption<0||!args[runnerOption+1])throw Error('Specify --runner native/path/to/abc.exe');
const runner=path.resolve(root,args[runnerOption+1]);
if(!/^abc(?:\.exe)?$/i.test(path.basename(runner)))throw Error('Only the native abc checker is supported');
const {stdout:version}=await exec(runner,['--version'],{encoding:'utf8',windowsHide:true,timeout:10000});
if(!version.startsWith('ABC Native '))throw Error('Runner is not the native checker');
const corpus=JSON.parse(await fs.readFile(path.join(root,'native/audit/stdlib-required-cases.json'),'utf8'));
const baseline=JSON.parse(await fs.readFile(path.join(root,'native/audit/stdlib-interface-source.json'),'utf8'));
const sources=new Map();
for(const module of baseline.modules){const source=(await fs.readFile(path.join(root,module.source),'utf8')).replaceAll('\r\n','\n');if(createHash('sha256').update(source).digest('hex')!==module.source_sha256)throw Error('Source baseline is stale: '+module.name);sources.set(module.name,source.split('\n'));}
const temporary=await fs.mkdtemp(path.join(root,'native/build/stdlib-probe-'));
const report={schema:1,scope:corpus.scope,runner:path.relative(root,runner).replaceAll('\\','/'),runner_sha256:createHash('sha256').update(await fs.readFile(runner)).digest('hex'),version:version.trim(),cases:[]};
try{
 const ids=new Set();
 for(const entry of corpus.cases){
  if(!/^[a-z0-9-]+$/.test(entry.id)||ids.has(entry.id))throw Error('Invalid or duplicated case id');
  const anchor=sources.get(entry.module)?.[entry.source_line-1]?.trim();if(!anchor)throw Error('Missing source anchor for '+entry.id);
  ids.add(entry.id);
  const filename=path.join(temporary,entry.id+'.pas');
  await fs.writeFile(filename,entry.source+'\n');
  let accepted=false,diagnostic='';
  try{await exec(runner,['check',filename],{encoding:'utf8',windowsHide:true,timeout:10000});accepted=true;}
  catch(error){if(error.code!==1)throw error;diagnostic=error.stderr.replaceAll(filename,entry.id+'.pas').replaceAll('\r\n','\n').trim();}
  report.cases.push({id:entry.id,module:entry.module,source_line:entry.source_line,source_anchor:anchor,accepted,diagnostic});
 }
}finally{
 // mkdtemp created this exact directory; verify containment before recursive cleanup.
 const build=await fs.realpath(path.join(root,'native/build')),target=await fs.realpath(temporary);
 if(path.dirname(target)!==build||!path.basename(target).startsWith('stdlib-probe-'))throw Error('Unsafe temporary directory');
 await fs.rm(target,{recursive:true});
}
report.summary={accepted:report.cases.filter(c=>c.accepted).length,required:report.cases.length};
const output=path.join(root,'native/build/stdlib-required-cases-report.json');
await fs.writeFile(output,JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify(report.summary));
console.log('Report: '+path.relative(root,output));
if(args.includes('--require-complete')&&report.summary.accepted!==report.summary.required)process.exitCode=1;
