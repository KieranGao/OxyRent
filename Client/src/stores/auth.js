import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { loginUser, registerUser } from '@/api/user'

const ROLE_LABELS = { 0: 'User', 1: 'Staff', 2: 'Admin' }
const ROLE_KEYS = { 0: 'customer', 1: 'staff', 2: 'admin' }

// 后端返回字符串角色，前端转换为数字
function mapRole(role) {
  if (typeof role === 'number') return role
  if (role === 'admin') return 2
  if (role === 'staff') return 1
  return 0 // customer
}

export const useAuthStore = defineStore('auth', () => {
  const token = ref(localStorage.getItem('token') || '')
  const uid = ref(Number(localStorage.getItem('uid')) || 0)
  const username = ref(localStorage.getItem('username') || '')
  const role = ref(Number(localStorage.getItem('role')) || 0)

  const isLoggedIn = computed(() => !!token.value)
  const isAdmin = computed(() => role.value === 2)
  const roleLabel = computed(() => ROLE_LABELS[role.value] || 'User')
  const roleKey = computed(() => ROLE_KEYS[role.value] || 'user')

  function persist(key, value) {
    if (value || value === 0) {
      localStorage.setItem(key, String(value))
    } else {
      localStorage.removeItem(key)
    }
  }

  function clearAuth() {
    token.value = ''
    uid.value = 0
    username.value = ''
    role.value = 0
    localStorage.removeItem('token')
    localStorage.removeItem('uid')
    localStorage.removeItem('username')
    localStorage.removeItem('role')
  }

  async function login({ username: loginUsername, password }) {
    const res = await loginUser({ username: loginUsername, password })
    if (res.error !== 0) throw new Error(res.error || 'Login failed')
    token.value = res.token
    uid.value = res.uid
    username.value = loginUsername
    role.value = mapRole(res.role)

    persist('token', res.token)
    persist('uid', res.uid)
    persist('username', loginUsername)
    persist('role', mapRole(res.role))
    return res
  }

  async function register(data) {
    const res = await registerUser(data)
    if (res.error !== 0) throw new Error(res.error || 'Registration failed')
    return res
  }

  function logout() {
    clearAuth()
    window.location.hash = '#/login'
  }

  return {
    token, uid, username, role,
    isLoggedIn, isAdmin, roleLabel, roleKey,
    login, register, logout, clearAuth,
  }
})
