#include "VehicleGrpcServiceImpl.h"
#include "MySQLManager.h"
#include "RedisManager.h"
#include "Logger.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cmath>
#include <random>

VehicleGrpcServiceImpl::VehicleGrpcServiceImpl() {}

// 生成唯一的锁持有者ID（用于分布式锁）
static std::string generateLockOwnerId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);
    std::ostringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return ss.str();
}

static std::string todayStr() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&tt, &tm);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return buf;
}

static int daysBetween(const std::string& start, const std::string& end) {
    std::tm tm_start = {}, tm_end = {};
    strptime(start.c_str(), "%Y-%m-%d", &tm_start);
    strptime(end.c_str(), "%Y-%m-%d", &tm_end);
    time_t t_start = mktime(&tm_start);
    time_t t_end = mktime(&tm_end);
    return static_cast<int>(difftime(t_end, t_start) / 86400);
}

// ==================== 车辆管理 ====================

Status VehicleGrpcServiceImpl::GetVehicleList(ServerContext* context, const VehicleListRequest* req, VehicleListResponse* resp) {
    LOG_DEBUG("[Vehicle] GetVehicleList page={} page_size={}", req->page(), req->page_size());

    std::vector<VehicleData> vehicles;
    int total = 0;
    bool ok = MySQLManager::getInstance().getVehicleList(
        req->page(), req->page_size(), req->keyword(), req->status(), req->brand(), vehicles, total);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to fetch vehicle list");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_total(total);
    for (auto& v : vehicles) {
        auto* item = resp->add_vehicles();
        item->set_id(v.id);
        item->set_plate_number(v.plate_number);
        item->set_brand(v.brand);
        item->set_model(v.model);
        item->set_color(v.color);
        item->set_year(v.year);
        item->set_mileage(v.mileage);
        item->set_status(v.status);
        item->set_daily_rate(v.daily_rate);
        item->set_deposit_amount(v.deposit_amount);
        item->set_image_url(v.image_url);
        item->set_description(v.description);
        item->set_created_at(v.created_at);
    }
    return Status::OK;
}

Status VehicleGrpcServiceImpl::GetVehicleDetail(ServerContext* context, const VehicleDetailRequest* req, VehicleInfo* resp) {
    int64_t id = req->id();
    LOG_DEBUG("[Vehicle] GetVehicleDetail id={}", id);

    VehicleData vehicle;
    bool ok = MySQLManager::getInstance().getVehicleDetail(id, vehicle);
    if (!ok) {
        resp->set_id(0);
        LOG_WARN("[Vehicle] Vehicle not found: {}", id);
        return Status::OK;
    }

    resp->set_id(vehicle.id);
    resp->set_plate_number(vehicle.plate_number);
    resp->set_brand(vehicle.brand);
    resp->set_model(vehicle.model);
    resp->set_color(vehicle.color);
    resp->set_year(vehicle.year);
    resp->set_mileage(vehicle.mileage);
    resp->set_status(vehicle.status);
    resp->set_daily_rate(vehicle.daily_rate);
    resp->set_deposit_amount(vehicle.deposit_amount);
    resp->set_image_url(vehicle.image_url);
    resp->set_description(vehicle.description);
    resp->set_created_at(vehicle.created_at);
    return Status::OK;
}

Status VehicleGrpcServiceImpl::AddVehicle(ServerContext* context, const AddVehicleRequest* req, CommonResponse* resp) {
    LOG_DEBUG("[Vehicle] AddVehicle plate={}", req->plate_number());

    int64_t id = MySQLManager::getInstance().addVehicle(
        req->plate_number(), req->brand(), req->model(), req->color(),
        req->year(), req->mileage(), req->daily_rate(), req->deposit_amount(),
        req->image_url(), req->description());

    if (id == -1) {
        resp->set_error(static_cast<int>(ErrorCodes::VEHICLE_UNAVAILABLE));
        resp->set_msg("Plate number already exists");
        return Status::OK;
    }
    if (id <= 0) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to add vehicle");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("Vehicle added successfully");
    resp->set_data(std::to_string(id));
    LOG_INFO("[Vehicle] AddVehicle success, id={}", id);
    return Status::OK;
}

Status VehicleGrpcServiceImpl::UpdateVehicle(ServerContext* context, const UpdateVehicleRequest* req, CommonResponse* resp) {
    int64_t id = req->id();
    LOG_DEBUG("[Vehicle] UpdateVehicle id={}", id);

    bool ok = MySQLManager::getInstance().updateVehicle(
        id, req->plate_number(), req->brand(), req->model(), req->color(),
        req->year(), req->mileage(), req->status(), req->daily_rate(),
        req->deposit_amount(), req->image_url(), req->description());

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::VEHICLE_NOT_FOUND));
        resp->set_msg("Vehicle not found or update failed");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("Vehicle updated successfully");
    return Status::OK;
}

Status VehicleGrpcServiceImpl::DeleteVehicle(ServerContext* context, const VehicleDetailRequest* req, CommonResponse* resp) {
    int64_t id = req->id();
    LOG_INFO("[Vehicle] DeleteVehicle id={}", id);

    int result = MySQLManager::getInstance().deleteVehicle(id);
    if (result == -1) {
        resp->set_error(static_cast<int>(ErrorCodes::VEHICLE_UNAVAILABLE));
        resp->set_msg("Cannot delete vehicle: it has active or pending rental orders");
        return Status::OK;
    }
    if (result == 0) {
        resp->set_error(static_cast<int>(ErrorCodes::VEHICLE_NOT_FOUND));
        resp->set_msg("Vehicle not found");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("Vehicle deleted successfully");
    return Status::OK;
}

// ==================== 租赁订单 ====================

Status VehicleGrpcServiceImpl::CreateOrder(ServerContext* context, const CreateOrderRequest* req, OrderInfo* resp) {
    LOG_DEBUG("[Vehicle] CreateOrder user_id={} vehicle_id={}", req->user_id(), req->vehicle_id());

    // 分布式锁：锁定车辆，防止同一辆车被重复租赁
    std::string lock_key = "lock:vehicle:" + std::to_string(req->vehicle_id());
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_id(0);
        LOG_WARN("[Vehicle] Failed to acquire lock for vehicle: {}", req->vehicle_id());
        return Status::OK;
    }

    // 检查车辆是否可用
    std::string vehicle_status;
    double daily_rate = 0.0, deposit_amount = 0.0;
    bool ok = MySQLManager::getInstance().getVehicleStatus(req->vehicle_id(), vehicle_status, daily_rate, deposit_amount);
    if (!ok) {
        redis.releaseLock(lock_key, owner_id);
        resp->set_id(0);
        LOG_WARN("[Vehicle] Vehicle not found for order: {}", req->vehicle_id());
        return Status::OK;
    }
    if (vehicle_status != "available") {
        redis.releaseLock(lock_key, owner_id);
        resp->set_id(0);
        LOG_WARN("[Vehicle] Vehicle not available: {} status={}", req->vehicle_id(), vehicle_status);
        return Status::OK;
    }

    std::string start_date = req->start_date();
    std::string end_date = req->end_date();
    int total_days = daysBetween(start_date, end_date);
    if (total_days <= 0) {
        redis.releaseLock(lock_key, owner_id);
        resp->set_id(0);
        LOG_WARN("[Vehicle] Invalid date range: {} to {}", start_date, end_date);
        return Status::OK;
    }

    int64_t order_id = MySQLManager::getInstance().createOrder(
        req->user_id(), req->vehicle_id(), start_date, end_date,
        req->notes(), deposit_amount, daily_rate, total_days);

    // 释放锁
    redis.releaseLock(lock_key, owner_id);

    if (order_id <= 0) {
        resp->set_id(0);
        LOG_ERROR("[Vehicle] Failed to create order");
        return Status::OK;
    }

    // 返回创建的订单详情
    OrderData order;
    MySQLManager::getInstance().getOrderDetail(order_id, order);
    resp->set_id(order.id);
    resp->set_order_no(order.order_no);
    resp->set_user_id(order.user_id);
    resp->set_username(order.username);
    resp->set_vehicle_id(order.vehicle_id);
    resp->set_plate_number(order.plate_number);
    resp->set_vehicle_info(order.vehicle_info);
    resp->set_start_date(order.start_date);
    resp->set_end_date(order.end_date);
    resp->set_status(order.status);
    resp->set_deposit(order.deposit);
    resp->set_daily_rate(order.daily_rate);
    resp->set_total_days(order.total_days);
    resp->set_total_cost(order.total_cost);
    resp->set_notes(order.notes);
    resp->set_created_at(order.created_at);

    LOG_INFO("[Vehicle] CreateOrder success, id={} order_no={}", order_id, order.order_no);
    return Status::OK;
}

Status VehicleGrpcServiceImpl::GetOrderList(ServerContext* context, const OrderListRequest* req, OrderListResponse* resp) {
    LOG_DEBUG("[Vehicle] GetOrderList page={} page_size={}", req->page(), req->page_size());

    std::vector<OrderData> orders;
    int total = 0;
    bool ok = MySQLManager::getInstance().getOrderList(
        req->page(), req->page_size(), req->user_id(), req->vehicle_id(),
        req->status(), req->keyword(), orders, total);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to fetch order list");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_total(total);
    for (auto& o : orders) {
        auto* item = resp->add_orders();
        item->set_id(o.id);
        item->set_order_no(o.order_no);
        item->set_user_id(o.user_id);
        item->set_username(o.username);
        item->set_vehicle_id(o.vehicle_id);
        item->set_plate_number(o.plate_number);
        item->set_vehicle_info(o.vehicle_info);
        item->set_start_date(o.start_date);
        item->set_end_date(o.end_date);
        item->set_actual_return_date(o.actual_return_date);
        item->set_status(o.status);
        item->set_deposit(o.deposit);
        item->set_daily_rate(o.daily_rate);
        item->set_total_days(o.total_days);
        item->set_total_cost(o.total_cost);
        item->set_penalty(o.penalty);
        item->set_notes(o.notes);
        item->set_created_at(o.created_at);
    }
    return Status::OK;
}

Status VehicleGrpcServiceImpl::GetOrderDetail(ServerContext* context, const OrderDetailRequest* req, OrderInfo* resp) {
    int64_t id = req->id();
    LOG_DEBUG("[Vehicle] GetOrderDetail id={}", id);

    OrderData order;
    bool ok = MySQLManager::getInstance().getOrderDetail(id, order);
    if (!ok) {
        resp->set_id(0);
        LOG_WARN("[Vehicle] Order not found: {}", id);
        return Status::OK;
    }

    resp->set_id(order.id);
    resp->set_order_no(order.order_no);
    resp->set_user_id(order.user_id);
    resp->set_username(order.username);
    resp->set_vehicle_id(order.vehicle_id);
    resp->set_plate_number(order.plate_number);
    resp->set_vehicle_info(order.vehicle_info);
    resp->set_start_date(order.start_date);
    resp->set_end_date(order.end_date);
    resp->set_actual_return_date(order.actual_return_date);
    resp->set_status(order.status);
    resp->set_deposit(order.deposit);
    resp->set_daily_rate(order.daily_rate);
    resp->set_total_days(order.total_days);
    resp->set_total_cost(order.total_cost);
    resp->set_penalty(order.penalty);
    resp->set_notes(order.notes);
    resp->set_created_at(order.created_at);
    return Status::OK;
}

Status VehicleGrpcServiceImpl::PickupVehicle(ServerContext* context, const PickupRequest* req, CommonResponse* resp) {
    int64_t order_id = req->order_id();
    LOG_INFO("[Vehicle] PickupVehicle order_id={}", order_id);

    // 分布式锁：锁定订单，防止并发操作
    std::string lock_key = "lock:order:" + std::to_string(order_id);
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("System busy, please retry");
        return Status::OK;
    }

    // 获取订单详情以获取vehicle_id
    OrderData order;
    bool ok = MySQLManager::getInstance().getOrderDetail(order_id, order);
    if (!ok) {
        redis.releaseLock(lock_key, owner_id);
        resp->set_error(static_cast<int>(ErrorCodes::RENTAL_ORDER_NOT_FOUND));
        resp->set_msg("Order not found");
        return Status::OK;
    }

    ok = MySQLManager::getInstance().pickupVehicle(order_id, order.vehicle_id);
    redis.releaseLock(lock_key, owner_id);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RENTAL_ORDER_NOT_FOUND));
        resp->set_msg("Order not found or not in pending status");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("Vehicle picked up successfully");
    return Status::OK;
}

Status VehicleGrpcServiceImpl::ReturnVehicle(ServerContext* context, const ReturnRequest* req, OrderInfo* resp) {
    int64_t order_id = req->order_id();
    LOG_INFO("[Vehicle] ReturnVehicle order_id={}", order_id);

    // 分布式锁：锁定订单，防止并发还车
    std::string lock_key = "lock:order:" + std::to_string(order_id);
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_id(0);
        LOG_WARN("[Vehicle] Failed to acquire lock for return order: {}", order_id);
        return Status::OK;
    }

    // 先获取订单详情
    OrderData order;
    bool ok = MySQLManager::getInstance().getOrderDetail(order_id, order);
    if (!ok) {
        redis.releaseLock(lock_key, owner_id);
        resp->set_id(0);
        LOG_WARN("[Vehicle] Order not found for return: {}", order_id);
        return Status::OK;
    }

    std::string actual_return = todayStr();
    int actual_days = daysBetween(order.start_date, actual_return);
    int planned_days = order.total_days;

    double total_cost = 0.0, penalty = 0.0;
    ok = MySQLManager::getInstance().returnVehicle(
        order_id, order.vehicle_id, actual_return, actual_days, planned_days,
        order.daily_rate, total_cost, penalty);

    // 释放锁
    redis.releaseLock(lock_key, owner_id);

    if (!ok) {
        resp->set_id(0);
        LOG_ERROR("[Vehicle] Failed to return vehicle for order: {}", order_id);
        return Status::OK;
    }

    // 返回更新后的订单详情
    MySQLManager::getInstance().getOrderDetail(order_id, order);
    resp->set_id(order.id);
    resp->set_order_no(order.order_no);
    resp->set_user_id(order.user_id);
    resp->set_username(order.username);
    resp->set_vehicle_id(order.vehicle_id);
    resp->set_plate_number(order.plate_number);
    resp->set_vehicle_info(order.vehicle_info);
    resp->set_start_date(order.start_date);
    resp->set_end_date(order.end_date);
    resp->set_actual_return_date(order.actual_return_date);
    resp->set_status(order.status);
    resp->set_deposit(order.deposit);
    resp->set_daily_rate(order.daily_rate);
    resp->set_total_days(order.total_days);
    resp->set_total_cost(order.total_cost);
    resp->set_penalty(order.penalty);
    resp->set_notes(order.notes);
    resp->set_created_at(order.created_at);

    LOG_INFO("[Vehicle] ReturnVehicle success, order_id={}, total_cost={}, penalty={}", order_id, total_cost, penalty);
    return Status::OK;
}

Status VehicleGrpcServiceImpl::RenewOrder(ServerContext* context, const RenewRequest* req, OrderInfo* resp) {
    int64_t order_id = req->order_id();
    std::string new_end_date = req->new_end_date();
    LOG_INFO("[Vehicle] RenewOrder order_id={} new_end_date={}", order_id, new_end_date);

    // 分布式锁：锁定订单，防止并发续租
    std::string lock_key = "lock:order:" + std::to_string(order_id);
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_id(0);
        LOG_WARN("[Vehicle] Failed to acquire lock for renew order: {}", order_id);
        return Status::OK;
    }

    // 获取订单详情
    OrderData order;
    bool ok = MySQLManager::getInstance().getOrderDetail(order_id, order);
    if (!ok) {
        redis.releaseLock(lock_key, owner_id);
        resp->set_id(0);
        LOG_WARN("[Vehicle] Order not found for renewal: {}", order_id);
        return Status::OK;
    }

    int new_total_days = daysBetween(order.start_date, new_end_date);
    if (new_total_days <= order.total_days) {
        redis.releaseLock(lock_key, owner_id);
        resp->set_id(0);
        LOG_WARN("[Vehicle] New end date must be after current end date");
        return Status::OK;
    }

    double total_cost = 0.0;
    ok = MySQLManager::getInstance().renewOrder(
        order_id, new_end_date, new_total_days, order.daily_rate, total_cost);

    // 释放锁
    redis.releaseLock(lock_key, owner_id);

    if (!ok) {
        resp->set_id(0);
        LOG_ERROR("[Vehicle] Failed to renew order: {}", order_id);
        return Status::OK;
    }

    // 返回更新后的订单详情
    MySQLManager::getInstance().getOrderDetail(order_id, order);
    resp->set_id(order.id);
    resp->set_order_no(order.order_no);
    resp->set_user_id(order.user_id);
    resp->set_username(order.username);
    resp->set_vehicle_id(order.vehicle_id);
    resp->set_plate_number(order.plate_number);
    resp->set_vehicle_info(order.vehicle_info);
    resp->set_start_date(order.start_date);
    resp->set_end_date(order.end_date);
    resp->set_actual_return_date(order.actual_return_date);
    resp->set_status(order.status);
    resp->set_deposit(order.deposit);
    resp->set_daily_rate(order.daily_rate);
    resp->set_total_days(order.total_days);
    resp->set_total_cost(order.total_cost);
    resp->set_penalty(order.penalty);
    resp->set_notes(order.notes);
    resp->set_created_at(order.created_at);

    LOG_INFO("[Vehicle] RenewOrder success, order_id={}, new_total_cost={}", order_id, total_cost);
    return Status::OK;
}

Status VehicleGrpcServiceImpl::CancelOrder(ServerContext* context, const PickupRequest* req, CommonResponse* resp) {
    int64_t order_id = req->order_id();
    LOG_INFO("[Vehicle] CancelOrder order_id={}", order_id);

    // 分布式锁：锁定订单，防止并发取消
    std::string lock_key = "lock:order:" + std::to_string(order_id);
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("System busy, please retry");
        return Status::OK;
    }

    // 获取订单详情以获取vehicle_id
    OrderData order;
    bool ok = MySQLManager::getInstance().getOrderDetail(order_id, order);
    if (!ok) {
        redis.releaseLock(lock_key, owner_id);
        resp->set_error(static_cast<int>(ErrorCodes::RENTAL_ORDER_NOT_FOUND));
        resp->set_msg("Order not found");
        return Status::OK;
    }

    ok = MySQLManager::getInstance().cancelOrder(order_id, order.vehicle_id);
    redis.releaseLock(lock_key, owner_id);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RENTAL_ORDER_NOT_FOUND));
        resp->set_msg("Order not found or not in pending status");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("Order cancelled successfully");
    return Status::OK;
}

// ==================== 维保管理 ====================

Status VehicleGrpcServiceImpl::CreateMaintenance(ServerContext* context, const CreateMaintenanceRequest* req, CommonResponse* resp) {
    LOG_DEBUG("[Vehicle] CreateMaintenance vehicle_id={}", req->vehicle_id());

    // 分布式锁：锁定车辆，防止并发创建维保记录
    std::string lock_key = "lock:vehicle:" + std::to_string(req->vehicle_id());
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("System busy, please retry");
        return Status::OK;
    }

    int64_t id = MySQLManager::getInstance().createMaintenance(
        req->vehicle_id(), req->type(), req->description(),
        req->cost(), req->technician(), req->start_date());

    // 释放锁
    redis.releaseLock(lock_key, owner_id);

    if (id <= 0) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to create maintenance record");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("Maintenance record created successfully");
    resp->set_data(std::to_string(id));
    LOG_INFO("[Vehicle] CreateMaintenance success, id={}", id);
    return Status::OK;
}

Status VehicleGrpcServiceImpl::UpdateMaintenance(ServerContext* context, const UpdateMaintenanceRequest* req, CommonResponse* resp) {
    int64_t id = req->id();
    LOG_DEBUG("[Vehicle] UpdateMaintenance id={}", id);

    // 分布式锁：锁定维保记录，防止并发更新
    std::string lock_key = "lock:maintenance:" + std::to_string(id);
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("System busy, please retry");
        return Status::OK;
    }

    bool ok = MySQLManager::getInstance().updateMaintenance(
        id, req->description(), req->cost(),
        req->technician(), req->end_date(), req->status());

    // 释放锁
    redis.releaseLock(lock_key, owner_id);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Maintenance record not found or update failed");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("Maintenance record updated successfully");
    return Status::OK;
}

Status VehicleGrpcServiceImpl::GetMaintenanceList(ServerContext* context, const MaintenanceListRequest* req, MaintenanceListResponse* resp) {
    LOG_DEBUG("[Vehicle] GetMaintenanceList page={} page_size={}", req->page(), req->page_size());

    std::vector<MaintenanceData> records;
    int total = 0;
    bool ok = MySQLManager::getInstance().getMaintenanceList(
        req->page(), req->page_size(), req->vehicle_id(),
        req->status(), req->type(), records, total);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to fetch maintenance list");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_total(total);
    for (auto& m : records) {
        auto* item = resp->add_records();
        item->set_id(m.id);
        item->set_vehicle_id(m.vehicle_id);
        item->set_plate_number(m.plate_number);
        item->set_type(m.type);
        item->set_description(m.description);
        item->set_cost(m.cost);
        item->set_technician(m.technician);
        item->set_start_date(m.start_date);
        item->set_end_date(m.end_date);
        item->set_status(m.status);
        item->set_created_at(m.created_at);
    }
    return Status::OK;
}

Status VehicleGrpcServiceImpl::DeleteMaintenance(ServerContext* context, const VehicleDetailRequest* req, CommonResponse* resp) {
    int64_t id = req->id();
    LOG_INFO("[Vehicle] DeleteMaintenance id={}", id);

    bool ok = MySQLManager::getInstance().deleteMaintenance(id);
    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Maintenance record not found or delete failed");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("Maintenance record deleted successfully");
    return Status::OK;
}
