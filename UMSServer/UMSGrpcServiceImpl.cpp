#include "UMSGrpcServiceImpl.h"
#include "MySQLManager.h"
#include "RedisManager.h"
#include "Logger.h"
#include <openssl/md5.h>
#include <sstream>
#include <iomanip>
#include <random>

UMSGrpcServiceImpl::UMSGrpcServiceImpl() {}

// 生成唯一的锁持有者ID（用于分布式锁）
static std::string generateLockOwnerId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);
    std::ostringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return ss.str();
}

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
        resp->set_msg("用户名已存在");
        return Status::OK;
    }
    if (uid == -2) {
        resp->set_error(static_cast<int>(ErrorCodes::USER_ALREADY_EXISTS));
        resp->set_msg("邮箱已存在");
        return Status::OK;
    }
    if (uid <= 0) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("注册失败");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("注册成功");
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
        resp->set_msg("用户名或密码错误");
        return Status::OK;
    }

    if (status != "active") {
        resp->set_error(static_cast<int>(ErrorCodes::USER_NOT_APPROVED));
        resp->set_msg("账号未激活");
        resp->set_uid(uid);
        return Status::OK;
    }

    // 生成32位随机十六进制token
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::ostringstream token_ss;
    for (int i = 0; i < 16; ++i) {
        token_ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    std::string token = token_ss.str();

    // 将token存储到Redis，24小时过期
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
        resp->set_msg("用户不存在");
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
        resp->set_msg("更新失败或用户不存在");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("个人信息更新成功");
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
        resp->set_msg("获取用户列表失败");
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
        item->set_balance(u.balance);
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
        resp->set_msg("用户不存在或更新失败");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("用户状态已更新");
    return Status::OK;
}

Status UMSGrpcServiceImpl::UpdateUserRole(ServerContext* context, const UpdateUserRoleRequest* req, CommonResponse* resp) {
    int64_t uid = req->uid();
    std::string role = req->role();
    LOG_INFO("[UMS] UpdateUserRole uid={} role={}", uid, role);

    bool ok = MySQLManager::getInstance().updateUserRole(uid, role);
    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::USER_DO_NOT_EXISTS));
        resp->set_msg("用户不存在或更新失败");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("用户角色已更新");
    return Status::OK;
}

Status UMSGrpcServiceImpl::GetBalance(ServerContext* context, const GetBalanceRequest* req, GetBalanceResponse* resp) {
    int64_t uid = req->uid();
    LOG_DEBUG("[UMS] GetBalance uid={}", uid);

    double balance = MySQLManager::getInstance().getBalance(uid);
    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_balance(balance);
    return Status::OK;
}

Status UMSGrpcServiceImpl::TopupBalance(ServerContext* context, const TopupRequest* req, CommonResponse* resp) {
    int64_t uid = req->uid();
    double amount = req->amount();
    std::string remark = req->remark();
    LOG_INFO("[UMS] TopupBalance uid={} amount={}", uid, amount);

    if (amount <= 0) {
        resp->set_error(static_cast<int>(ErrorCodes::BALANCE_TOPUP_FAILED));
        resp->set_msg("充值金额必须大于0");
        return Status::OK;
    }

    // 分布式锁：锁定用户余额，防止并发充值
    std::string lock_key = "lock:balance:" + std::to_string(uid);
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_error(static_cast<int>(ErrorCodes::BALANCE_TOPUP_FAILED));
        resp->set_msg("系统繁忙，请稍后重试");
        return Status::OK;
    }

    bool ok = MySQLManager::getInstance().topupBalance(uid, amount, uid, remark);

    // 释放锁
    redis.releaseLock(lock_key, owner_id);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::BALANCE_TOPUP_FAILED));
        resp->set_msg("充值失败");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("充值成功");
    return Status::OK;
}

Status UMSGrpcServiceImpl::ConsumeBalance(ServerContext* context, const ConsumeBalanceRequest* req, CommonResponse* resp) {
    int64_t uid = req->uid();
    double amount = req->amount();
    std::string remark = req->remark();
    LOG_INFO("[UMS] ConsumeBalance uid={} amount={}", uid, amount);

    if (amount <= 0) {
        resp->set_error(static_cast<int>(ErrorCodes::BALANCE_CONSUME_FAILED));
        resp->set_msg("消费金额必须大于0");
        return Status::OK;
    }

    // 分布式锁：锁定用户余额，防止并发消费
    std::string lock_key = "lock:balance:" + std::to_string(uid);
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_error(static_cast<int>(ErrorCodes::BALANCE_CONSUME_FAILED));
        resp->set_msg("系统繁忙，请稍后重试");
        return Status::OK;
    }

    bool ok = MySQLManager::getInstance().consumeBalance(uid, amount, remark);

    // 释放锁
    redis.releaseLock(lock_key, owner_id);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::BALANCE_INSUFFICIENT));
        resp->set_msg("余额不足");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("消费成功");
    return Status::OK;
}

Status UMSGrpcServiceImpl::GetBalanceRecords(ServerContext* context, const BalanceRecordListRequest* req, BalanceRecordListResponse* resp) {
    int64_t uid = req->uid();
    int page = req->page() > 0 ? req->page() : 1;
    int page_size = req->page_size() > 0 ? req->page_size() : 20;
    LOG_INFO("[UMS] GetBalanceRecords uid={} page={} page_size={}", uid, page, page_size);

    std::vector<BalanceRecordData> records;
    int total = 0;
    bool ok = MySQLManager::getInstance().getBalanceRecords(uid, page, page_size, records, total);
    LOG_INFO("[UMS] GetBalanceRecords ok={} total={} records_size={}", ok, total, records.size());
    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to fetch balance records");
        return Status::OK;
    }

    double balance = MySQLManager::getInstance().getBalance(uid);
    LOG_INFO("[UMS] GetBalanceRecords balance={}", balance);
    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_total(total);
    resp->set_balance(balance);
    for (auto& r : records) {
        auto* item = resp->add_records();
        item->set_id(r.id);
        item->set_user_id(r.user_id);
        item->set_amount(r.amount);
        item->set_type(r.type);
        item->set_operator_id(r.operator_id);
        item->set_remark(r.remark);
        item->set_created_at(r.created_at);
    }
    return Status::OK;
}

Status UMSGrpcServiceImpl::UpdateBalanceRecordRemark(ServerContext* context, const UpdateBalanceRecordRemarkRequest* req, CommonResponse* resp) {
    int64_t user_id = req->user_id();
    std::string old_remark = req->old_remark();
    std::string new_remark = req->new_remark();
    LOG_INFO("[UMS] UpdateBalanceRecordRemark user_id={} old={} new={}", user_id, old_remark, new_remark);

    bool ok = MySQLManager::getInstance().updateBalanceRecordRemark(user_id, old_remark, new_remark);
    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("更新备注失败");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("备注已更新");
    return Status::OK;
}

Status UMSGrpcServiceImpl::ResetPassword(ServerContext* context, const ResetPasswordRequest* req, CommonResponse* resp) {
    std::string email = req->email();
    std::string new_password = req->new_password();
    LOG_INFO("[UMS] ResetPassword email={}", email);

    if (email.empty() || new_password.empty()) {
        resp->set_error(static_cast<int>(ErrorCodes::JSON_PARSE_ERROR));
        resp->set_msg("邮箱和新密码不能为空");
        return Status::OK;
    }

    if (new_password.length() < 6) {
        resp->set_error(static_cast<int>(ErrorCodes::JSON_PARSE_ERROR));
        resp->set_msg("密码至少6个字符");
        return Status::OK;
    }

    std::string password_hash = md5Hash(new_password);
    bool ok = MySQLManager::getInstance().resetPassword(email, password_hash);
    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::USER_DO_NOT_EXISTS));
        resp->set_msg("邮箱不存在或重置失败");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_msg("密码重置成功");
    LOG_INFO("[UMS] Password reset success for email={}", email);
    return Status::OK;
}
