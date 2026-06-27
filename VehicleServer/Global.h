#ifndef GLOBAL_H
#define GLOBAL_H

#include <functional>
#include <memory>
#include <mutex>
#include <iostream>
#include <chrono>
#include <string>

#define CODE_PREFIX "code_"

struct VehicleData {
    int64_t id = 0;
    std::string plate_number;
    std::string brand;
    std::string model;
    std::string color;
    int year = 0;
    int mileage = 0;
    std::string status;
    double daily_rate = 0.0;
    double deposit_amount = 0.0;
    std::string image_url;
    std::string description;
    std::string created_at;
};

struct OrderData {
    int64_t id = 0;
    std::string order_no;
    int64_t user_id = 0;
    std::string username;
    int64_t vehicle_id = 0;
    std::string plate_number;
    std::string vehicle_info;
    std::string start_date;
    std::string end_date;
    std::string actual_return_date;
    std::string status;
    double deposit = 0.0;
    double daily_rate = 0.0;
    int total_days = 0;
    double total_cost = 0.0;
    double penalty = 0.0;
    std::string notes;
    std::string created_at;
};

struct MaintenanceData {
    int64_t id = 0;
    int64_t vehicle_id = 0;
    std::string plate_number;
    std::string type;
    std::string description;
    double cost = 0.0;
    std::string technician;
    std::string start_date;
    std::string end_date;
    std::string status;
    std::string created_at;
};

enum class ErrorCodes {
    SUCCESS = 0,

    // 通用
    JSON_PARSE_ERROR        = 1001,
    RPC_ERROR               = 1002,

    // 认证
    VERIFY_CODE_EXPIRED     = 1003,
    USER_ALREADY_EXISTS     = 1004,
    USER_DO_NOT_EXISTS      = 1005,
    USER_LOGIN_ERROR        = 1006,
    USER_ID_INVALID         = 1007,
    USER_NOT_APPROVED       = 1008,
    USER_ALREADY_APPROVED   = 1009,
    AUTH_TOKEN_INVALID      = 1010,
    AUTH_TOKEN_MISSING      = 1011,

    // 车辆
    VEHICLE_NOT_FOUND       = 1012,
    VEHICLE_UNAVAILABLE     = 1013,

    // 租赁
    RENTAL_ORDER_NOT_FOUND  = 1014,
    RENTAL_ORDER_ACTIVE     = 1015,

    // 支付
    PAYMENT_FAILED          = 1016,
};

#endif /* GLOBAL_H */
