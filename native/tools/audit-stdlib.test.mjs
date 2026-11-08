// Development-only checks for mistakes that would hide declarations in the audit.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {inventory,dependencies} from './audit-stdlib.mjs';
const scan=source=>inventory('unit Sample; interface\n'+source+'\nimplementation end.','Sample');

test('attributes, comments and strings cannot introduce false declarations',()=>{
 const actual=scan(`
 { function Hidden: integer; { nested } }
 (* procedure Hidden; (* nested *) *)
 [AttributeUsage(AttributeTargets.Property or AttributeTargets.Interface)]
 type Thing=class
 public constructor := exit;
 end;
 const Message='procedure Fake; ''function Fake''';
 function Visible: string;
 `);
 assert.deepEqual(actual.entries.map(e=>[e.kind,e.name,e.owner]),[
  ['constructor','','thing'],['function','visible','']
 ]);
 assert.equal(actual.declarations.find(e=>e.name==='message').kind,'constant');
 assert.deepEqual(actual.warnings,[]);
});

test('generic owners, inline bodies and explicit interface member names',()=>{
 const actual=scan(`type Node<T> = class
  secret:T;
 public
  constructor(value:T); begin secret:=value; case 1 of 1: begin end; end; end;
  property Value:T read secret write secret;
  function ICollection<T>.Contains(value:T):boolean := secret=value;
  function ToString:string; override := 'value';
 end;
 function Following<T>(a:T;b:T):T; where T: record;
 `);
 assert.deepEqual(actual.entries.map(e=>[e.name,e.owner,e.visibility]),[
  ['','node<t>','public'],['value','node<t>','public'],
  ['icollection<t>.contains','node<t>','public'],['tostring','node<t>','public'],
  ['following<t>','','module']
 ]);
 assert.equal(actual.declarations.find(e=>e.name==='secret').visibility,'private');
 assert(!actual.entries[3].signature.includes(':='));
 assert.deepEqual(actual.warnings,[]);
});

test('operators cannot swallow the next declaration or generic brackets',()=>{
 const actual=scan(`type R=record
 static function operator<(a,b:R):boolean := false;
 static function operator><T>(a:R;b:T):boolean := true;
 static function operator:=(var a:R;b:R):R;
 static function operator=(a,b:R):boolean;
 property Item[index:integer]:R read GetItem; default;
 end;
 function Last:integer;
 `);
 assert.deepEqual(actual.entries.map(e=>e.name),[
  'operator<','operator>','operator:=','operator=','item','last'
 ]);
 assert.deepEqual(actual.entries.map(e=>e.owner),['r','r','r','r','r','']);
 assert.equal(actual.entries[4].visibility,'public');
 assert.deepEqual(actual.warnings,[]);
});

test('aliases, grouped fields, constants, variables and forward classes',()=>{
 const actual=scan(`uses System.IO,OtherUnit;
 type Callback=procedure(x:integer;y:string);
 P=^integer;
 Future=class;
 Future=class
  left,right:integer;
 public property X:integer read left;
 end;
 const Answer=42;
 var A,B:integer;
 `);
 assert.deepEqual(actual.declarations.map(d=>[d.kind,d.name,d.owner]),[
  ['type','callback',''],['type','p',''],['type','future',''],['type','future',''],
  ['field','left','future'],['field','right','future'],
  ['constant','answer',''],['variable','a',''],['variable','b','']
 ]);
 assert(actual.declarations.find(d=>d.name==='future').forward);
 assert.deepEqual(actual.imports.map(d=>d.name),['system.io','otherunit']);
 assert.deepEqual(actual.entries.map(e=>e.name),['x']);
 assert.deepEqual(actual.warnings,[]);
});

test('conditional branches stay visible and implementation bodies stay excluded',()=>{
 const actual=inventory(`unit Sample; interface
 {$ifdef ALTERNATE}
 function First:integer;
 {$else}
 function Second:integer;
 {$endif}
 implementation
 function InternalOnly:integer; begin Result:=1; end;
 initialization InternalOnly;
 finalization InternalOnly;
 end.`,'Sample');
 assert.deepEqual(actual.entries.map(e=>e.name),['first','second']);
 assert.equal(actual.directives.length,3);
 assert.deepEqual(actual.lifecycle.map(e=>e.kind),['initialization','finalization']);
 assert.deepEqual(actual.warnings,[]);
});

test('constructor constraints are not constructor or field declarations',()=>{
 const actual=scan(`type Factory<T> = class where T: constructor;
  class instance:T;
 public class function Get:T;
 end;`);
 assert.deepEqual(actual.entries.map(e=>e.name),['get']);
 assert.deepEqual(actual.declarations.map(e=>e.name),['factory<t>','instance']);
 assert.equal(actual.entries[0].owner,'factory<t>');
 assert.deepEqual(actual.warnings,[]);
});

test('dependencies retain phases, branches, explicit paths and external libraries',()=>{
 const actual=dependencies(`unit Example; interface
 // uses Fake;
 uses Arrays, System.IO;
 {$ifdef EXTRA}
 uses School in 'school.pas';
 {$endif}
 implementation
 uses Collections;
 procedure Load; external 'example.dll' name 'load';
 const S='uses NotAUnit; external fake';
 end.`,new Map([['arrays','Arrays'],['school','School'],['collections','Collections']]));
 assert.deepEqual(actual.uses.map(u=>[u.name,u.source_unit,u.section,u.source_file]),[
  ['arrays','Arrays','interface',null],['system.io',null,'interface',null],
  ['school','School','interface',"'school.pas'"],['collections','Collections','implementation',null]
 ]);
 assert.deepEqual(actual.external.map(e=>e.library),["'example.dll'"]);
 assert.equal(actual.directives.length,2);
});
