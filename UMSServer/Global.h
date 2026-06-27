#ifndef GLOBAL_H
#define GLOBAL_H

#include <functional>
#include <memory>
#include <mutex>
#include <iostream>
#include <chrono>
#include <string>

#define CODE_PREFIX "code_"

struct UserInfo {
    std::string username;
    std::string password;
    std::string phone;
    std::string email;
    int64_t uid;
    int role;       // 0=客户, 1=员工, 2=管理员
    std::string status; // active=已激活, inactive=未激活, banned=已封禁
};

struct UserProfileData {
    int64_t uid = 0;
    std::string username;
    std::string phone;
    std::string email;
    std::string id_card;
    std::string driver_license;
    std::string real_name;
    int gender = 0;
    std::string birth_date;
    std::string address;
    std::string avatar_url;
    std::string role;
    std::string status;
};

struct BalanceRecordData {
    int64_t id = 0;
    int64_t user_id = 0;
    double amount = 0.0;
    std::string type;
    int64_t operator_id = 0;
    std::string remark;
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

    // 余额
    BALANCE_INSUFFICIENT    = 1017,
    BALANCE_TOPUP_FAILED    = 1018,
    BALANCE_CONSUME_FAILED  = 1019,
};

#endif /* GLOBAL_H */
