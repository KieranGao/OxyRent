import request from './request'

export function createPayment(data) {
  return request.post('/payment/create', data)
}

export function getPaymentList(params) {
  return request.get('/payment/list', { params })
}

export function getPaymentDetail(params) {
  return request.get('/payment/detail', { params })
}

export function generateInvoice(data) {
  return request.post('/invoice/generate', data)
}

export function getInvoiceDetail(params) {
  return request.get('/invoice/detail', { params })
}

export function getInvoiceList(params) {
  return request.get('/invoice/list', { params })
}

export function getStatsOverview() {
  return request.get('/stats/overview')
}

export function getStatsRevenue(params) {
  return request.get('/stats/revenue', { params })
}

export function getStatsVehicles() {
  return request.get('/stats/vehicles')
}
