#include "LogicSystem.h"
#include "AsyncTaskPool.h"
#include "HttpConnection.h"
#include "UserGrpcClient.h"
#include "VehicleGrpcClient.h"
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

    // ============ Vehicle routes ============

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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
        }
    });

    registerGet("/vehicle/detail", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        if (!connection->get_params_.count("id")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            jsonResp["msg"] = "Missing id parameter";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }
        int64_t id = 0;
        try { id = std::stoll(connection->get_params_["id"]); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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

    registerPost("/vehicle", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] ADD_VEHICLE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }

        // Admin check
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }

        // Admin check
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

        int64_t id = 0;
        try { id = jsonData["id"].asInt64(); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }

        // Admin check
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

        int64_t id = 0;
        try { id = jsonData["id"].asInt64(); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
        }
    });

    // ============ Rental routes ============

    registerPost("/rental/create", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] CREATE_ORDER: {}", body);
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
        try { uid = std::stoll(std::string(uid_it->value().data(), uid_it->value().size())); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::USER_ID_INVALID);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
        }
    });

    registerGet("/rental/detail", [](std::shared_ptr<HttpConnection> connection) {
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonResp;

        if (!connection->get_params_.count("id")) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            jsonResp["msg"] = "Missing id parameter";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }
        int64_t id = 0;
        try { id = std::stoll(connection->get_params_["id"]); } catch (...) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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

    registerPost("/rental/pickup", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] PICKUP_VEHICLE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
        }
    });

    // ============ Maintenance routes ============

    registerPost("/maintenance/create", [](std::shared_ptr<HttpConnection> connection) {
        auto body = beast::buffers_to_string(connection->req_.body().data());
        LOG_DEBUG("[Gate] CREATE_MAINTENANCE: {}", body);
        connection->resp_.set(http::field::content_type, "application/json");
        Json::Value jsonData, jsonResp;
        Json::Reader reader;
        if(!reader.parse(body, jsonData)) {
            jsonResp["error"] = static_cast<int>(ErrorCodes::JSON_PARSE_ERROR);
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }

        // Admin check
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            return;
        }

        // Admin check
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
            beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
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
                beast::ostream(connection->resp_.body()) << result.toStyledString();
            } catch (...) {
                jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
                beast::ostream(connection->resp_.body()) << jsonResp.toStyledString();
            }
        } else {
            jsonResp["error"] = static_cast<int>(ErrorCodes::RPC_ERROR);
            jsonResp["msg"] = "timeout";
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
