import request from './request'

export function createRental(data) {
  return request.post('/rental/create', data)
}

export function getRentalList(params) {
  return request.get('/rental/list', { params })
}

export function getRentalDetail(params) {
  return request.get('/rental/detail', { params })
}

export function pickupVehicle(data) {
  return request.post('/rental/pickup', data)
}

export function returnVehicle(data) {
  return request.post('/rental/return', data)
}

export function renewRental(data) {
  return request.post('/rental/renew', data)
}
