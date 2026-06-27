#include "MySQLDao.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "ConnectionGuard.h"

MySQLDao::MySQLDao() {
    ConfigManager& config = ConfigManager::getInstance();
    std::string host = config["MySQL"]["host"];
    std::string port = config["MySQL"]["port"];
    std::string user = config["MySQL"]["user"];
    std::string dbName = config["MySQL"]["dbName"];
    std::string password = config["MySQL"]["password"];
    pool_ = std::make_unique<MySQLConnectPool>(5, host + ":" + port, user, password, dbName);
}

MySQLDao::~MySQLDao() {
    pool_->stop();
}

int64_t MySQLDao::registerUser(const std::string& username, const std::string& password_hash,
                               const std::string& phone, const std::string& email) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        LOG_DEBUG("registering user: {}", username);
        auto& sql_conn = connection.get()->getConn();

        // 检查用户名是否重复
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            sql_conn->prepareStatement("SELECT uid FROM user WHERE username = ? LIMIT 1"));
        checkStmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());
        if (checkRes && checkRes->next()) {
            LOG_WARN("Username already exists: {}", username);
            return -1;
        }

        // 检查邮箱是否重复
        if (!email.empty()) {
            std::unique_ptr<sql::PreparedStatement> emailStmt(
                sql_conn->prepareStatement("SELECT uid FROM user WHERE email = ? LIMIT 1"));
            emailStmt->setString(1, email);
            std::unique_ptr<sql::ResultSet> emailRes(emailStmt->executeQuery());
            if (emailRes && emailRes->next()) {
                LOG_WARN("Email already exists: {}", email);
                return -2;
            }
        }

        // 插入新用户
        std::unique_ptr<sql::PreparedStatement> insertStmt(
            sql_conn->prepareStatement(
                "INSERT INTO user (username, password, phone, email, role, status) VALUES (?, ?, ?, ?, 'customer', 'active')"));
        insertStmt->setString(1, username);
        insertStmt->setString(2, password_hash);
        insertStmt->setString(3, phone);
        insertStmt->setString(4, email);
        insertStmt->executeUpdate();

        // 获取新用户ID
        std::unique_ptr<sql::Statement> stmtResult(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT LAST_INSERT_ID() AS uid"));
        if (res && res->next()) {
            int64_t uid = res->getInt64("uid");
            LOG_DEBUG("new user uid: {}", uid);
            return uid;
        }
        return -1;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in registerUser: {}", exp.what());
        return -1;
    }
}

bool MySQLDao::loginUser(const std::string& username, const std::string& password_hash,
                         int64_t& uid, std::string& role, std::string& status) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        LOG_DEBUG("login attempt for user: {}", username);
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "SELECT uid, role, status FROM user WHERE username = ? AND password = ? LIMIT 1"));
        pstmt->setString(1, username);
        pstmt->setString(2, password_hash);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res || !res->next()) {
            LOG_ERROR("Login failed: bad credentials for {}", username);
            return false;
        }
        uid = res->getInt64("uid");
        role = res->getString("role");
        status = res->getString("status");
        LOG_DEBUG("user {} has login, uid={}, role={}, status={}", username, uid, role, status);
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in loginUser: {}", exp.what());
        return false;
    }
}

bool MySQLDao::getUserProfile(int64_t uid, UserProfileData& profile) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "SELECT uid, username, phone, email, id_card, driver_license, real_name, "
                "gender, birth_date, address, avatar_url, role, status FROM user WHERE uid = ? LIMIT 1"));
        pstmt->setInt64(1, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res || !res->next()) {
            return false;
        }
        profile.uid = res->getInt64("uid");
        profile.username = res->getString("username");
        profile.phone = res->getString("phone");
        profile.email = res->getString("email");
        profile.id_card = res->getString("id_card");
        profile.driver_license = res->getString("driver_license");
        profile.real_name = res->getString("real_name");
        profile.gender = res->getInt("gender");
        profile.birth_date = res->getString("birth_date");
        profile.address = res->getString("address");
        profile.avatar_url = res->getString("avatar_url");
        profile.role = res->getString("role");
        profile.status = res->getString("status");
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getUserProfile: {}", exp.what());
        return false;
    }
}

bool MySQLDao::updateProfile(int64_t uid, const std::string& phone, const std::string& email,
                             const std::string& id_card, const std::string& driver_license,
                             const std::string& real_name, int gender, const std::string& birth_date,
                             const std::string& address, const std::string& avatar_url) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "UPDATE user SET phone=?, email=?, id_card=?, driver_license=?, real_name=?, "
                "gender=?, birth_date=?, address=?, avatar_url=? WHERE uid=?"));
        pstmt->setString(1, phone);
        pstmt->setString(2, email);
        pstmt->setString(3, id_card);
        pstmt->setString(4, driver_license);
        pstmt->setString(5, real_name);
        pstmt->setInt(6, gender);
        pstmt->setString(7, birth_date);
        pstmt->setString(8, address);
        pstmt->setString(9, avatar_url);
        pstmt->setInt64(10, uid);
        int affected = pstmt->executeUpdate();
        LOG_DEBUG("updateProfile uid={} affected={}", uid, affected);
        return affected > 0;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in updateProfile: {}", exp.what());
        return false;
    }
}

bool MySQLDao::getUserList(int page, int page_size, const std::string& keyword,
                           const std::string& role, const std::string& status,
                           std::vector<UserListItemData>& users, int& total) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // 构建WHERE子句
        std::string where = "1=1";
        std::vector<std::string> params;

        if (!keyword.empty()) {
            where += " AND (username LIKE ? OR phone LIKE ? OR email LIKE ? OR real_name LIKE ?)";
            std::string kw = "%" + keyword + "%";
            params.push_back(kw);
            params.push_back(kw);
            params.push_back(kw);
            params.push_back(kw);
        }
        if (!role.empty()) {
            where += " AND role = ?";
            params.push_back(role);
        }
        if (!status.empty()) {
            where += " AND status = ?";
            params.push_back(status);
        }

        // 统计总数
        std::string countSql = "SELECT COUNT(*) AS cnt FROM user WHERE " + where;
        std::unique_ptr<sql::PreparedStatement> countStmt(sql_conn->prepareStatement(countSql));
        for (size_t i = 0; i < params.size(); ++i) {
            countStmt->setString(i + 1, params[i]);
        }
        std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());
        if (countRes && countRes->next()) {
            total = countRes->getInt("cnt");
        }

        // 分页查询
        std::string querySql = "SELECT uid, username, phone, email, real_name, role, status, created_at, balance "
                               "FROM user WHERE " + where + " ORDER BY uid DESC LIMIT ? OFFSET ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(sql_conn->prepareStatement(querySql));
        for (size_t i = 0; i < params.size(); ++i) {
            pstmt->setString(i + 1, params[i]);
        }
        pstmt->setInt(params.size() + 1, page_size);
        pstmt->setInt(params.size() + 2, (page - 1) * page_size);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res && res->next()) {
            UserListItemData item;
            item.uid = res->getInt64("uid");
            item.username = res->getString("username");
            item.phone = res->getString("phone");
            item.email = res->getString("email");
            item.real_name = res->getString("real_name");
            item.role = res->getString("role");
            item.status = res->getString("status");
            item.created_at = res->getString("created_at");
            item.balance = res->getDouble("balance");
            users.push_back(item);
        }
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getUserList: {}", exp.what());
        return false;
    }
}

bool MySQLDao::updateUserStatus(int64_t uid, const std::string& status) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement("UPDATE user SET status = ? WHERE uid = ?"));
        pstmt->setString(1, status);
        pstmt->setInt64(2, uid);
        int affected = pstmt->executeUpdate();
        LOG_DEBUG("updateUserStatus uid={} status={} affected={}", uid, status, affected);
        return affected > 0;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in updateUserStatus: {}", exp.what());
        return false;
    }
}

bool MySQLDao::updateUserRole(int64_t uid, const std::string& role) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement("UPDATE user SET role = ? WHERE uid = ?"));
        pstmt->setString(1, role);
        pstmt->setInt64(2, uid);
        int affected = pstmt->executeUpdate();
        LOG_DEBUG("updateUserRole uid={} role={} affected={}", uid, role, affected);
        return affected > 0;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in updateUserRole: {}", exp.what());
        return false;
    }
}

double MySQLDao::getBalance(int64_t uid) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement("SELECT balance FROM user WHERE uid = ? LIMIT 1"));
        pstmt->setInt64(1, uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res && res->next()) {
            return res->getDouble("balance");
        }
        return 0.0;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getBalance: {}", exp.what());
        return 0.0;
    }
}

bool MySQLDao::topupBalance(int64_t uid, double amount, int64_t operator_id, const std::string& remark) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        // 更新余额
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement("UPDATE user SET balance = balance + ? WHERE uid = ? AND balance + ? >= 0"));
        pstmt->setDouble(1, amount);
        pstmt->setInt64(2, uid);
        pstmt->setDouble(3, amount);
        int affected = pstmt->executeUpdate();
        if (affected <= 0) {
            LOG_ERROR("topupBalance failed for uid={}", uid);
            return false;
        }
        // 插入记录
        std::unique_ptr<sql::PreparedStatement> recStmt(
            sql_conn->prepareStatement("INSERT INTO balance_records (user_id, amount, type, operator_id, remark) VALUES (?, ?, 'topup', ?, ?)"));
        recStmt->setInt64(1, uid);
        recStmt->setDouble(2, amount);
        recStmt->setInt64(3, operator_id);
        recStmt->setString(4, remark);
        recStmt->executeUpdate();
        LOG_DEBUG("topupBalance uid={} amount={} operator_id={}", uid, amount, operator_id);
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in topupBalance: {}", exp.what());
        return false;
    }
}

bool MySQLDao::consumeBalance(int64_t uid, double amount, const std::string& remark) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        // 更新余额
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement("UPDATE user SET balance = balance - ? WHERE uid = ? AND balance >= ?"));
        pstmt->setDouble(1, amount);
        pstmt->setInt64(2, uid);
        pstmt->setDouble(3, amount);
        int affected = pstmt->executeUpdate();
        if (affected <= 0) {
            LOG_ERROR("consumeBalance failed: insufficient balance for uid={}", uid);
            return false;
        }
        // 插入记录
        std::unique_ptr<sql::PreparedStatement> recStmt(
            sql_conn->prepareStatement("INSERT INTO balance_records (user_id, amount, type, remark) VALUES (?, ?, 'consume', ?)"));
        recStmt->setInt64(1, uid);
        recStmt->setDouble(2, amount);
        recStmt->setString(3, remark);
        recStmt->executeUpdate();
        LOG_DEBUG("consumeBalance uid={} amount={}", uid, amount);
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in consumeBalance: {}", exp.what());
        return false;
    }
}

bool MySQLDao::getBalanceRecords(int64_t uid, int page, int page_size, std::vector<BalanceRecordData>& records, int& total) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        // 统计总数
        std::unique_ptr<sql::PreparedStatement> countStmt(
            sql_conn->prepareStatement("SELECT COUNT(*) AS cnt FROM balance_records WHERE user_id = ?"));
        countStmt->setInt64(1, uid);
        std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());
        if (countRes && countRes->next()) {
            total = countRes->getInt("cnt");
        }
        // 查询
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "SELECT id, user_id, amount, type, operator_id, remark, created_at "
                "FROM balance_records WHERE user_id = ? ORDER BY id DESC LIMIT ? OFFSET ?"));
        pstmt->setInt64(1, uid);
        pstmt->setInt(2, page_size);
        pstmt->setInt(3, (page - 1) * page_size);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res && res->next()) {
            BalanceRecordData item;
            item.id = res->getInt64("id");
            item.user_id = res->getInt64("user_id");
            item.amount = res->getDouble("amount");
            item.type = res->getString("type");
            item.operator_id = res->getInt64("operator_id");
            item.remark = res->getString("remark");
            item.created_at = res->getString("created_at");
            records.push_back(item);
        }
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getBalanceRecords: {}", exp.what());
        return false;
    }
}

bool MySQLDao::updateBalanceRecordRemark(int64_t user_id, const std::string& old_remark, const std::string& new_remark) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "UPDATE balance_records SET remark = ? WHERE user_id = ? AND remark = ? ORDER BY id DESC LIMIT 1"));
        pstmt->setString(1, new_remark);
        pstmt->setInt64(2, user_id);
        pstmt->setString(3, old_remark);
        int affected = pstmt->executeUpdate();
        LOG_INFO("updateBalanceRecordRemark user_id={} old={} new={} affected={}", user_id, old_remark, new_remark, affected);
        return affected > 0;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in updateBalanceRecordRemark: {}", exp.what());
        return false;
    }
}

bool MySQLDao::resetPassword(const std::string& email, const std::string& new_password_hash) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement("UPDATE user SET password = ? WHERE email = ?"));
        pstmt->setString(1, new_password_hash);
        pstmt->setString(2, email);
        int affected = pstmt->executeUpdate();
        LOG_INFO("resetPassword email={} affected={}", email, affected);
        return affected > 0;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in resetPassword: {}", exp.what());
        return false;
    }
}
