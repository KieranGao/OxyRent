import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'
import fs from 'fs'

const configPath = resolve(__dirname, 'config.json')
let gateHost = '127.0.0.1'
let gatePort = 8080

try {
  const serverConfig = JSON.parse(fs.readFileSync(configPath, 'utf-8'))
  gateHost = serverConfig.GateServer?.host || gateHost
  gatePort = serverConfig.GateServer?.port || gatePort
} catch {
  // config.json not found, use defaults
}

export default defineConfig({
  plugins: [vue()],
  base: './',
  resolve: {
    alias: {
      '@': resolve(__dirname, 'src'),
    },
  },
  server: {
    port: 5173,
    open: true,
    proxy: {
      '/api': {
        target: `http://${gateHost}:${gatePort}`,
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/api/, ''),
      },
    },
  },
  build: {
    outDir: 'dist',
  },
})
