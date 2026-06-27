import request from './request'

export function loginUser(data) {
  return request.post('/user/login', data)
}

export function registerUser(data) {
  return request.post('/user/register', data)
}

export function getProfile() {
  return request.get('/user/profile')
}

export function updateProfile(data) {
  return request.put('/user/profile', data)
}

export function getUserList(params) {
  return request.get('/user/list', { params })
}

export function updateUserStatus(data) {
  return request.put('/user/status', data)
}

export function updateUserRole(data) {
  return request.put('/user/role', data)
}

export function getBalance() {
  return request.get('/user/balance')
}

export function topupBalance(data) {
  return request.post('/user/topup', data)
}

export function getBalanceRecords(params) {
  return request.get('/user/balance/records', { params })
}

export function sendVerifyCode(data) {
  return request.post('/send-verify-code', data)
}

export function resetPassword(data) {
  return request.post('/user/reset-password', data)
}
