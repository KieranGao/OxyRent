import serverConfig from '../../config.json'

const SERVER_HOST = serverConfig.GateServer?.host || '127.0.0.1'
const SERVER_PORT = serverConfig.GateServer?.port || 8080

const isDev = import.meta.env.DEV
// 在Docker中始终使用/api前缀，由nginx代理到gate-server
export const API_BASE_URL = '/api'

export default {
  SERVER_HOST,
  SERVER_PORT,
  API_BASE_URL,
}
