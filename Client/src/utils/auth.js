export function getToken() {
  return localStorage.getItem('token') || ''
}

export function getUid() {
  return localStorage.getItem('uid') || ''
}

export function getRole() {
  return localStorage.getItem('role') || '0'
}

export function isAdmin() {
  return getRole() === '1'
}

export function clearAuth() {
  localStorage.removeItem('token')
  localStorage.removeItem('uid')
  localStorage.removeItem('username')
  localStorage.removeItem('role')
}
