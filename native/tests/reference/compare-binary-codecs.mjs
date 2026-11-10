// Independent public API results are retained when runtime versions disagree.
import fs from 'node:fs';
const [framework, modern, output] = process.argv.slice(2);
if (!output) throw Error('Pass Framework trace, modern trace, difference JSON');
const lines = file => fs.readFileSync(file, 'utf8').trim().split(/\r?\n/);
const old = lines(framework), recent = lines(modern);
if (old.length !== recent.length) throw Error('Binary codec trace counts differ');
const differences = [];
for (let row = 1; row < old.length; ++row)
  if (old[row] !== recent[row]) differences.push({row, framework: old[row], modern: recent[row]});
fs.writeFileSync(output, JSON.stringify({schema:1, rows:old.length-1, differences:differences.length,
  scope:'Actual public BinaryReader/BinaryWriter traces. Native compatibility target is Framework. Modern malformed-prefix and decoder differences remain explicit; neither runtime is shipped.', outcomes:differences}, null, 2)+'\n');
console.log(`${old.length-1} binary codec rows; ${differences.length} runtime differences retained`);
