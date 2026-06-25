#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H

#include "MySQLDao.h"
#include "Singleton.h"

class MySQLManager : public Singleton<MySQLManager> {
    friend class Singleton<MySQLManager>;
public:
    ~MySQLManager() = default;

    // Vehicle operations
    bool getVehicleList(int page, int page_size, const std::string& keyword,
                        const std::string& status, const std::string& brand,
                        std::vector<VehicleData>& vehicles, int& total);
    bool getVehicleDetail(int64_t id, VehicleData& vehicle);
    int64_t addVehicle(const std::string& plate_number, const std::string& brand,
                       const std::string& model, const std::string& color,
                       int year, int mileage, double daily_rate, double deposit_amount,
                       const std::string& image_url, const std::string& description);
    bool updateVehicle(int64_t id, const std::string& plate_number, const std::string& brand,
                       const std::string& model, const std::string& color,
                       int year, int mileage, const std::string& status,
                       double daily_rate, double deposit_amount,
                       const std::string& image_url, const std::string& description);
    int deleteVehicle(int64_t id);
    bool updateVehicleStatus(int64_t id, const std::string& status);
    bool getVehicleStatus(int64_t id, std::string& status, double& daily_rate, double& deposit_amount);

    // Rental order operations
    int64_t createOrder(int64_t user_id, int64_t vehicle_id,
                        const std::string& start_date, const std::string& end_date,
                        const std::string& notes, double deposit, double daily_rate,
                        int total_days);
    bool getOrderList(int page, int page_size, int64_t user_id, int64_t vehicle_id,
                      const std::string& status, const std::string& keyword,
                      std::vector<OrderData>& orders, int& total);
    bool getOrderDetail(int64_t id, OrderData& order);
    bool pickupVehicle(int64_t order_id, int64_t vehicle_id);
    bool returnVehicle(int64_t order_id, int64_t vehicle_id, const std::string& actual_return_date,
                       int actual_days, int planned_days, double daily_rate,
                       double& total_cost, double& penalty);
    bool renewOrder(int64_t order_id, const std::string& new_end_date,
                    int new_total_days, double daily_rate, double& total_cost);

    // Maintenance operations
    int64_t createMaintenance(int64_t vehicle_id, const std::string& type,
                              const std::string& description, double cost,
                              const std::string& technician, const std::string& start_date);
    bool updateMaintenance(int64_t id, const std::string& description, double cost,
                           const std::string& technician, const std::string& end_date,
                           const std::string& status);
    bool getMaintenanceList(int page, int page_size, int64_t vehicle_id,
                            const std::string& status, const std::string& type,
                            std::vector<MaintenanceData>& records, int& total);

private:
    MySQLManager();
    std::unique_ptr<MySQLDao> dao_;
};

#endif /* MYSQLMANAGER_H */
