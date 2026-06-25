#include "MySQLManager.h"

MySQLManager::MySQLManager() {
    dao_ = std::make_unique<MySQLDao>();
}

int64_t MySQLManager::registerUser(const std::string& username, const std::string& password_hash,
                                   const std::string& phone, const std::string& email) {
    return dao_->registerUser(username, password_hash, phone, email);
}

bool MySQLManager::loginUser(const std::string& username, const std::string& password_hash,
                             int64_t& uid, std::string& role, std::string& status) {
    return dao_->loginUser(username, password_hash, uid, role, status);
}

bool MySQLManager::getUserProfile(int64_t uid, UserProfileData& profile) {
    return dao_->getUserProfile(uid, profile);
}

bool MySQLManager::updateProfile(int64_t uid, const std::string& phone, const std::string& email,
                                 const std::string& id_card, const std::string& driver_license,
                                 const std::string& real_name, int gender, const std::string& birth_date,
                                 const std::string& address, const std::string& avatar_url) {
    return dao_->updateProfile(uid, phone, email, id_card, driver_license, real_name,
                               gender, birth_date, address, avatar_url);
}

bool MySQLManager::getUserList(int page, int page_size, const std::string& keyword,
                               const std::string& role, const std::string& status,
                               std::vector<UserListItemData>& users, int& total) {
    return dao_->getUserList(page, page_size, keyword, role, status, users, total);
}

bool MySQLManager::updateUserStatus(int64_t uid, const std::string& status) {
    return dao_->updateUserStatus(uid, status);
}

bool MySQLManager::updateUserRole(int64_t uid, const std::string& role) {
    return dao_->updateUserRole(uid, role);
}
