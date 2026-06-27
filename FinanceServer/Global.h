#ifndef GLOBAL_H
#define GLOBAL_H

#include <functional>
#include <memory>
#include <mutex>
#include <iostream>
#include <chrono>
#include <string>

#define CODE_PREFIX "code_"

struct PaymentData {
    int64_t id = 0;
    int64_t order_id = 0;
    std::string order_no;
    double amount = 0.0;
    std::string type;
    std::string method;
    std::string status;
    std::string remark;
    std::string paid_at;
    std::string created_at;
};

struct InvoiceData {
    int64_t id = 0;
    std::string invoice_no;
    int64_t order_id = 0;
    std::string order_no;
    int64_t user_id = 0;
    std::string username;
    double total_amount = 0.0;
    std::string items;
    std::string status;
    std::string generated_at;
};

struct RevenueStatsItemData {
    std::string date;
    double amount = 0.0;      // 净收入
    double rental = 0.0;      // 租赁收入
    double maintenance = 0.0; // 维保支出
    int count = 0;
};

struct VehicleStatsBrandData {
    std::string brand;
    int count = 0;
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
    PAYMENT_NOT_FOUND       = 1017,
    INVOICE_NOT_FOUND       = 1018,
    ORDER_NOT_FOUND         = 1019,
};

#endif /* GLOBAL_H */
