#include "LogicSystem.h"
#include "AsyncTaskPool.h"
#include "HttpConnection.h"
#include "UserGrpcClient.h"
#include "VehicleGrpcClient.h"
#include "FinanceGrpcClient.h"
#include "RedisManager.h"
#include "Logger.h"
#include <json/json.h>

void LogicSystem::registerGet(std::string url, HttpHandler handler) {
    getHandlers_[url] = handler;
}

void LogicSystem::registerPost(std::string url, HttpHandler handler) {
    postHandlers_[url] = handler;
}

void LogicSystem::registerPut(std::string url, HttpHandler handler) {
    putHandlers_[url] = handler;
}

LogicSystem::LogicSystem() {
    // ============ 公开路由（无需认证） ============

    registerPost("/user/register", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] USER_REGISTER: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        UserRegisterRequest request;
        request.set_username(jsonData["username"].asString());
        request.set_password(jsonData["password"].asString());
        request.set_phone(jsonData.get("phone", "").asString());
        request.set_email(jsonData.get("email", "").asString());

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, request]() {
            Json::Value result;
            try {
                CommonResponse rsp = UserGrpcClient::getInstance().userRegister(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            LOG_ERROR("[Gate] /user/register timed out after 5s");
        }
    });

    registerPost("/user/login", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] USER_LOGIN: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string username = jsonData["username"].asString();
        std::string password = jsonData["password"].asString();

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, username, password]() {
            Json::Value result;
            try {
                UserLoginRequest request;
                request.set_username(username);
                request.set_password(password);
                UserLoginResponse loginRsp = UserGrpcClient::getInstance().userLogin(request);
                result["error"] = loginRsp.error();
                if (loginRsp.error() != 0) {
                    result["msg"] = loginRsp.msg();
                } else {
                    result["uid"] = static_cast<Json::Int64>(loginRsp.uid());
                    result["token"] = loginRsp.token();
                    result["role"] = loginRsp.role();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                if (result["error"].asInt() == 0) {
                    // 将token存储到Redis
                    std::string uid_str = std::to_string(result["uid"].asInt64());
                    std::string token = result["token"].asString();
                    RedisManager::getInstance().setex(USER_TOKEN_PREFIX + uid_str, token, 86400);
                    // 将角色存储到Redis用于管理员权限验证
                    std::string role = result["role"].asString();
                    RedisManager::getInstance().setex(USER_ROLE_PREFIX + uid_str, role, 86400);
                }
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            LOG_ERROR("[Gate] /user/login timed out after 5s");
        }
    });

    // ============ 受保护路由（需要认证） ============

    registerGet("/user/profile", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        auto uid_it = connection->req_.find("X-User-Id");
        if (uid_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        int64_t uid = 0;
        try {
            uid = std::stoll(std::string(uid_it->value().data(), uid_it->value().size()));
        } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, uid]() {
            Json::Value result;
            try {
                UserProfileRequest request;
                request.set_uid(uid);
                UserProfileResponse rsp = UserGrpcClient::getInstance().getUserProfile(request);
                result["error"] = rsp.error();
                if (rsp.error() == 0) {
                    result["uid"] = static_cast<Json::Int64>(rsp.uid());
                    result["username"] = rsp.username();
                    result["phone"] = rsp.phone();
                    result["email"] = rsp.email();
                    result["id_card"] = rsp.id_card();
                    result["driver_license"] = rsp.driver_license();
                    result["real_name"] = rsp.real_name();
                    result["gender"] = rsp.gender();
                    result["birth_date"] = rsp.birth_date();
                    result["address"] = rsp.address();
                    result["avatar_url"] = rsp.avatar_url();
                    result["role"] = rsp.role();
                    result["status"] = rsp.status();
                } else {
                    result["msg"] = rsp.msg();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPut("/user/profile", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] UPDATE_PROFILE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto uid_it = connection->req_.find("X-User-Id");
        if (uid_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        int64_t uid = 0;
        try {
            uid = std::stoll(std::string(uid_it->value().data(), uid_it->value().size()));
        } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData, uid]() {
            Json::Value result;
            try {
                UpdateProfileRequest request;
                request.set_uid(uid);
                request.set_phone(jsonData.get("phone", "").asString());
                request.set_email(jsonData.get("email", "").asString());
                request.set_id_card(jsonData.get("id_card", "").asString());
                request.set_driver_license(jsonData.get("driver_license", "").asString());
                request.set_real_name(jsonData.get("real_name", "").asString());
                request.set_gender(jsonData.get("gender", 0).asInt());
                request.set_birth_date(jsonData.get("birth_date", "").asString());
                request.set_address(jsonData.get("address", "").asString());
                request.set_avatar_url(jsonData.get("avatar_url", "").asString());
                CommonResponse rsp = UserGrpcClient::getInstance().updateProfile(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/user/list", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        // 提取查询参数
        std::string keyword = connection->get_params_.count("keyword") ? connection->get_params_["keyword"] : "";
        std::string role = connection->get_params_.count("role") ? connection->get_params_["role"] : "";
        std::string status = connection->get_params_.count("status") ? connection->get_params_["status"] : "";
        int page = 1;
        int page_size = 20;
        if (connection->get_params_.count("page")) {
            try { page = std::stoi(connection->get_params_["page"]); } catch (...) {}
        }
        if (connection->get_params_.count("page_size")) {
            try { page_size = std::stoi(connection->get_params_["page_size"]); } catch (...) {}
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, page, page_size, keyword, role, status]() {
            Json::Value result;
            try {
                UserListRequest request;
                request.set_page(page);
                request.set_page_size(page_size);
                request.set_keyword(keyword);
                request.set_role(role);
                request.set_status(status);
                UserListResponse rsp = UserGrpcClient::getInstance().getUserList(request);
                result["error"] = rsp.error();
                result["total"] = rsp.total();
                Json::Value users(Json::arrayValue);
                for (int i = 0; i < rsp.users_size(); ++i) {
                    auto& u = rsp.users(i);
                    Json::Value user;
                    user["uid"] = static_cast<Json::Int64>(u.uid());
                    user["username"] = u.username();
                    user["phone"] = u.phone();
                    user["email"] = u.email();
                    user["real_name"] = u.real_name();
                    user["role"] = u.role();
                    user["status"] = u.status();
                    user["created_at"] = u.created_at();
                    users.append(user);
                }
                result["users"] = users;
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status_f = f.wait_for(std::chrono::seconds(5));
        if (status_f == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // PUT /user/{id}/status - 仅管理员
    registerPut("/user/status", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] UPDATE_USER_STATUS: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 检查调用者角色 - 仅管理员
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        int64_t uid = 0;
        try {
            uid = jsonData["uid"].asInt64();
        } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string new_status = jsonData["status"].asString();

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, uid, new_status]() {
            Json::Value result;
            try {
                UpdateUserStatusRequest request;
                request.set_uid(uid);
                request.set_status(new_status);
                CommonResponse rsp = UserGrpcClient::getInstance().updateUserStatus(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // PUT /user/{id}/role - 仅管理员
    registerPut("/user/role", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] UPDATE_USER_ROLE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 检查调用者角色 - 仅管理员
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        int64_t uid = 0;
        try {
            uid = jsonData["uid"].asInt64();
        } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string role = jsonData["role"].asString();

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, uid, role]() {
            Json::Value result;
            try {
                UpdateUserRoleRequest request;
                request.set_uid(uid);
                request.set_role(role);
                CommonResponse rsp = UserGrpcClient::getInstance().updateUserRole(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // ============ 余额路由 ============

    // GET /user/balance - 获取当前用户余额（任何登录用户）
    registerGet("/user/balance", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        auto uid_it = connection->req_.find("X-User-Id");
        if (uid_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        int64_t uid = 0;
        try {
            uid = std::stoll(std::string(uid_it->value().data(), uid_it->value().size()));
        } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, uid]() {
            Json::Value result;
            try {
                GetBalanceRequest request;
                request.set_uid(uid);
                GetBalanceResponse rsp = UserGrpcClient::getInstance().getBalance(request);
                result["error"] = rsp.error();
                result["balance"] = rsp.balance();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // POST /user/topup - 充值余额（仅管理员/员工）
    registerPost("/user/topup", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] TOPUP_BALANCE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 检查调用者角色 - 仅管理员/员工
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin" && caller_role != "1" && caller_role != "staff")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Staff access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        int64_t uid = jsonData["uid"].asInt64();
        double amount = jsonData["amount"].asDouble();
        std::string remark = jsonData.get("remark", "").asString();
        int64_t caller_uid = std::stoll(caller_uid_str);

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, uid, amount, remark, caller_uid]() {
            Json::Value result;
            try {
                TopupRequest request;
                request.set_uid(uid);
                request.set_amount(amount);
                request.set_remark(remark);
                CommonResponse rsp = UserGrpcClient::getInstance().topupBalance(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // GET /user/balance/records - 获取余额记录
    registerGet("/user/balance/records", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        auto uid_it = connection->req_.find("X-User-Id");
        if (uid_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        int64_t uid = 0;
        try {
            uid = std::stoll(std::string(uid_it->value().data(), uid_it->value().size()));
        } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        int page = 1, page_size = 20;
        if (connection->get_params_.count("page")) {
            try { page = std::stoi(connection->get_params_["page"]); } catch (...) {}
        }
        if (connection->get_params_.count("page_size")) {
            try { page_size = std::stoi(connection->get_params_["page_size"]); } catch (...) {}
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, uid, page, page_size]() {
            Json::Value result;
            try {
                BalanceRecordListRequest request;
                request.set_uid(uid);
                request.set_page(page);
                request.set_page_size(page_size);
                BalanceRecordListResponse rsp = UserGrpcClient::getInstance().getBalanceRecords(request);
                result["error"] = rsp.error();
                result["total"] = rsp.total();
                result["balance"] = rsp.balance();
                Json::Value records(Json::arrayValue);
                for (int i = 0; i < rsp.records_size(); ++i) {
                    auto& r = rsp.records(i);
                    Json::Value record;
                    record["id"] = static_cast<Json::Int64>(r.id());
                    record["user_id"] = static_cast<Json::Int64>(r.user_id());
                    record["amount"] = r.amount();
                    record["type"] = r.type();
                    record["operator_id"] = static_cast<Json::Int64>(r.operator_id());
                    record["remark"] = r.remark();
                    record["created_at"] = r.created_at();
                    records.append(record);
                }
                result["records"] = records;
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // ============ 车辆路由 ============

    registerGet("/vehicle/list", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        std::string keyword = connection->get_params_.count("keyword") ? connection->get_params_["keyword"] : "";
        std::string status = connection->get_params_.count("status") ? connection->get_params_["status"] : "";
        std::string brand = connection->get_params_.count("brand") ? connection->get_params_["brand"] : "";
        int page = 1, page_size = 20;
        if (connection->get_params_.count("page")) {
            try { page = std::stoi(connection->get_params_["page"]); } catch (...) {}
        }
        if (connection->get_params_.count("page_size")) {
            try { page_size = std::stoi(connection->get_params_["page_size"]); } catch (...) {}
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, page, page_size, keyword, status, brand]() {
            Json::Value result;
            try {
                VehicleListRequest request;
                request.set_page(page);
                request.set_page_size(page_size);
                request.set_keyword(keyword);
                request.set_status(status);
                request.set_brand(brand);
                VehicleListResponse rsp = VehicleGrpcClient::getInstance().getVehicleList(request);
                result["error"] = rsp.error();
                result["total"] = rsp.total();
                Json::Value vehicles(Json::arrayValue);
                for (int i = 0; i < rsp.vehicles_size(); ++i) {
                    auto& v = rsp.vehicles(i);
                    Json::Value vehicle;
                    vehicle["id"] = static_cast<Json::Int64>(v.id());
                    vehicle["plate_number"] = v.plate_number();
                    vehicle["brand"] = v.brand();
                    vehicle["model"] = v.model();
                    vehicle["color"] = v.color();
                    vehicle["year"] = v.year();
                    vehicle["mileage"] = v.mileage();
                    vehicle["status"] = v.status();
                    vehicle["daily_rate"] = v.daily_rate();
                    vehicle["deposit_amount"] = v.deposit_amount();
                    vehicle["image_url"] = v.image_url();
                    vehicle["description"] = v.description();
                    vehicle["created_at"] = v.created_at();
                    vehicles.append(vehicle);
                }
                result["vehicles"] = vehicles;
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status_f = f.wait_for(std::chrono::seconds(5));
        if (status_f == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/vehicle/detail", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        if (!connection->get_params_.count("id")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            jsonResp["msg"] = "Missing id parameter";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        int64_t id = 0;
        try { id = std::stoll(connection->get_params_["id"]); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, id]() {
            Json::Value result;
            try {
                VehicleDetailRequest request;
                request.set_id(id);
                VehicleInfo rsp = VehicleGrpcClient::getInstance().getVehicleDetail(request);
                if (rsp.id() == 0) {
                    result["error"] = static_cast<int>(ErrorCodes::VEHICLE_NOT_FOUND);
                    result["msg"] = "Vehicle not found";
                } else {
                    result["error"] = 0;
                    result["id"] = static_cast<Json::Int64>(rsp.id());
                    result["plate_number"] = rsp.plate_number();
                    result["brand"] = rsp.brand();
                    result["model"] = rsp.model();
                    result["color"] = rsp.color();
                    result["year"] = rsp.year();
                    result["mileage"] = rsp.mileage();
                    result["status"] = rsp.status();
                    result["daily_rate"] = rsp.daily_rate();
                    result["deposit_amount"] = rsp.deposit_amount();
                    result["image_url"] = rsp.image_url();
                    result["description"] = rsp.description();
                    result["created_at"] = rsp.created_at();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status_f = f.wait_for(std::chrono::seconds(5));
        if (status_f == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPost("/vehicle", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] ADD_VEHICLE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData]() {
            Json::Value result;
            try {
                AddVehicleRequest request;
                request.set_plate_number(jsonData["plate_number"].asString());
                request.set_brand(jsonData["brand"].asString());
                request.set_model(jsonData["model"].asString());
                request.set_color(jsonData["color"].asString());
                request.set_year(jsonData.get("year", 0).asInt());
                request.set_mileage(jsonData.get("mileage", 0).asInt());
                request.set_daily_rate(jsonData.get("daily_rate", 0.0).asDouble());
                request.set_deposit_amount(jsonData.get("deposit_amount", 0.0).asDouble());
                request.set_image_url(jsonData.get("image_url", "").asString());
                request.set_description(jsonData.get("description", "").asString());
                CommonResponse rsp = VehicleGrpcClient::getInstance().addVehicle(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
                if (rsp.error() == 0 && !rsp.data().empty()) {
                    result["id"] = static_cast<Json::Int64>(std::stoll(rsp.data()));
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPut("/vehicle", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] UPDATE_VEHICLE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        int64_t id = 0;
        try { id = jsonData["id"].asInt64(); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData, id]() {
            Json::Value result;
            try {
                UpdateVehicleRequest request;
                request.set_id(id);
                request.set_plate_number(jsonData.get("plate_number", "").asString());
                request.set_brand(jsonData.get("brand", "").asString());
                request.set_model(jsonData.get("model", "").asString());
                request.set_color(jsonData.get("color", "").asString());
                request.set_year(jsonData.get("year", 0).asInt());
                request.set_mileage(jsonData.get("mileage", 0).asInt());
                request.set_status(jsonData.get("status", "").asString());
                request.set_daily_rate(jsonData.get("daily_rate", 0.0).asDouble());
                request.set_deposit_amount(jsonData.get("deposit_amount", 0.0).asDouble());
                request.set_image_url(jsonData.get("image_url", "").asString());
                request.set_description(jsonData.get("description", "").asString());
                CommonResponse rsp = VehicleGrpcClient::getInstance().updateVehicle(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPost("/vehicle/delete", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] DELETE_VEHICLE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        int64_t id = 0;
        try { id = jsonData["id"].asInt64(); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, id]() {
            Json::Value result;
            try {
                VehicleDetailRequest request;
                request.set_id(id);
                CommonResponse rsp = VehicleGrpcClient::getInstance().deleteVehicle(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // ============ 租赁路由 ============

    registerPost("/rental/create", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] CREATE_ORDER: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto uid_it = connection->req_.find("X-User-Id");
        if (uid_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        int64_t uid = 0;
        try { uid = std::stoll(std::string(uid_it->value().data(), uid_it->value().size())); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData, uid]() {
            Json::Value result;
            try {
                CreateOrderRequest request;
                request.set_user_id(uid);
                request.set_vehicle_id(jsonData["vehicle_id"].asInt64());
                request.set_start_date(jsonData["start_date"].asString());
                request.set_end_date(jsonData["end_date"].asString());
                request.set_notes(jsonData.get("notes", "").asString());
                OrderInfo rsp = VehicleGrpcClient::getInstance().createOrder(request);
                if (rsp.id() == 0) {
                    result["error"] = static_cast<int>(ErrorCodes::VEHICLE_UNAVAILABLE);
                    result["msg"] = "Vehicle not available or order creation failed";
                } else {
                    result["error"] = 0;
                    result["id"] = static_cast<Json::Int64>(rsp.id());
                    result["order_no"] = rsp.order_no();
                    result["user_id"] = static_cast<Json::Int64>(rsp.user_id());
                    result["vehicle_id"] = static_cast<Json::Int64>(rsp.vehicle_id());
                    result["plate_number"] = rsp.plate_number();
                    result["start_date"] = rsp.start_date();
                    result["end_date"] = rsp.end_date();
                    result["status"] = rsp.status();
                    result["deposit"] = rsp.deposit();
                    result["daily_rate"] = rsp.daily_rate();
                    result["total_days"] = rsp.total_days();
                    result["total_cost"] = rsp.total_cost();
                    result["notes"] = rsp.notes();
                    result["created_at"] = rsp.created_at();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/rental/list", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        std::string status_str = connection->get_params_.count("status") ? connection->get_params_["status"] : "";
        std::string keyword = connection->get_params_.count("keyword") ? connection->get_params_["keyword"] : "";
        int page = 1, page_size = 20;
        int64_t user_id = 0, vehicle_id = 0;
        if (connection->get_params_.count("page")) {
            try { page = std::stoi(connection->get_params_["page"]); } catch (...) {}
        }
        if (connection->get_params_.count("page_size")) {
            try { page_size = std::stoi(connection->get_params_["page_size"]); } catch (...) {}
        }
        if (connection->get_params_.count("user_id")) {
            try { user_id = std::stoll(connection->get_params_["user_id"]); } catch (...) {}
        }
        if (connection->get_params_.count("vehicle_id")) {
            try { vehicle_id = std::stoll(connection->get_params_["vehicle_id"]); } catch (...) {}
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, page, page_size, user_id, vehicle_id, status_str, keyword]() {
            Json::Value result;
            try {
                OrderListRequest request;
                request.set_page(page);
                request.set_page_size(page_size);
                request.set_user_id(user_id);
                request.set_vehicle_id(vehicle_id);
                request.set_status(status_str);
                request.set_keyword(keyword);
                OrderListResponse rsp = VehicleGrpcClient::getInstance().getOrderList(request);
                result["error"] = rsp.error();
                result["total"] = rsp.total();
                Json::Value orders(Json::arrayValue);
                for (int i = 0; i < rsp.orders_size(); ++i) {
                    auto& o = rsp.orders(i);
                    Json::Value order;
                    order["id"] = static_cast<Json::Int64>(o.id());
                    order["order_no"] = o.order_no();
                    order["user_id"] = static_cast<Json::Int64>(o.user_id());
                    order["username"] = o.username();
                    order["vehicle_id"] = static_cast<Json::Int64>(o.vehicle_id());
                    order["plate_number"] = o.plate_number();
                    order["vehicle_info"] = o.vehicle_info();
                    order["start_date"] = o.start_date();
                    order["end_date"] = o.end_date();
                    order["actual_return_date"] = o.actual_return_date();
                    order["status"] = o.status();
                    order["deposit"] = o.deposit();
                    order["daily_rate"] = o.daily_rate();
                    order["total_days"] = o.total_days();
                    order["total_cost"] = o.total_cost();
                    order["penalty"] = o.penalty();
                    order["notes"] = o.notes();
                    order["created_at"] = o.created_at();
                    orders.append(order);
                }
                result["orders"] = orders;
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status_f = f.wait_for(std::chrono::seconds(5));
        if (status_f == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/rental/detail", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        if (!connection->get_params_.count("id")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            jsonResp["msg"] = "Missing id parameter";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        int64_t id = 0;
        try { id = std::stoll(connection->get_params_["id"]); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, id]() {
            Json::Value result;
            try {
                OrderDetailRequest request;
                request.set_id(id);
                OrderInfo rsp = VehicleGrpcClient::getInstance().getOrderDetail(request);
                if (rsp.id() == 0) {
                    result["error"] = static_cast<int>(ErrorCodes::RENTAL_ORDER_NOT_FOUND);
                    result["msg"] = "Order not found";
                } else {
                    result["error"] = 0;
                    result["id"] = static_cast<Json::Int64>(rsp.id());
                    result["order_no"] = rsp.order_no();
                    result["user_id"] = static_cast<Json::Int64>(rsp.user_id());
                    result["username"] = rsp.username();
                    result["vehicle_id"] = static_cast<Json::Int64>(rsp.vehicle_id());
                    result["plate_number"] = rsp.plate_number();
                    result["vehicle_info"] = rsp.vehicle_info();
                    result["start_date"] = rsp.start_date();
                    result["end_date"] = rsp.end_date();
                    result["actual_return_date"] = rsp.actual_return_date();
                    result["status"] = rsp.status();
                    result["deposit"] = rsp.deposit();
                    result["daily_rate"] = rsp.daily_rate();
                    result["total_days"] = rsp.total_days();
                    result["total_cost"] = rsp.total_cost();
                    result["penalty"] = rsp.penalty();
                    result["notes"] = rsp.notes();
                    result["created_at"] = rsp.created_at();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status_f = f.wait_for(std::chrono::seconds(5));
        if (status_f == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPost("/rental/pickup", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] PICKUP_VEHICLE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData]() {
            Json::Value result;
            try {
                PickupRequest request;
                request.set_order_id(jsonData["order_id"].asInt64());
                CommonResponse rsp = VehicleGrpcClient::getInstance().pickupVehicle(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPost("/rental/return", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] RETURN_VEHICLE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData]() {
            Json::Value result;
            try {
                ReturnRequest request;
                request.set_order_id(jsonData["order_id"].asInt64());
                OrderInfo rsp = VehicleGrpcClient::getInstance().returnVehicle(request);
                if (rsp.id() == 0) {
                    result["error"] = static_cast<int>(ErrorCodes::RENTAL_ORDER_NOT_FOUND);
                    result["msg"] = "Return failed";
                } else {
                    result["error"] = 0;
                    result["id"] = static_cast<Json::Int64>(rsp.id());
                    result["order_no"] = rsp.order_no();
                    result["status"] = rsp.status();
                    result["actual_return_date"] = rsp.actual_return_date();
                    result["total_cost"] = rsp.total_cost();
                    result["penalty"] = rsp.penalty();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPost("/rental/cancel", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] CANCEL_ORDER: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData]() {
            Json::Value result;
            try {
                PickupRequest request;
                request.set_order_id(jsonData["order_id"].asInt64());
                CommonResponse rsp = VehicleGrpcClient::getInstance().cancelOrder(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPost("/rental/renew", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] RENEW_ORDER: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData]() {
            Json::Value result;
            try {
                RenewRequest request;
                request.set_order_id(jsonData["order_id"].asInt64());
                request.set_new_end_date(jsonData["new_end_date"].asString());
                OrderInfo rsp = VehicleGrpcClient::getInstance().renewOrder(request);
                if (rsp.id() == 0) {
                    result["error"] = static_cast<int>(ErrorCodes::RENTAL_ORDER_NOT_FOUND);
                    result["msg"] = "Renewal failed";
                } else {
                    result["error"] = 0;
                    result["id"] = static_cast<Json::Int64>(rsp.id());
                    result["order_no"] = rsp.order_no();
                    result["end_date"] = rsp.end_date();
                    result["total_days"] = rsp.total_days();
                    result["total_cost"] = rsp.total_cost();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // ============ 维保路由 ============

    registerPost("/maintenance/create", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] CREATE_MAINTENANCE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData]() {
            Json::Value result;
            try {
                CreateMaintenanceRequest request;
                request.set_vehicle_id(jsonData["vehicle_id"].asInt64());
                request.set_type(jsonData["type"].asString());
                request.set_description(jsonData.get("description", "").asString());
                request.set_cost(jsonData.get("cost", 0.0).asDouble());
                request.set_technician(jsonData.get("technician", "").asString());
                request.set_start_date(jsonData["start_date"].asString());
                CommonResponse rsp = VehicleGrpcClient::getInstance().createMaintenance(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
                if (rsp.error() == 0 && !rsp.data().empty()) {
                    result["id"] = static_cast<Json::Int64>(std::stoll(rsp.data()));
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPut("/maintenance", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] UPDATE_MAINTENANCE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData]() {
            Json::Value result;
            try {
                UpdateMaintenanceRequest request;
                request.set_id(jsonData["id"].asInt64());
                request.set_description(jsonData.get("description", "").asString());
                request.set_cost(jsonData.get("cost", 0.0).asDouble());
                request.set_technician(jsonData.get("technician", "").asString());
                request.set_end_date(jsonData.get("end_date", "").asString());
                request.set_status(jsonData.get("status", "").asString());
                CommonResponse rsp = VehicleGrpcClient::getInstance().updateMaintenance(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/maintenance/list", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        std::string status_str = connection->get_params_.count("status") ? connection->get_params_["status"] : "";
        std::string type = connection->get_params_.count("type") ? connection->get_params_["type"] : "";
        int page = 1, page_size = 20;
        int64_t vehicle_id = 0;
        if (connection->get_params_.count("page")) {
            try { page = std::stoi(connection->get_params_["page"]); } catch (...) {}
        }
        if (connection->get_params_.count("page_size")) {
            try { page_size = std::stoi(connection->get_params_["page_size"]); } catch (...) {}
        }
        if (connection->get_params_.count("vehicle_id")) {
            try { vehicle_id = std::stoll(connection->get_params_["vehicle_id"]); } catch (...) {}
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, page, page_size, vehicle_id, status_str, type]() {
            Json::Value result;
            try {
                MaintenanceListRequest request;
                request.set_page(page);
                request.set_page_size(page_size);
                request.set_vehicle_id(vehicle_id);
                request.set_status(status_str);
                request.set_type(type);
                MaintenanceListResponse rsp = VehicleGrpcClient::getInstance().getMaintenanceList(request);
                result["error"] = rsp.error();
                result["total"] = rsp.total();
                Json::Value records(Json::arrayValue);
                for (int i = 0; i < rsp.records_size(); ++i) {
                    auto& m = rsp.records(i);
                    Json::Value record;
                    record["id"] = static_cast<Json::Int64>(m.id());
                    record["vehicle_id"] = static_cast<Json::Int64>(m.vehicle_id());
                    record["plate_number"] = m.plate_number();
                    record["type"] = m.type();
                    record["description"] = m.description();
                    record["cost"] = m.cost();
                    record["technician"] = m.technician();
                    record["start_date"] = m.start_date();
                    record["end_date"] = m.end_date();
                    record["status"] = m.status();
                    record["created_at"] = m.created_at();
                    records.append(record);
                }
                result["records"] = records;
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status_f = f.wait_for(std::chrono::seconds(5));
        if (status_f == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPost("/maintenance/delete", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] DELETE_MAINTENANCE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 管理员/员工验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin" && caller_role != "1" && caller_role != "staff")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Staff access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        int64_t id = 0;
        try { id = jsonData["id"].asInt64(); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, id]() {
            Json::Value result;
            try {
                VehicleDetailRequest request;
                request.set_id(id);
                CommonResponse rsp = VehicleGrpcClient::getInstance().deleteMaintenance(request);
                result["error"] = rsp.error();
                result["msg"] = rsp.msg();
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // ============ 财务路由 ============

    registerPost("/payment/create", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] CREATE_PAYMENT: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData]() {
            Json::Value result;
            try {
                CreatePaymentRequest request;
                request.set_order_id(jsonData["order_id"].asInt64());
                request.set_amount(jsonData["amount"].asDouble());
                request.set_type(jsonData["type"].asString());
                request.set_method(jsonData["method"].asString());
                request.set_remark(jsonData.get("remark", "").asString());
                PaymentInfo rsp = FinanceGrpcClient::getInstance().createPayment(request);
                if (rsp.id() == 0) {
                    result["error"] = static_cast<int>(ErrorCodes::PAYMENT_FAILED);
                    result["msg"] = "Payment creation failed";
                } else {
                    result["error"] = 0;
                    result["id"] = static_cast<Json::Int64>(rsp.id());
                    result["order_id"] = static_cast<Json::Int64>(rsp.order_id());
                    result["order_no"] = rsp.order_no();
                    result["amount"] = rsp.amount();
                    result["type"] = rsp.type();
                    result["method"] = rsp.method();
                    result["status"] = rsp.status();
                    result["remark"] = rsp.remark();
                    result["created_at"] = rsp.created_at();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // POST /payment/confirm - 确认支付（管理员/员工）
    registerPost("/payment/confirm", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] CONFIRM_PAYMENT: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 管理员/员工验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin" && caller_role != "1" && caller_role != "staff")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Staff access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        int64_t id = 0;
        try { id = jsonData["id"].asInt64(); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, id]() {
            Json::Value result;
            try {
                PaymentInfo request;
                request.set_id(id);
                PaymentInfo rsp = FinanceGrpcClient::getInstance().confirmPayment(request);
                if (rsp.id() == 0) {
                    result["error"] = static_cast<int>(ErrorCodes::PAYMENT_FAILED);
                    result["msg"] = "Payment not found or not in pending status";
                } else {
                    result["error"] = 0;
                    result["id"] = static_cast<Json::Int64>(rsp.id());
                    result["order_id"] = static_cast<Json::Int64>(rsp.order_id());
                    result["order_no"] = rsp.order_no();
                    result["amount"] = rsp.amount();
                    result["type"] = rsp.type();
                    result["method"] = rsp.method();
                    result["status"] = rsp.status();
                    result["remark"] = rsp.remark();
                    result["paid_at"] = rsp.paid_at();
                    result["created_at"] = rsp.created_at();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/payment/list", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        int page = 1, page_size = 20;
        int64_t order_id = 0;
        std::string status_str = connection->get_params_.count("status") ? connection->get_params_["status"] : "";
        std::string type = connection->get_params_.count("type") ? connection->get_params_["type"] : "";
        if (connection->get_params_.count("page")) {
            try { page = std::stoi(connection->get_params_["page"]); } catch (...) {}
        }
        if (connection->get_params_.count("page_size")) {
            try { page_size = std::stoi(connection->get_params_["page_size"]); } catch (...) {}
        }
        if (connection->get_params_.count("order_id")) {
            try { order_id = std::stoll(connection->get_params_["order_id"]); } catch (...) {}
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, page, page_size, order_id, status_str, type]() {
            Json::Value result;
            try {
                PaymentListRequest request;
                request.set_page(page);
                request.set_page_size(page_size);
                request.set_order_id(order_id);
                request.set_status(status_str);
                request.set_type(type);
                PaymentListResponse rsp = FinanceGrpcClient::getInstance().getPaymentList(request);
                result["error"] = rsp.error();
                result["total"] = rsp.total();
                Json::Value payments(Json::arrayValue);
                for (int i = 0; i < rsp.payments_size(); ++i) {
                    auto& pay = rsp.payments(i);
                    Json::Value payment;
                    payment["id"] = static_cast<Json::Int64>(pay.id());
                    payment["order_id"] = static_cast<Json::Int64>(pay.order_id());
                    payment["order_no"] = pay.order_no();
                    payment["amount"] = pay.amount();
                    payment["type"] = pay.type();
                    payment["method"] = pay.method();
                    payment["status"] = pay.status();
                    payment["remark"] = pay.remark();
                    payment["paid_at"] = pay.paid_at();
                    payment["created_at"] = pay.created_at();
                    payments.append(payment);
                }
                result["payments"] = payments;
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status_f = f.wait_for(std::chrono::seconds(5));
        if (status_f == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/payment/detail", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        if (!connection->get_params_.count("id")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            jsonResp["msg"] = "Missing id parameter";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        int64_t id = 0;
        try { id = std::stoll(connection->get_params_["id"]); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, id]() {
            Json::Value result;
            try {
                PaymentInfo request;
                request.set_id(id);
                PaymentInfo rsp = FinanceGrpcClient::getInstance().getPaymentDetail(request);
                if (rsp.id() == 0) {
                    result["error"] = static_cast<int>(ErrorCodes::PAYMENT_NOT_FOUND);
                    result["msg"] = "Payment not found";
                } else {
                    result["error"] = 0;
                    result["id"] = static_cast<Json::Int64>(rsp.id());
                    result["order_id"] = static_cast<Json::Int64>(rsp.order_id());
                    result["order_no"] = rsp.order_no();
                    result["amount"] = rsp.amount();
                    result["type"] = rsp.type();
                    result["method"] = rsp.method();
                    result["status"] = rsp.status();
                    result["remark"] = rsp.remark();
                    result["paid_at"] = rsp.paid_at();
                    result["created_at"] = rsp.created_at();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status_f = f.wait_for(std::chrono::seconds(5));
        if (status_f == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerPost("/invoice/generate", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] GENERATE_INVOICE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, jsonData]() {
            Json::Value result;
            try {
                GenerateInvoiceRequest request;
                request.set_order_id(jsonData["order_id"].asInt64());
                InvoiceInfo rsp = FinanceGrpcClient::getInstance().generateInvoice(request);
                if (rsp.id() == -2) {
                    result["error"] = static_cast<int>(ErrorCodes::ORDER_NOT_COMPLETED);
                    result["msg"] = "只能为已完成的订单生成账单";
                } else if (rsp.id() == 0) {
                    result["error"] = static_cast<int>(ErrorCodes::INVOICE_NOT_FOUND);
                    result["msg"] = "Invoice generation failed";
                } else {
                    result["error"] = 0;
                    result["id"] = static_cast<Json::Int64>(rsp.id());
                    result["invoice_no"] = rsp.invoice_no();
                    result["order_id"] = static_cast<Json::Int64>(rsp.order_id());
                    result["order_no"] = rsp.order_no();
                    result["user_id"] = static_cast<Json::Int64>(rsp.user_id());
                    result["username"] = rsp.username();
                    result["total_amount"] = rsp.total_amount();
                    result["items"] = rsp.items();
                    result["generated_at"] = rsp.generated_at();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/invoice/detail", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        if (!connection->get_params_.count("id")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            jsonResp["msg"] = "Missing id parameter";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        int64_t id = 0;
        try { id = std::stoll(connection->get_params_["id"]); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, id]() {
            Json::Value result;
            try {
                InvoiceInfo request;
                request.set_id(id);
                InvoiceInfo rsp = FinanceGrpcClient::getInstance().getInvoiceDetail(request);
                if (rsp.id() == 0) {
                    result["error"] = static_cast<int>(ErrorCodes::INVOICE_NOT_FOUND);
                    result["msg"] = "Invoice not found";
                } else {
                    result["error"] = 0;
                    result["id"] = static_cast<Json::Int64>(rsp.id());
                    result["invoice_no"] = rsp.invoice_no();
                    result["order_id"] = static_cast<Json::Int64>(rsp.order_id());
                    result["order_no"] = rsp.order_no();
                    result["user_id"] = static_cast<Json::Int64>(rsp.user_id());
                    result["username"] = rsp.username();
                    result["total_amount"] = rsp.total_amount();
                    result["items"] = rsp.items();
                    result["generated_at"] = rsp.generated_at();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status_f = f.wait_for(std::chrono::seconds(5));
        if (status_f == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/invoice/list", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "需要管理员权限";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        int page = 1, page_size = 20;
        if (connection->get_params_.count("page")) {
            try { page = std::stoi(connection->get_params_["page"]); } catch (...) {}
        }
        if (connection->get_params_.count("page_size")) {
            try { page_size = std::stoi(connection->get_params_["page_size"]); } catch (...) {}
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, page, page_size]() {
            Json::Value result;
            try {
                InvoiceListRequest request;
                request.set_page(page);
                request.set_page_size(page_size);
                InvoiceListResponse rsp = FinanceGrpcClient::getInstance().getInvoiceList(request);
                result["error"] = rsp.error();
                result["total"] = rsp.total();
                Json::Value invoices(Json::arrayValue);
                for (int i = 0; i < rsp.invoices_size(); ++i) {
                    auto& inv = rsp.invoices(i);
                    Json::Value item;
                    item["id"] = static_cast<Json::Int64>(inv.id());
                    item["invoice_no"] = inv.invoice_no();
                    item["order_id"] = static_cast<Json::Int64>(inv.order_id());
                    item["order_no"] = inv.order_no();
                    item["username"] = inv.username();
                    item["total_amount"] = inv.total_amount();
                    item["items"] = inv.items();
                    item["generated_at"] = inv.generated_at();
                    invoices.append(item);
                }
                result["invoices"] = invoices;
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status_f = f.wait_for(std::chrono::seconds(5));
        if (status_f == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    // ============ 统计路由 ============

    registerGet("/stats/overview", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p]() {
            Json::Value result;
            try {
                CommonResponse request;
                StatsOverviewResponse rsp = FinanceGrpcClient::getInstance().getStatsOverview(request);
                result["error"] = rsp.error();
                if (rsp.error() == 0) {
                    result["total_users"] = rsp.total_users();
                    result["total_vehicles"] = rsp.total_vehicles();
                    result["available_vehicles"] = rsp.available_vehicles();
                    result["active_orders"] = rsp.active_orders();
                    result["completed_orders"] = rsp.completed_orders();
                    result["total_revenue"] = rsp.total_revenue();
                    result["month_revenue"] = rsp.month_revenue();
                } else {
                    result["msg"] = rsp.msg();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/stats/revenue", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        std::string start_date = connection->get_params_.count("start_date") ? connection->get_params_["start_date"] : "";
        std::string end_date = connection->get_params_.count("end_date") ? connection->get_params_["end_date"] : "";
        std::string granularity = connection->get_params_.count("granularity") ? connection->get_params_["granularity"] : "daily";

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p, start_date, end_date, granularity]() {
            Json::Value result;
            try {
                RevenueStatsRequest request;
                request.set_start_date(start_date);
                request.set_end_date(end_date);
                request.set_granularity(granularity);
                RevenueStatsResponse rsp = FinanceGrpcClient::getInstance().getRevenueStats(request);
                result["error"] = rsp.error();
                result["total"] = rsp.total();
                Json::Value items(Json::arrayValue);
                for (int i = 0; i < rsp.items_size(); ++i) {
                    auto& item = rsp.items(i);
                    Json::Value stat;
                    stat["date"] = item.date();
                    stat["amount"] = item.amount();
                    stat["count"] = item.count();
                    items.append(stat);
                }
                result["items"] = items;
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });

    registerGet("/stats/vehicles", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        // 管理员验证
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            return;
        }

        auto p = std::make_shared<std::promise<Json::Value>>();
        auto f = p->get_future();
        AsyncTaskPool::getInstance().post([p]() {
            Json::Value result;
            try {
                CommonResponse request;
                VehicleStatsResponse rsp = FinanceGrpcClient::getInstance().getVehicleStats(request);
                result["error"] = rsp.error();
                if (rsp.error() == 0) {
                    result["total"] = rsp.total();
                    result["available"] = rsp.available();
                    result["rented"] = rsp.rented();
                    result["maintenance"] = rsp.maintenance();
                    result["utilization_rate"] = rsp.utilization_rate();
                    Json::Value brands(Json::arrayValue);
                    for (int i = 0; i < rsp.by_brand_size(); ++i) {
                        auto& b = rsp.by_brand(i);
                        Json::Value brand;
                        brand["brand"] = b.brand();
                        brand["count"] = b.count();
                        brands.append(brand);
                    }
                    result["by_brand"] = brands;
                } else {
                    result["msg"] = rsp.msg();
                }
            } catch (...) {
                result["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            }
            p->set_value(result);
        });

        auto status = f.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::ready) {
            try {
                Json::Value result = f.get();
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(result);
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonToStringPretty(jsonResp);
        }
    });
}

bool LogicSystem::handleGet(std::string url, std::shared_ptr<HttpConnection> connection) {
    if(getHandlers_.find(url) == getHandlers_.end()) return false;
    getHandlers_[url](connection);
    return true;
}

bool LogicSystem::handlePost(std::string url, std::shared_ptr<HttpConnection> connection) {
    if(postHandlers_.find(url) == postHandlers_.end()) return false;
    postHandlers_[url](connection);
    return true;
}

bool LogicSystem::handlePut(std::string url, std::shared_ptr<HttpConnection> connection) {
    if(putHandlers_.find(url) == putHandlers_.end()) return false;
    putHandlers_[url](connection);
    return true;
}
