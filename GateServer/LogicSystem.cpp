#include "LogicSystem.h"
#include "AsyncTaskPool.h"
#include "HttpConnection.h"
#include "UserGrpcClient.h"
#include "RedisManager.h"
#include "Logger.h"

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
    // ============ Public routes (no auth) ============

    registerPost("/user/register", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] USER_REGISTER: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                    // Store token in Redis
                    std::string uid_str = std::to_string(result["uid"].asInt64());
                    std::string token = result["token"].asString();
                    RedisManager::getInstance().setex(USER_TOKEN_PREFIX + uid_str, token, 86400);
                    // Store role in Redis for admin authorization checks
                    std::string role = result["role"].asString();
                    RedisManager::getInstance().setex(USER_ROLE_PREFIX + uid_str, role, 86400);
                }
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            LOG_ERROR("[Gate] /user/login timed out after 5s");
        }
    });

    // ============ Protected routes (require auth) ============

    registerGet("/user/profile", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        auto uid_it = connection->req_.find("X-User-Id");
        if (uid_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }
        int64_t uid = 0;
        try {
            uid = std::stoll(std::string(uid_it->value().data(), uid_it->value().size()));
        } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }

        auto uid_it = connection->req_.find("X-User-Id");
        if (uid_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }
        int64_t uid = 0;
        try {
            uid = std::stoll(std::string(uid_it->value().data(), uid_it->value().size()));
        } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
        }
    });

    registerGet("/user/list", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        // Extract query params
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
        }
    });

    // PUT /user/{id}/status - Admin only
    registerPut("/user/status", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] UPDATE_USER_STATUS: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }

        // Check caller's role - admin only
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }

        int64_t uid = 0;
        try {
            uid = jsonData["uid"].asInt64();
        } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
        }
    });

    // PUT /user/{id}/role - Admin only
    registerPut("/user/role", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] UPDATE_USER_ROLE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }

        // Check caller's role - admin only
        auto caller_it = connection->req_.find("X-User-Id");
        if (caller_it == connection->req_.end()) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }
        std::string caller_uid_str(caller_it->value().data(), caller_it->value().size());
        std::string caller_role;
        if (!RedisManager::getInstance().get(USER_ROLE_PREFIX + caller_uid_str, caller_role) ||
            (caller_role != "2" && caller_role != "admin")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID);
            jsonResp["msg"] = "Admin access required";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }

        int64_t uid = 0;
        try {
            uid = jsonData["uid"].asInt64();
        } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
