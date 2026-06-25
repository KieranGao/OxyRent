#ifndef CONNECTIONGUARD_H
#define CONNECTIONGUARD_H

#include <memory>
#include "MySQLConnectPool.h"

// RAII guard for MySQL connections: returns connection to pool on destruction
class ConnectionGuard {
public:
    ConnectionGuard(MySQLConnectPool& pool, std::unique_ptr<SqlConnection> conn)
        : pool_(pool), conn_(std::move(conn)) {}
    ~ConnectionGuard() {
        if (conn_) {
            pool_.returnConnection(std::move(conn_));
        }
    }
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
    ConnectionGuard(ConnectionGuard&&) = default;
    ConnectionGuard& operator=(ConnectionGuard&&) = default;

    std::unique_ptr<SqlConnection>& get() { return conn_; }
    explicit operator bool() const { return conn_ != nullptr; }
private:
    MySQLConnectPool& pool_;
    std::unique_ptr<SqlConnection> conn_;
};

#endif /* CONNECTIONGUARD_H */
