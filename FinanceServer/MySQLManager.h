#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H

#include "MySQLDao.h"
#include "Singleton.h"

class MySQLManager : public Singleton<MySQLManager> {
    friend class Singleton<MySQLManager>;
public:
    ~MySQLManager() = default;

    // Payment operations
    int64_t createPayment(int64_t order_id, double amount, const std::string& type,
                          const std::string& method, const std::string& remark);
    bool confirmPayment(int64_t id);
    bool getPaymentList(int page, int page_size, int64_t order_id,
                        const std::string& status, const std::string& type,
                        std::vector<PaymentData>& payments, int& total);
    bool getPaymentDetail(int64_t id, PaymentData& payment);

    // Invoice operations
    int64_t generateInvoice(int64_t order_id);
    bool getInvoiceDetail(int64_t id, InvoiceData& invoice);
    bool getInvoiceList(int page, int page_size, std::vector<InvoiceData>& invoices, int& total);

    // Statistics operations
    bool getStatsOverview(int& total_users, int& total_vehicles, int& available_vehicles,
                          int& active_orders, int& completed_orders,
                          double& total_revenue, double& month_revenue);
    bool getRevenueStats(const std::string& start_date, const std::string& end_date,
                         const std::string& granularity,
                         std::vector<RevenueStatsItemData>& items, double& total);
    bool getVehicleStats(int& total, int& available, int& rented, int& maintenance,
                         double& utilization_rate,
                         std::vector<VehicleStatsBrandData>& by_brand);

private:
    MySQLManager();
    std::unique_ptr<MySQLDao> dao_;
};

#endif /* MYSQLMANAGER_H */
