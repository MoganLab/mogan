import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
// Vite config for the Mogan WASM React shell.
//
// The shell is served alongside the Emscripten-generated stem.js / stem.wasm /
// stem.data in the stem target dir. stem.js is loaded as an external classic
// script (see index.html), NOT bundled by Vite — it owns the WASM module and
// the GL canvas. Vite only builds the surrounding chrome (menu / footer /
// context menu) as a separate module bundle.
export default defineConfig({
    plugins: [react()],
    base: './',
    // Keep hashed asset names out of subfolders so relative asset URLs resolve
    // next to stem.js under both the local dev server and GitHub Pages.
    build: {
        outDir: 'dist',
        emptyOutDir: true,
        assetsDir: '.',
        rollupOptions: {
            output: {
                // Stable, readable chunk names (vite default prepends a hash dir).
                entryFileNames: 'assets/[name].js',
                chunkFileNames: 'assets/[name].js',
                assetFileNames: 'assets/[name][extname]',
            },
        },
    },
    server: {
        headers: {
            // Required for SharedArrayBuffer / pthreads — mirrors bin/wasm_server.py.
            'Cross-Origin-Opener-Policy': 'same-origin',
            'Cross-Origin-Embedder-Policy': 'require-corp',
        },
    },
});
