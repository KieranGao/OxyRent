#ifndef GLOBAL_H
#define GLOBAL_H

#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <json/json.h>
#include <string>

// UTF-8安全的JSON序列化（替代toStyledString，解决中文乱码问题）
inline std::string jsonToString(const Json::Value& value) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    builder["emitUTF8"] = true;
    return Json::writeString(builder, value);
}

inline std::string jsonToStringPretty(const Json::Value& value) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "    ";
    builder["emitUTF8"] = true;
    return Json::writeString(builder, value);
}

#define CODE_PREFIX "verify_code_"
#define USER_TOKEN_PREFIX  "utoken_"
#define USER_ROLE_PREFIX   "urole_"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// ---------- 领域数据结构 ----------

struct UserInfo {
    std::string username;
    std::string password;
    std::string email;
    std::string phone;
    std::string real_name;
    int uid;
    int role;       // 0=客户, 1=员工, 2=管理员
    int status;     // 0=待审核, 1=已激活, 2=已封禁
};

// ---------- 错误码 ----------

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

    // 发票
    INVOICE_NOT_FOUND       = 1018,

    // 余额
    BALANCE_INSUFFICIENT    = 1019,
    BALANCE_TOPUP_FAILED    = 1020,
    BALANCE_CONSUME_FAILED  = 1021,

    // 发票（续）
    ORDER_NOT_COMPLETED     = 1023,
};

// ---------- 网络常量 ----------

#define MAX_LENGTH  2048
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000

enum class MsgId : int {
    VEHICLE_LIST      = 2001,
    VEHICLE_LIST_RSP  = 2002,
    RENTAL_CREATE     = 2003,
    RENTAL_CREATE_RSP = 2004,
};

#endif /* GLOBAL_H */
