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
    int role;       // 0=customer, 1=staff, 2=admin
    std::string status; // active, inactive, banned
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

    // General
    JSON_PARSE_ERROR        = 1001,
    RPC_ERROR               = 1002,

    // Auth
    VERIFY_CODE_EXPIRED     = 1003,
    USER_ALREADY_EXISTS     = 1004,
    USER_DO_NOT_EXISTS      = 1005,
    USER_LOGIN_ERROR        = 1006,
    USER_ID_INVALID         = 1007,
    USER_NOT_APPROVED       = 1008,
    USER_ALREADY_APPROVED   = 1009,
    AUTH_TOKEN_INVALID      = 1010,
    AUTH_TOKEN_MISSING      = 1011,

    // Vehicle
    VEHICLE_NOT_FOUND       = 1012,
    VEHICLE_UNAVAILABLE     = 1013,

    // Rental
    RENTAL_ORDER_NOT_FOUND  = 1014,
    RENTAL_ORDER_ACTIVE     = 1015,

    // Payment
    PAYMENT_FAILED          = 1016,

    // Balance
    BALANCE_INSUFFICIENT    = 1017,
    BALANCE_TOPUP_FAILED    = 1018,
    BALANCE_CONSUME_FAILED  = 1019,
};

#endif /* GLOBAL_H */
