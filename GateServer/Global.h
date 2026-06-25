#ifndef GLOBAL_H
#define GLOBAL_H

#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#define CODE_PREFIX "code_"
#define USER_TOKEN_PREFIX  "utoken_"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// ---------- Domain structs ----------

struct UserInfo {
    std::string username;
    std::string password;
    std::string email;
    std::string phone;
    std::string real_name;
    int uid;
    int role;       // 0=customer, 1=staff, 2=admin
    int status;     // 0=pending, 1=active, 2=banned
};

// ---------- Error codes ----------

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
};

// ---------- Network constants ----------

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
