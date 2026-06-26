import request from './request'

export function getVehicleList(params) {
  return request.get('/vehicle/list', { params })
}

export function getVehicleDetail(params) {
  return request.get('/vehicle/detail', { params })
}

export function createVehicle(data) {
  return request.post('/vehicle', data)
}

export function updateVehicle(data) {
  return request.put('/vehicle', data)
}

export function deleteVehicle(data) {
  return request.post('/vehicle/delete', data)
}
