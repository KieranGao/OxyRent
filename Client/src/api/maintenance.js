import request from './request'

export function createMaintenance(data) {
  return request.post('/maintenance/create', data)
}

export function updateMaintenance(data) {
  return request.put('/maintenance', data)
}

export function getMaintenanceList(params) {
  return request.get('/maintenance/list', { params })
}

export function deleteMaintenance(data) {
  return request.post('/maintenance/delete', data)
}
