import {build} from 'esbuild';
import {mkdir,readFile,writeFile} from 'node:fs/promises';
import {dirname,resolve} from 'node:path';
import {fileURLToPath} from 'node:url';

const root=resolve(dirname(fileURLToPath(import.meta.url)),'..');
const result=await build({
  absWorkingDir:root,entryPoints:['src/main.jsx'],bundle:true,write:false,
  outfile:'portable.js',format:'iife',platform:'browser',target:'chrome100',
  jsx:'automatic',minify:true,legalComments:'inline',
  define:{'process.env.NODE_ENV':'"production"'},
  plugins:[{name:'embedded-sysex',setup(b){
    b.onResolve({filter:/\.syx\?base64$/},args=>({path:resolve(args.resolveDir,args.path.slice(0,-7)),namespace:'sysex'}));
    b.onLoad({filter:/.*/,namespace:'sysex'},async args=>({contents:`export default ${JSON.stringify((await readFile(args.path)).toString('base64'))};`,loader:'js'}));
  }}]
});
const js=result.outputFiles.find(f=>f.path.endsWith('.js')).text.replace(/<\/script/gi,'<\\/script');
const css=result.outputFiles.find(f=>f.path.endsWith('.css')).text.replace(/<\/style/gi,'<\\/style');
const html=`<!doctype html>
<html lang="ru"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#202733"><title>X-Station — Synth & Template Editor</title><style>${css}</style></head>
<body><noscript>Для работы редактора включите JavaScript в браузере.</noscript><div id="root"></div><script>${js}</script></body></html>`;
const destination=resolve(root,'../dist/XStationEditor.html');
await mkdir(dirname(destination),{recursive:true});
await writeFile(destination,html);
console.log(`Portable HTML: ${destination} (${Buffer.byteLength(html)} bytes)`);
