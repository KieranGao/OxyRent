import request from './request'

export function getStatsOverview() {
  return request.get('/stats/overview')
}

export function getStatsRevenue(params) {
  return request.get('/stats/revenue', { params })
}

export function getStatsVehicles() {
  return request.get('/stats/vehicles')
}
