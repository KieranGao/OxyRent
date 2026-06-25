#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H

#include "MySQLDao.h"
#include "Singleton.h"

class MySQLManager : public Singleton<MySQLManager> {
    friend class Singleton<MySQLManager>;
public:
    ~MySQLManager() = default;
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
private:
    MySQLManager();
    std::unique_ptr<MySQLDao> dao_;
};

#endif /* MYSQLMANAGER_H */
