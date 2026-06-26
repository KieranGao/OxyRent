#include "UMSGrpcServiceImpl.h"
#include "MySQLManager.h"
#include "RedisManager.h"
#include "Logger.h"
#include <openssl/md5.h>
#include <sstream>
#include <iomanip>
#include <random>

UMSGrpcServiceImpl::UMSGrpcServiceImpl() {}

static std::string md5Hash(const std::string& input) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), digest);
    std::ostringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}

Status UMSGrpcServiceImpl::UserRegister(ServerContext* context, const UserRegisterRequest* req, CommonResponse* resp) {
    std::string username = req->username();
    std::string password = req->password();
    std::string phone = req->phone();
    std::string email = req->email();
    LOG_DEBUG("[UMS] UserRegister: username={}", username);

    std::string password_hash = md5Hash(password);
    int64_t uid = MySQLManager::getInstance().registerUser(username, password_hash, phone, email);
    if (uid == -1) {
        resp->set_error(static_cast<int>(ErrorCodes::USER_ALREADY_EXISTS));
        resp->set_msg("Username already exists");
        return Status::OK;
    }
    if (uid == -2) {
        resp->set_error(static_cast<int>(ErrorCodes::USER_ALREADY_EXISTS));
        resp->set_msg("Email already exists");
        return Status::OK;
    }
    if (uid <= 0) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Registration failed");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("Registration successful");
    LOG_INFO("[UMS] Registration success, uid={}", uid);
    return Status::OK;
}

Status UMSGrpcServiceImpl::UserLogin(ServerContext* context, const UserLoginRequest* req, UserLoginResponse* resp) {
    std::string username = req->username();
    std::string password = req->password();
    LOG_DEBUG("[UMS] Login attempt: {}", username);

    std::string password_hash = md5Hash(password);
    int64_t uid = 0;
    std::string role, status;
    bool ok = MySQLManager::getInstance().loginUser(username, password_hash, uid, role, status);
    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::USER_LOGIN_ERROR));
        resp->set_msg("Invalid username or password");
        return Status::OK;
    }

    if (status != "active") {
        resp->set_error(static_cast<int>(ErrorCodes::USER_NOT_APPROVED));
        resp->set_msg("Account is not active");
        resp->set_uid(uid);
        return Status::OK;
    }

    // Generate a random 32-character hex token
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::ostringstream token_ss;
    for (int i = 0; i < 16; ++i) {
        token_ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    std::string token = token_ss.str();

    // Store token in Redis with 24-hour expiry
    std::string token_key = "utoken_" + std::to_string(uid);
    LOG_INFO("[UMS] Storing token in Redis: key={}, token={}", token_key, token);
    bool redis_ok = RedisManager::getInstance().setex(token_key, token, 86400);
    LOG_INFO("[UMS] Redis SETEX result: {}", redis_ok ? "success" : "failed");

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_uid(uid);
    resp->set_token(token);
    resp->set_role(role);
    LOG_DEBUG("[UMS] Login success, uid={}, role={}", uid, role);
    return Status::OK;
}

Status UMSGrpcServiceImpl::GetUserProfile(ServerContext* context, const UserProfileRequest* req, UserProfileResponse* resp) {
    int64_t uid = req->uid();
    LOG_DEBUG("[UMS] GetUserProfile uid={}", uid);

    UserProfileData profile;
    bool ok = MySQLManager::getInstance().getUserProfile(uid, profile);
    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::USER_DO_NOT_EXISTS));
        resp->set_msg("User not found");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_uid(profile.uid);
    resp->set_username(profile.username);
    resp->set_phone(profile.phone);
    resp->set_email(profile.email);
    resp->set_id_card(profile.id_card);
    resp->set_driver_license(profile.driver_license);
    resp->set_real_name(profile.real_name);
    resp->set_gender(profile.gender);
    resp->set_birth_date(profile.birth_date);
    resp->set_address(profile.address);
    resp->set_avatar_url(profile.avatar_url);
    resp->set_role(profile.role);
    resp->set_status(profile.status);
    return Status::OK;
}

Status UMSGrpcServiceImpl::UpdateProfile(ServerContext* context, const UpdateProfileRequest* req, CommonResponse* resp) {
    int64_t uid = req->uid();
    LOG_DEBUG("[UMS] UpdateProfile uid={}", uid);

    bool ok = MySQLManager::getInstance().updateProfile(
        uid, req->phone(), req->email(), req->id_card(), req->driver_license(),
        req->real_name(), req->gender(), req->birth_date(), req->address(), req->avatar_url());

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::USER_DO_NOT_EXISTS));
        resp->set_msg("Update failed or user not found");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("Profile updated successfully");
    return Status::OK;
}

Status UMSGrpcServiceImpl::GetUserList(ServerContext* context, const UserListRequest* req, UserListResponse* resp) {
    LOG_DEBUG("[UMS] GetUserList page={} page_size={}", req->page(), req->page_size());

    std::vector<UserListItemData> users;
    int total = 0;
    bool ok = MySQLManager::getInstance().getUserList(
        req->page(), req->page_size(), req->keyword(), req->role(), req->status(), users, total);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to fetch user list");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_total(total);
    for (auto& u : users) {
        auto* item = resp->add_users();
        item->set_uid(u.uid);
        item->set_username(u.username);
        item->set_phone(u.phone);
        item->set_email(u.email);
        item->set_real_name(u.real_name);
        item->set_role(u.role);
        item->set_status(u.status);
        item->set_created_at(u.created_at);
    }
    return Status::OK;
}

Status UMSGrpcServiceImpl::UpdateUserStatus(ServerContext* context, const UpdateUserStatusRequest* req, CommonResponse* resp) {
    int64_t uid = req->uid();
    std::string status = req->status();
    LOG_INFO("[UMS] UpdateUserStatus uid={} status={}", uid, status);

    bool ok = MySQLManager::getInstance().updateUserStatus(uid, status);
    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::USER_DO_NOT_EXISTS));
        resp->set_msg("User not found or update failed");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("User status updated");
    return Status::OK;
}

Status UMSGrpcServiceImpl::UpdateUserRole(ServerContext* context, const UpdateUserRoleRequest* req, CommonResponse* resp) {
    int64_t uid = req->uid();
    std::string role = req->role();
    LOG_INFO("[UMS] UpdateUserRole uid={} role={}", uid, role);

    bool ok = MySQLManager::getInstance().updateUserRole(uid, role);
    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::USER_DO_NOT_EXISTS));
        resp->set_msg("User not found or update failed");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("User role updated");
    return Status::OK;
}
