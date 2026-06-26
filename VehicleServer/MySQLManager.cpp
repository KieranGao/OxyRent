#include "MySQLManager.h"

MySQLManager::MySQLManager() {
    dao_ = std::make_unique<MySQLDao>();
}

// ==================== Vehicle Operations ====================

bool MySQLManager::getVehicleList(int page, int page_size, const std::string& keyword,
                                   const std::string& status, const std::string& brand,
                                   std::vector<VehicleData>& vehicles, int& total) {
    return dao_->getVehicleList(page, page_size, keyword, status, brand, vehicles, total);
}

bool MySQLManager::getVehicleDetail(int64_t id, VehicleData& vehicle) {
    return dao_->getVehicleDetail(id, vehicle);
}

int64_t MySQLManager::addVehicle(const std::string& plate_number, const std::string& brand,
                                  const std::string& model, const std::string& color,
                                  int year, int mileage, double daily_rate, double deposit_amount,
                                  const std::string& image_url, const std::string& description) {
    return dao_->addVehicle(plate_number, brand, model, color, year, mileage,
                            daily_rate, deposit_amount, image_url, description);
}

bool MySQLManager::updateVehicle(int64_t id, const std::string& plate_number, const std::string& brand,
                                  const std::string& model, const std::string& color,
                                  int year, int mileage, const std::string& status,
                                  double daily_rate, double deposit_amount,
                                  const std::string& image_url, const std::string& description) {
    return dao_->updateVehicle(id, plate_number, brand, model, color, year, mileage,
                               status, daily_rate, deposit_amount, image_url, description);
}

int MySQLManager::deleteVehicle(int64_t id) {
    return dao_->deleteVehicle(id);
}

bool MySQLManager::updateVehicleStatus(int64_t id, const std::string& status) {
    return dao_->updateVehicleStatus(id, status);
}

bool MySQLManager::getVehicleStatus(int64_t id, std::string& status, double& daily_rate, double& deposit_amount) {
    return dao_->getVehicleStatus(id, status, daily_rate, deposit_amount);
}

// ==================== Rental Order Operations ====================

int64_t MySQLManager::createOrder(int64_t user_id, int64_t vehicle_id,
                                   const std::string& start_date, const std::string& end_date,
                                   const std::string& notes, double deposit, double daily_rate,
                                   int total_days) {
    return dao_->createOrder(user_id, vehicle_id, start_date, end_date, notes,
                             deposit, daily_rate, total_days);
}

bool MySQLManager::getOrderList(int page, int page_size, int64_t user_id, int64_t vehicle_id,
                                 const std::string& status, const std::string& keyword,
                                 std::vector<OrderData>& orders, int& total) {
    return dao_->getOrderList(page, page_size, user_id, vehicle_id, status, keyword, orders, total);
}

bool MySQLManager::getOrderDetail(int64_t id, OrderData& order) {
    return dao_->getOrderDetail(id, order);
}

bool MySQLManager::pickupVehicle(int64_t order_id, int64_t vehicle_id) {
    return dao_->pickupVehicle(order_id, vehicle_id);
}

bool MySQLManager::returnVehicle(int64_t order_id, int64_t vehicle_id, const std::string& actual_return_date,
                                  int actual_days, int planned_days, double daily_rate,
                                  double& total_cost, double& penalty) {
    return dao_->returnVehicle(order_id, vehicle_id, actual_return_date, actual_days, planned_days,
                               daily_rate, total_cost, penalty);
}

bool MySQLManager::renewOrder(int64_t order_id, const std::string& new_end_date,
                               int new_total_days, double daily_rate, double& total_cost) {
    return dao_->renewOrder(order_id, new_end_date, new_total_days, daily_rate, total_cost);
}

// ==================== Maintenance Operations ====================

int64_t MySQLManager::createMaintenance(int64_t vehicle_id, const std::string& type,
                                         const std::string& description, double cost,
                                         const std::string& technician, const std::string& start_date) {
    return dao_->createMaintenance(vehicle_id, type, description, cost, technician, start_date);
}

bool MySQLManager::updateMaintenance(int64_t id, const std::string& description, double cost,
                                      const std::string& technician, const std::string& end_date,
                                      const std::string& status) {
    return dao_->updateMaintenance(id, description, cost, technician, end_date, status);
}

bool MySQLManager::getMaintenanceList(int page, int page_size, int64_t vehicle_id,
                                       const std::string& status, const std::string& type,
                                       std::vector<MaintenanceData>& records, int& total) {
    return dao_->getMaintenanceList(page, page_size, vehicle_id, status, type, records, total);
}

bool MySQLManager::deleteMaintenance(int64_t id) {
    return dao_->deleteMaintenance(id);
}
