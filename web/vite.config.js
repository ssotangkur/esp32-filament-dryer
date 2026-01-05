import { defineConfig, loadEnv } from 'vite'
import react from '@vitejs/plugin-react'

// https://vitejs.dev/config/
export default defineConfig(({ mode }) => {
  // Load environment variables
  const env = loadEnv(mode, process.cwd(), '')

  return {
    root: '.',
    plugins: [
      react(),
    ],
    server: {
      proxy: {
        '/api': {
          target: `http://${env.VITE_DEVICE_IP}:3000`,
          changeOrigin: true,
        },
        '/ws': {
          target: `ws://${env.VITE_DEVICE_IP}:3000`,
          ws: true,
          changeOrigin: true,
        },
      },
    },
  }
})
