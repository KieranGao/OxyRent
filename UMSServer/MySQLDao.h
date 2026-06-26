#ifndef MYSQLDAO_H
#define MYSQLDAO_H

#include "MySQLConnectPool.h"
#include <string>
#include <vector>

struct UserListItemData {
    int64_t uid;
    std::string username;
    std::string phone;
    std::string email;
    std::string real_name;
    std::string role;
    std::string status;
    std::string created_at;
};

class MySQLDao {
public:
    MySQLDao();
    ~MySQLDao();
    int64_t registerUser(const std::string& username, const std::string& password_hash,
                         const std::string& phone, const std::string& email);
    bool loginUser(const std::string& username, const std::string& password_hash,
                   int64_t& uid, std::string& role, std::string& status);
    bool getUserProfile(int64_t uid, UserProfileData& profile);
    bool updateProfile(int64_t uid, const std::string& phone, const std::string& email,
                       const std::string& id_card, const std::string& driver_license,
                       const std::string& real_name, int gender, const std::string& birth_date,
                       const std::string& address, const std::string& avatar_url);
    bool getUserList(int page, int page_size, const std::string& keyword,
                     const std::string& role, const std::string& status,
                     std::vector<UserListItemData>& users, int& total);
    bool updateUserStatus(int64_t uid, const std::string& status);
    bool updateUserRole(int64_t uid, const std::string& role);
    // Balance operations
    double getBalance(int64_t uid);
    bool topupBalance(int64_t uid, double amount, int64_t operator_id, const std::string& remark);
    bool consumeBalance(int64_t uid, double amount, const std::string& remark);
    bool getBalanceRecords(int64_t uid, int page, int page_size, std::vector<BalanceRecordData>& records, int& total);
private:
    std::unique_ptr<MySQLConnectPool> pool_;
};

#endif /* MYSQLDAO_H */
