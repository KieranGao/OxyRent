#include "MySQLManager.h"

MySQLManager::MySQLManager() {
    dao_ = std::make_unique<MySQLDao>();
}

// ==================== Payment Operations ====================

int64_t MySQLManager::createPayment(int64_t order_id, double amount, const std::string& type,
                                     const std::string& method, const std::string& remark) {
    return dao_->createPayment(order_id, amount, type, method, remark);
}

bool MySQLManager::getPaymentList(int page, int page_size, int64_t order_id,
                                   const std::string& status, const std::string& type,
                                   std::vector<PaymentData>& payments, int& total) {
    return dao_->getPaymentList(page, page_size, order_id, status, type, payments, total);
}

bool MySQLManager::getPaymentDetail(int64_t id, PaymentData& payment) {
    return dao_->getPaymentDetail(id, payment);
}

// ==================== Invoice Operations ====================

int64_t MySQLManager::generateInvoice(int64_t order_id) {
    return dao_->generateInvoice(order_id);
}

bool MySQLManager::getInvoiceDetail(int64_t id, InvoiceData& invoice) {
    return dao_->getInvoiceDetail(id, invoice);
}

// ==================== Statistics Operations ====================

bool MySQLManager::getStatsOverview(int& total_users, int& total_vehicles, int& available_vehicles,
                                     int& active_orders, int& completed_orders,
                                     double& total_revenue, double& month_revenue) {
    return dao_->getStatsOverview(total_users, total_vehicles, available_vehicles,
                                  active_orders, completed_orders, total_revenue, month_revenue);
}

bool MySQLManager::getRevenueStats(const std::string& start_date, const std::string& end_date,
                                    const std::string& granularity,
                                    std::vector<RevenueStatsItemData>& items, double& total) {
    return dao_->getRevenueStats(start_date, end_date, granularity, items, total);
}

bool MySQLManager::getVehicleStats(int& total, int& available, int& rented, int& maintenance,
                                    double& utilization_rate,
                                    std::vector<VehicleStatsBrandData>& by_brand) {
    return dao_->getVehicleStats(total, available, rented, maintenance, utilization_rate, by_brand);
}
