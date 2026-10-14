import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import {readFile} from 'node:fs/promises';
export default defineConfig({plugins:[react(),{
  name:'embedded-sysex',enforce:'pre',
  async load(id){if(id.endsWith('.syx?base64'))return `export default ${JSON.stringify((await readFile(id.slice(0,-7))).toString('base64'))};`;}
}], base:'./', build:{outDir:'build'}});
