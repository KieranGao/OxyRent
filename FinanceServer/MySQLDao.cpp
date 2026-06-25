#include "MySQLDao.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "ConnectionGuard.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

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

// ==================== Payment Operations ====================

int64_t MySQLDao::createPayment(int64_t order_id, double amount, const std::string& type,
                                 const std::string& method, const std::string& remark) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // Get order_no from rental_orders
        std::string order_no;
        std::unique_ptr<sql::PreparedStatement> orderStmt(
            sql_conn->prepareStatement("SELECT order_no FROM rental_orders WHERE id = ? LIMIT 1"));
        orderStmt->setInt64(1, order_id);
        std::unique_ptr<sql::ResultSet> orderRes(orderStmt->executeQuery());
        if (!orderRes || !orderRes->next()) {
            LOG_WARN("createPayment: order {} not found", order_id);
            return -1;
        }
        order_no = orderRes->getString("order_no");

        std::unique_ptr<sql::PreparedStatement> insertStmt(
            sql_conn->prepareStatement(
                "INSERT INTO payments (order_id, order_no, amount, type, method, status, remark) "
                "VALUES (?, ?, ?, ?, ?, 'pending', ?)"));
        insertStmt->setInt64(1, order_id);
        insertStmt->setString(2, order_no);
        insertStmt->setDouble(3, amount);
        insertStmt->setString(4, type);
        insertStmt->setString(5, method);
        insertStmt->setString(6, remark);
        insertStmt->executeUpdate();

        std::unique_ptr<sql::Statement> stmtResult(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT LAST_INSERT_ID() AS id"));
        if (res && res->next()) {
            int64_t id = res->getInt64("id");
            LOG_DEBUG("new payment id: {}", id);
            return id;
        }
        return -1;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in createPayment: {}", exp.what());
        return -1;
    }
}

bool MySQLDao::getPaymentList(int page, int page_size, int64_t order_id,
                               const std::string& status, const std::string& type,
                               std::vector<PaymentData>& payments, int& total) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        std::string where = "1=1";
        std::vector<std::string> params;

        if (order_id > 0) {
            where += " AND p.order_id = ?";
            params.push_back(std::to_string(order_id));
        }
        if (!status.empty()) {
            where += " AND p.status = ?";
            params.push_back(status);
        }
        if (!type.empty()) {
            where += " AND p.type = ?";
            params.push_back(type);
        }

        // Count total
        std::string countSql = "SELECT COUNT(*) AS cnt FROM payments p WHERE " + where;
        std::unique_ptr<sql::PreparedStatement> countStmt(sql_conn->prepareStatement(countSql));
        for (size_t i = 0; i < params.size(); ++i) {
            countStmt->setString(i + 1, params[i]);
        }
        std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());
        if (countRes && countRes->next()) {
            total = countRes->getInt("cnt");
        }

        // Paginated query
        std::string querySql = "SELECT id, order_id, order_no, amount, type, method, status, "
                               "remark, paid_at, created_at "
                               "FROM payments p WHERE " + where + " ORDER BY id DESC LIMIT ? OFFSET ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(sql_conn->prepareStatement(querySql));
        for (size_t i = 0; i < params.size(); ++i) {
            pstmt->setString(i + 1, params[i]);
        }
        pstmt->setInt(params.size() + 1, page_size);
        pstmt->setInt(params.size() + 2, (page - 1) * page_size);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res && res->next()) {
            PaymentData item;
            item.id = res->getInt64("id");
            item.order_id = res->getInt64("order_id");
            item.order_no = res->getString("order_no");
            item.amount = res->getDouble("amount");
            item.type = res->getString("type");
            item.method = res->getString("method");
            item.status = res->getString("status");
            item.remark = res->getString("remark");
            item.paid_at = res->getString("paid_at");
            item.created_at = res->getString("created_at");
            payments.push_back(item);
        }
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getPaymentList: {}", exp.what());
        return false;
    }
}

bool MySQLDao::getPaymentDetail(int64_t id, PaymentData& payment) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "SELECT id, order_id, order_no, amount, type, method, status, "
                "remark, paid_at, created_at "
                "FROM payments WHERE id = ? LIMIT 1"));
        pstmt->setInt64(1, id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res || !res->next()) {
            return false;
        }
        payment.id = res->getInt64("id");
        payment.order_id = res->getInt64("order_id");
        payment.order_no = res->getString("order_no");
        payment.amount = res->getDouble("amount");
        payment.type = res->getString("type");
        payment.method = res->getString("method");
        payment.status = res->getString("status");
        payment.remark = res->getString("remark");
        payment.paid_at = res->getString("paid_at");
        payment.created_at = res->getString("created_at");
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getPaymentDetail: {}", exp.what());
        return false;
    }
}

// ==================== Invoice Operations ====================

int64_t MySQLDao::generateInvoice(int64_t order_id) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // Check if invoice already exists for this order
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            sql_conn->prepareStatement("SELECT id FROM invoices WHERE order_id = ? LIMIT 1"));
        checkStmt->setInt64(1, order_id);
        std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());
        if (checkRes && checkRes->next()) {
            LOG_WARN("generateInvoice: invoice already exists for order {}", order_id);
            return checkRes->getInt64("id");
        }

        // Get order details
        std::unique_ptr<sql::PreparedStatement> orderStmt(
            sql_conn->prepareStatement(
                "SELECT o.id, o.order_no, o.user_id, u.username, o.total_cost "
                "FROM rental_orders o "
                "LEFT JOIN user u ON o.user_id = u.uid "
                "WHERE o.id = ? LIMIT 1"));
        orderStmt->setInt64(1, order_id);
        std::unique_ptr<sql::ResultSet> orderRes(orderStmt->executeQuery());
        if (!orderRes || !orderRes->next()) {
            LOG_WARN("generateInvoice: order {} not found", order_id);
            return -1;
        }

        std::string order_no = orderRes->getString("order_no");
        int64_t user_id = orderRes->getInt64("user_id");
        std::string username = orderRes->getString("username");
        double total_cost = orderRes->getDouble("total_cost");

        // Generate invoice number
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_r(&tt, &tm);
        char dateBuf[16];
        snprintf(dateBuf, sizeof(dateBuf), "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        std::string datePrefix(dateBuf);

        std::string countSql = "SELECT COUNT(*) AS cnt FROM invoices WHERE invoice_no LIKE ?";
        std::unique_ptr<sql::PreparedStatement> countStmt(sql_conn->prepareStatement(countSql));
        countStmt->setString(1, "INV" + datePrefix + "%");
        std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());
        int seq = 1;
        if (countRes && countRes->next()) {
            seq = countRes->getInt("cnt") + 1;
        }
        char seqBuf[8];
        snprintf(seqBuf, sizeof(seqBuf), "%04d", seq);
        std::string invoice_no = "INV" + datePrefix + std::string(seqBuf);

        // Aggregate payments as JSON items
        std::string items_json = "[]";
        std::unique_ptr<sql::PreparedStatement> payStmt(
            sql_conn->prepareStatement(
                "SELECT id, amount, type, method, paid_at FROM payments "
                "WHERE order_id = ? AND status = 'completed'"));
        payStmt->setInt64(1, order_id);
        std::unique_ptr<sql::ResultSet> payRes(payStmt->executeQuery());

        std::ostringstream jsonItems;
        jsonItems << "[";
        bool first = true;
        double invoice_total = 0.0;
        while (payRes && payRes->next()) {
            if (!first) jsonItems << ",";
            first = false;
            int64_t pay_id = payRes->getInt64("id");
            double amount = payRes->getDouble("amount");
            std::string ptype = payRes->getString("type");
            std::string method = payRes->getString("method");
            std::string paid_at = payRes->getString("paid_at");
            invoice_total += amount;
            jsonItems << "{\"id\":" << pay_id
                      << ",\"amount\":" << amount
                      << ",\"type\":\"" << ptype
                      << "\",\"method\":\"" << method
                      << "\",\"paid_at\":\"" << paid_at << "\"}";
        }
        jsonItems << "]";
        items_json = jsonItems.str();

        // Use total_cost if no completed payments, otherwise use aggregated total
        if (invoice_total == 0.0) {
            invoice_total = total_cost;
        }

        std::unique_ptr<sql::PreparedStatement> insertStmt(
            sql_conn->prepareStatement(
                "INSERT INTO invoices (invoice_no, order_id, order_no, user_id, username, "
                "total_amount, items) VALUES (?, ?, ?, ?, ?, ?, ?)"));
        insertStmt->setString(1, invoice_no);
        insertStmt->setInt64(2, order_id);
        insertStmt->setString(3, order_no);
        insertStmt->setInt64(4, user_id);
        insertStmt->setString(5, username);
        insertStmt->setDouble(6, invoice_total);
        insertStmt->setString(7, items_json);
        insertStmt->executeUpdate();

        std::unique_ptr<sql::Statement> stmtResult(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT LAST_INSERT_ID() AS id"));
        if (res && res->next()) {
            int64_t id = res->getInt64("id");
            LOG_DEBUG("new invoice id: {} invoice_no: {}", id, invoice_no);
            return id;
        }
        return -1;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in generateInvoice: {}", exp.what());
        return -1;
    }
}

bool MySQLDao::getInvoiceDetail(int64_t id, InvoiceData& invoice) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "SELECT id, invoice_no, order_id, order_no, user_id, username, "
                "total_amount, items, generated_at "
                "FROM invoices WHERE id = ? LIMIT 1"));
        pstmt->setInt64(1, id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res || !res->next()) {
            return false;
        }
        invoice.id = res->getInt64("id");
        invoice.invoice_no = res->getString("invoice_no");
        invoice.order_id = res->getInt64("order_id");
        invoice.order_no = res->getString("order_no");
        invoice.user_id = res->getInt64("user_id");
        invoice.username = res->getString("username");
        invoice.total_amount = res->getDouble("total_amount");
        invoice.items = res->getString("items");
        invoice.generated_at = res->getString("generated_at");
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getInvoiceDetail: {}", exp.what());
        return false;
    }
}

// ==================== Statistics Operations ====================

bool MySQLDao::getStatsOverview(int& total_users, int& total_vehicles, int& available_vehicles,
                                 int& active_orders, int& completed_orders,
                                 double& total_revenue, double& month_revenue) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // Count users
        std::unique_ptr<sql::Statement> stmt1(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res1(stmt1->executeQuery("SELECT COUNT(*) AS cnt FROM user"));
        if (res1 && res1->next()) total_users = res1->getInt("cnt");

        // Count vehicles (total and available)
        std::unique_ptr<sql::Statement> stmt2(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res2(stmt2->executeQuery(
            "SELECT COUNT(*) AS total, "
            "SUM(CASE WHEN status = 'available' THEN 1 ELSE 0 END) AS available "
            "FROM vehicles"));
        if (res2 && res2->next()) {
            total_vehicles = res2->getInt("total");
            available_vehicles = res2->getInt("available");
        }

        // Count orders (active and completed)
        std::unique_ptr<sql::Statement> stmt3(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res3(stmt3->executeQuery(
            "SELECT "
            "SUM(CASE WHEN status = 'active' THEN 1 ELSE 0 END) AS active, "
            "SUM(CASE WHEN status = 'completed' THEN 1 ELSE 0 END) AS completed "
            "FROM rental_orders"));
        if (res3 && res3->next()) {
            active_orders = res3->getInt("active");
            completed_orders = res3->getInt("completed");
        }

        // Total revenue
        std::unique_ptr<sql::Statement> stmt4(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res4(stmt4->executeQuery(
            "SELECT COALESCE(SUM(total_cost), 0) AS total_revenue FROM rental_orders WHERE status = 'completed'"));
        if (res4 && res4->next()) total_revenue = res4->getDouble("total_revenue");

        // Month revenue
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_r(&tt, &tm);
        char monthBuf[8];
        snprintf(monthBuf, sizeof(monthBuf), "%04d-%02d", tm.tm_year + 1900, tm.tm_mon + 1);

        std::unique_ptr<sql::PreparedStatement> monthStmt(
            sql_conn->prepareStatement(
                "SELECT COALESCE(SUM(total_cost), 0) AS month_revenue FROM rental_orders "
                "WHERE status = 'completed' AND created_at LIKE ?"));
        monthStmt->setString(1, std::string(monthBuf) + "%");
        std::unique_ptr<sql::ResultSet> monthRes(monthStmt->executeQuery());
        if (monthRes && monthRes->next()) month_revenue = monthRes->getDouble("month_revenue");

        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getStatsOverview: {}", exp.what());
        return false;
    }
}

bool MySQLDao::getRevenueStats(const std::string& start_date, const std::string& end_date,
                                const std::string& granularity,
                                std::vector<RevenueStatsItemData>& items, double& total) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        std::string dateExpr;
        if (granularity == "monthly") {
            dateExpr = "DATE_FORMAT(created_at, '%Y-%m')";
        } else if (granularity == "weekly") {
            dateExpr = "DATE_FORMAT(DATE(created_at), '%x-W%v')";
        } else {
            dateExpr = "DATE(created_at)";
        }

        std::string querySql = "SELECT " + dateExpr + " AS date, "
                               "COALESCE(SUM(total_cost), 0) AS amount, "
                               "COUNT(*) AS count "
                               "FROM rental_orders "
                               "WHERE status = 'completed' "
                               "AND DATE(created_at) >= ? AND DATE(created_at) <= ? "
                               "GROUP BY date ORDER BY date";

        std::unique_ptr<sql::PreparedStatement> pstmt(sql_conn->prepareStatement(querySql));
        pstmt->setString(1, start_date);
        pstmt->setString(2, end_date);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        total = 0.0;
        while (res && res->next()) {
            RevenueStatsItemData item;
            item.date = res->getString("date");
            item.amount = res->getDouble("amount");
            item.count = res->getInt("count");
            total += item.amount;
            items.push_back(item);
        }
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getRevenueStats: {}", exp.what());
        return false;
    }
}

bool MySQLDao::getVehicleStats(int& total, int& available, int& rented, int& maintenance,
                                double& utilization_rate,
                                std::vector<VehicleStatsBrandData>& by_brand) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // Count by status
        std::unique_ptr<sql::Statement> stmt1(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res1(stmt1->executeQuery(
            "SELECT COUNT(*) AS total, "
            "SUM(CASE WHEN status = 'available' THEN 1 ELSE 0 END) AS available, "
            "SUM(CASE WHEN status = 'rented' THEN 1 ELSE 0 END) AS rented, "
            "SUM(CASE WHEN status = 'maintenance' THEN 1 ELSE 0 END) AS maintenance "
            "FROM vehicles"));
        if (res1 && res1->next()) {
            total = res1->getInt("total");
            available = res1->getInt("available");
            rented = res1->getInt("rented");
            maintenance = res1->getInt("maintenance");
            utilization_rate = total > 0 ? static_cast<double>(rented) / total * 100.0 : 0.0;
        }

        // Count by brand
        std::unique_ptr<sql::Statement> stmt2(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res2(stmt2->executeQuery(
            "SELECT brand, COUNT(*) AS count FROM vehicles GROUP BY brand ORDER BY count DESC"));
        while (res2 && res2->next()) {
            VehicleStatsBrandData item;
            item.brand = res2->getString("brand");
            item.count = res2->getInt("count");
            by_brand.push_back(item);
        }

        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getVehicleStats: {}", exp.what());
        return false;
    }
}
