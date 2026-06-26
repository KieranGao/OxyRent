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
