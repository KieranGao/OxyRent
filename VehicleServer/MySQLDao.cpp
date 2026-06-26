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

// ==================== 车辆操作 ====================

bool MySQLDao::getVehicleList(int page, int page_size, const std::string& keyword,
                               const std::string& status, const std::string& brand,
                               std::vector<VehicleData>& vehicles, int& total) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        std::string where = "1=1";
        std::vector<std::string> params;

        if (!keyword.empty()) {
            where += " AND (plate_number LIKE ? OR brand LIKE ? OR model LIKE ?)";
            std::string kw = "%" + keyword + "%";
            params.push_back(kw);
            params.push_back(kw);
            params.push_back(kw);
        }
        if (!status.empty()) {
            where += " AND status = ?";
            params.push_back(status);
        }
        if (!brand.empty()) {
            where += " AND brand = ?";
            params.push_back(brand);
        }

        // 统计总数
        std::string countSql = "SELECT COUNT(*) AS cnt FROM vehicles WHERE " + where;
        std::unique_ptr<sql::PreparedStatement> countStmt(sql_conn->prepareStatement(countSql));
        for (size_t i = 0; i < params.size(); ++i) {
            countStmt->setString(i + 1, params[i]);
        }
        std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());
        if (countRes && countRes->next()) {
            total = countRes->getInt("cnt");
        }

        // 分页查询
        std::string querySql = "SELECT id, plate_number, brand, model, color, year, mileage, "
                               "status, daily_rate, deposit_amount, image_url, description, created_at "
                               "FROM vehicles WHERE " + where + " ORDER BY id DESC LIMIT ? OFFSET ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(sql_conn->prepareStatement(querySql));
        for (size_t i = 0; i < params.size(); ++i) {
            pstmt->setString(i + 1, params[i]);
        }
        pstmt->setInt(params.size() + 1, page_size);
        pstmt->setInt(params.size() + 2, (page - 1) * page_size);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res && res->next()) {
            VehicleData item;
            item.id = res->getInt64("id");
            item.plate_number = res->getString("plate_number");
            item.brand = res->getString("brand");
            item.model = res->getString("model");
            item.color = res->getString("color");
            item.year = res->getInt("year");
            item.mileage = res->getInt("mileage");
            item.status = res->getString("status");
            item.daily_rate = res->getDouble("daily_rate");
            item.deposit_amount = res->getDouble("deposit_amount");
            item.image_url = res->getString("image_url");
            item.description = res->getString("description");
            item.created_at = res->getString("created_at");
            vehicles.push_back(item);
        }
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getVehicleList: {}", exp.what());
        return false;
    }
}

bool MySQLDao::getVehicleDetail(int64_t id, VehicleData& vehicle) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "SELECT id, plate_number, brand, model, color, year, mileage, "
                "status, daily_rate, deposit_amount, image_url, description, created_at "
                "FROM vehicles WHERE id = ? LIMIT 1"));
        pstmt->setInt64(1, id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res || !res->next()) {
            return false;
        }
        vehicle.id = res->getInt64("id");
        vehicle.plate_number = res->getString("plate_number");
        vehicle.brand = res->getString("brand");
        vehicle.model = res->getString("model");
        vehicle.color = res->getString("color");
        vehicle.year = res->getInt("year");
        vehicle.mileage = res->getInt("mileage");
        vehicle.status = res->getString("status");
        vehicle.daily_rate = res->getDouble("daily_rate");
        vehicle.deposit_amount = res->getDouble("deposit_amount");
        vehicle.image_url = res->getString("image_url");
        vehicle.description = res->getString("description");
        vehicle.created_at = res->getString("created_at");
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getVehicleDetail: {}", exp.what());
        return false;
    }
}

int64_t MySQLDao::addVehicle(const std::string& plate_number, const std::string& brand,
                              const std::string& model, const std::string& color,
                              int year, int mileage, double daily_rate, double deposit_amount,
                              const std::string& image_url, const std::string& description) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // 检查车牌号是否重复
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            sql_conn->prepareStatement("SELECT id FROM vehicles WHERE plate_number = ? LIMIT 1"));
        checkStmt->setString(1, plate_number);
        std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());
        if (checkRes && checkRes->next()) {
            LOG_WARN("Plate number already exists: {}", plate_number);
            return -1;
        }

        std::unique_ptr<sql::PreparedStatement> insertStmt(
            sql_conn->prepareStatement(
                "INSERT INTO vehicles (plate_number, brand, model, color, year, mileage, "
                "status, daily_rate, deposit_amount, image_url, description) "
                "VALUES (?, ?, ?, ?, ?, ?, 'available', ?, ?, ?, ?)"));
        insertStmt->setString(1, plate_number);
        insertStmt->setString(2, brand);
        insertStmt->setString(3, model);
        insertStmt->setString(4, color);
        insertStmt->setInt(5, year);
        insertStmt->setInt(6, mileage);
        insertStmt->setDouble(7, daily_rate);
        insertStmt->setDouble(8, deposit_amount);
        insertStmt->setString(9, image_url);
        insertStmt->setString(10, description);
        insertStmt->executeUpdate();

        std::unique_ptr<sql::Statement> stmtResult(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT LAST_INSERT_ID() AS id"));
        if (res && res->next()) {
            int64_t id = res->getInt64("id");
            LOG_DEBUG("new vehicle id: {}", id);
            return id;
        }
        return -1;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in addVehicle: {}", exp.what());
        return -1;
    }
}

bool MySQLDao::updateVehicle(int64_t id, const std::string& plate_number, const std::string& brand,
                              const std::string& model, const std::string& color,
                              int year, int mileage, const std::string& status,
                              double daily_rate, double deposit_amount,
                              const std::string& image_url, const std::string& description) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "UPDATE vehicles SET plate_number=?, brand=?, model=?, color=?, year=?, mileage=?, "
                "status=?, daily_rate=?, deposit_amount=?, image_url=?, description=? WHERE id=?"));
        pstmt->setString(1, plate_number);
        pstmt->setString(2, brand);
        pstmt->setString(3, model);
        pstmt->setString(4, color);
        pstmt->setInt(5, year);
        pstmt->setInt(6, mileage);
        pstmt->setString(7, status);
        pstmt->setDouble(8, daily_rate);
        pstmt->setDouble(9, deposit_amount);
        pstmt->setString(10, image_url);
        pstmt->setString(11, description);
        pstmt->setInt64(12, id);
        int affected = pstmt->executeUpdate();
        LOG_DEBUG("updateVehicle id={} affected={}", id, affected);
        return affected > 0;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in updateVehicle: {}", exp.what());
        return false;
    }
}

int MySQLDao::deleteVehicle(int64_t id) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // 删除前检查是否有未完成的订单
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            sql_conn->prepareStatement(
                "SELECT COUNT(*) AS cnt FROM rental_orders WHERE vehicle_id = ? AND status IN ('pending', 'active')"));
        checkStmt->setInt64(1, id);
        std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());
        if (checkRes && checkRes->next() && checkRes->getInt("cnt") > 0) {
            LOG_WARN("deleteVehicle: vehicle {} has active/pending orders, cannot delete", id);
            return -1;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement("DELETE FROM vehicles WHERE id = ?"));
        pstmt->setInt64(1, id);
        int affected = pstmt->executeUpdate();
        LOG_DEBUG("deleteVehicle id={} affected={}", id, affected);
        return affected > 0 ? 1 : 0;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in deleteVehicle: {}", exp.what());
        return 0;
    }
}

bool MySQLDao::updateVehicleStatus(int64_t id, const std::string& status) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement("UPDATE vehicles SET status = ? WHERE id = ?"));
        pstmt->setString(1, status);
        pstmt->setInt64(2, id);
        int affected = pstmt->executeUpdate();
        LOG_DEBUG("updateVehicleStatus id={} status={} affected={}", id, status, affected);
        return affected > 0;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in updateVehicleStatus: {}", exp.what());
        return false;
    }
}

bool MySQLDao::getVehicleStatus(int64_t id, std::string& status, double& daily_rate, double& deposit_amount) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "SELECT status, daily_rate, deposit_amount FROM vehicles WHERE id = ? LIMIT 1"));
        pstmt->setInt64(1, id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res || !res->next()) {
            return false;
        }
        status = res->getString("status");
        daily_rate = res->getDouble("daily_rate");
        deposit_amount = res->getDouble("deposit_amount");
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getVehicleStatus: {}", exp.what());
        return false;
    }
}

// ==================== 租赁订单操作 ====================

std::string MySQLDao::generateOrderNo() {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // 获取今日日期前缀
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_r(&tt, &tm);
        char dateBuf[16];
        snprintf(dateBuf, sizeof(dateBuf), "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        std::string datePrefix(dateBuf);

        // 获取今日订单数量用于序列号生成
        std::string countSql = "SELECT COUNT(*) AS cnt FROM rental_orders WHERE order_no LIKE ?";
        std::unique_ptr<sql::PreparedStatement> countStmt(sql_conn->prepareStatement(countSql));
        countStmt->setString(1, "R" + datePrefix + "%");
        std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());
        int seq = 1;
        if (countRes && countRes->next()) {
            seq = countRes->getInt("cnt") + 1;
        }

        char seqBuf[8];
        snprintf(seqBuf, sizeof(seqBuf), "%04d", seq);
        return "R" + datePrefix + std::string(seqBuf);
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in generateOrderNo: {}", exp.what());
        return "R00000000000000";
    }
}

int64_t MySQLDao::createOrder(int64_t user_id, int64_t vehicle_id,
                               const std::string& start_date, const std::string& end_date,
                               const std::string& notes, double deposit, double daily_rate,
                               int total_days) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::string order_no = generateOrderNo();
        double total_cost = total_days * daily_rate;

        std::unique_ptr<sql::PreparedStatement> insertStmt(
            sql_conn->prepareStatement(
                "INSERT INTO rental_orders (order_no, user_id, vehicle_id, start_date, end_date, "
                "status, deposit, daily_rate, total_days, total_cost, notes) "
                "VALUES (?, ?, ?, ?, ?, 'pending', ?, ?, ?, ?, ?)"));
        insertStmt->setString(1, order_no);
        insertStmt->setInt64(2, user_id);
        insertStmt->setInt64(3, vehicle_id);
        insertStmt->setString(4, start_date);
        insertStmt->setString(5, end_date);
        insertStmt->setDouble(6, deposit);
        insertStmt->setDouble(7, daily_rate);
        insertStmt->setInt(8, total_days);
        insertStmt->setDouble(9, total_cost);
        insertStmt->setString(10, notes);
        insertStmt->executeUpdate();

        std::unique_ptr<sql::Statement> stmtResult(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT LAST_INSERT_ID() AS id"));
        if (res && res->next()) {
            int64_t id = res->getInt64("id");
            LOG_DEBUG("new order id: {} order_no: {}", id, order_no);
            return id;
        }
        return -1;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in createOrder: {}", exp.what());
        return -1;
    }
}

bool MySQLDao::getOrderList(int page, int page_size, int64_t user_id, int64_t vehicle_id,
                             const std::string& status, const std::string& keyword,
                             std::vector<OrderData>& orders, int& total) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        std::string where = "1=1";
        std::vector<std::string> params;

        if (user_id > 0) {
            where += " AND o.user_id = ?";
            params.push_back(std::to_string(user_id));
        }
        if (vehicle_id > 0) {
            where += " AND o.vehicle_id = ?";
            params.push_back(std::to_string(vehicle_id));
        }
        if (!status.empty()) {
            where += " AND o.status = ?";
            params.push_back(status);
        }
        if (!keyword.empty()) {
            where += " AND (o.order_no LIKE ? OR u.username LIKE ? OR v.plate_number LIKE ?)";
            std::string kw = "%" + keyword + "%";
            params.push_back(kw);
            params.push_back(kw);
            params.push_back(kw);
        }

        // 统计总数
        std::string countSql = "SELECT COUNT(*) AS cnt FROM rental_orders o "
                               "LEFT JOIN user u ON o.user_id = u.uid "
                               "LEFT JOIN vehicles v ON o.vehicle_id = v.id "
                               "WHERE " + where;
        std::unique_ptr<sql::PreparedStatement> countStmt(sql_conn->prepareStatement(countSql));
        for (size_t i = 0; i < params.size(); ++i) {
            countStmt->setString(i + 1, params[i]);
        }
        std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());
        if (countRes && countRes->next()) {
            total = countRes->getInt("cnt");
        }

        // 分页查询
        std::string querySql = "SELECT o.id, o.order_no, o.user_id, u.username, o.vehicle_id, "
                               "v.plate_number, CONCAT(v.brand, ' ', v.model, ' ', v.color) AS vehicle_info, "
                               "o.start_date, o.end_date, o.actual_return_date, o.status, "
                               "o.deposit, o.daily_rate, o.total_days, o.total_cost, o.penalty, "
                               "o.notes, o.created_at "
                               "FROM rental_orders o "
                               "LEFT JOIN user u ON o.user_id = u.uid "
                               "LEFT JOIN vehicles v ON o.vehicle_id = v.id "
                               "WHERE " + where + " ORDER BY o.id DESC LIMIT ? OFFSET ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(sql_conn->prepareStatement(querySql));
        for (size_t i = 0; i < params.size(); ++i) {
            pstmt->setString(i + 1, params[i]);
        }
        pstmt->setInt(params.size() + 1, page_size);
        pstmt->setInt(params.size() + 2, (page - 1) * page_size);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res && res->next()) {
            OrderData item;
            item.id = res->getInt64("id");
            item.order_no = res->getString("order_no");
            item.user_id = res->getInt64("user_id");
            item.username = res->getString("username");
            item.vehicle_id = res->getInt64("vehicle_id");
            item.plate_number = res->getString("plate_number");
            item.vehicle_info = res->getString("vehicle_info");
            item.start_date = res->getString("start_date");
            item.end_date = res->getString("end_date");
            item.actual_return_date = res->getString("actual_return_date");
            item.status = res->getString("status");
            item.deposit = res->getDouble("deposit");
            item.daily_rate = res->getDouble("daily_rate");
            item.total_days = res->getInt("total_days");
            item.total_cost = res->getDouble("total_cost");
            item.penalty = res->getDouble("penalty");
            item.notes = res->getString("notes");
            item.created_at = res->getString("created_at");
            orders.push_back(item);
        }
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getOrderList: {}", exp.what());
        return false;
    }
}

bool MySQLDao::getOrderDetail(int64_t id, OrderData& order) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "SELECT o.id, o.order_no, o.user_id, u.username, o.vehicle_id, "
                "v.plate_number, CONCAT(v.brand, ' ', v.model, ' ', v.color) AS vehicle_info, "
                "o.start_date, o.end_date, o.actual_return_date, o.status, "
                "o.deposit, o.daily_rate, o.total_days, o.total_cost, o.penalty, "
                "o.notes, o.created_at "
                "FROM rental_orders o "
                "LEFT JOIN user u ON o.user_id = u.uid "
                "LEFT JOIN vehicles v ON o.vehicle_id = v.id "
                "WHERE o.id = ? LIMIT 1"));
        pstmt->setInt64(1, id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res || !res->next()) {
            return false;
        }
        order.id = res->getInt64("id");
        order.order_no = res->getString("order_no");
        order.user_id = res->getInt64("user_id");
        order.username = res->getString("username");
        order.vehicle_id = res->getInt64("vehicle_id");
        order.plate_number = res->getString("plate_number");
        order.vehicle_info = res->getString("vehicle_info");
        order.start_date = res->getString("start_date");
        order.end_date = res->getString("end_date");
        order.actual_return_date = res->getString("actual_return_date");
        order.status = res->getString("status");
        order.deposit = res->getDouble("deposit");
        order.daily_rate = res->getDouble("daily_rate");
        order.total_days = res->getInt("total_days");
        order.total_cost = res->getDouble("total_cost");
        order.penalty = res->getDouble("penalty");
        order.notes = res->getString("notes");
        order.created_at = res->getString("created_at");
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getOrderDetail: {}", exp.what());
        return false;
    }
}

bool MySQLDao::pickupVehicle(int64_t order_id, int64_t vehicle_id) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // 更新订单状态为'active'
        std::unique_ptr<sql::PreparedStatement> orderStmt(
            sql_conn->prepareStatement(
                "UPDATE rental_orders SET status = 'active' WHERE id = ? AND status = 'pending'"));
        orderStmt->setInt64(1, order_id);
        int affected = orderStmt->executeUpdate();
        if (affected == 0) {
            LOG_WARN("pickupVehicle: order {} not found or not in pending status", order_id);
            return false;
        }

        // 更新车辆状态为'rented'（vehicle_id由服务层传入）
        std::unique_ptr<sql::PreparedStatement> vehicleStmt(
            sql_conn->prepareStatement(
                "UPDATE vehicles SET status = 'rented' WHERE id = ?"));
        vehicleStmt->setInt64(1, vehicle_id);
        vehicleStmt->executeUpdate();

        LOG_INFO("pickupVehicle: order {} picked up", order_id);
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in pickupVehicle: {}", exp.what());
        return false;
    }
}

bool MySQLDao::returnVehicle(int64_t order_id, int64_t vehicle_id, const std::string& actual_return_date,
                              int actual_days, int planned_days, double daily_rate,
                              double& total_cost, double& penalty) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // 计算罚金和总费用
        penalty = 0.0;
        if (actual_days > planned_days) {
            penalty = (actual_days - planned_days) * daily_rate * 1.5;
        }
        total_cost = actual_days * daily_rate + penalty;

        // 更新订单
        std::unique_ptr<sql::PreparedStatement> orderStmt(
            sql_conn->prepareStatement(
                "UPDATE rental_orders SET status = 'completed', actual_return_date = ?, "
                "total_cost = ?, penalty = ? WHERE id = ? AND status = 'active'"));
        orderStmt->setString(1, actual_return_date);
        orderStmt->setDouble(2, total_cost);
        orderStmt->setDouble(3, penalty);
        orderStmt->setInt64(4, order_id);
        int affected = orderStmt->executeUpdate();
        if (affected == 0) {
            LOG_WARN("returnVehicle: order {} not found or not in active status", order_id);
            return false;
        }

        // 更新车辆状态为'available'（vehicle_id由服务层传入）
        std::unique_ptr<sql::PreparedStatement> vehicleStmt(
            sql_conn->prepareStatement(
                "UPDATE vehicles SET status = 'available' WHERE id = ?"));
        vehicleStmt->setInt64(1, vehicle_id);
        vehicleStmt->executeUpdate();

        LOG_INFO("returnVehicle: order {} returned, total_cost={}, penalty={}", order_id, total_cost, penalty);
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in returnVehicle: {}", exp.what());
        return false;
    }
}

bool MySQLDao::renewOrder(int64_t order_id, const std::string& new_end_date,
                           int new_total_days, double daily_rate, double& total_cost) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        total_cost = new_total_days * daily_rate;

        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "UPDATE rental_orders SET end_date = ?, total_days = ?, total_cost = ? "
                "WHERE id = ? AND status IN ('pending', 'active')"));
        pstmt->setString(1, new_end_date);
        pstmt->setInt(2, new_total_days);
        pstmt->setDouble(3, total_cost);
        pstmt->setInt64(4, order_id);
        int affected = pstmt->executeUpdate();
        if (affected == 0) {
            LOG_WARN("renewOrder: order {} not found or not in valid status", order_id);
            return false;
        }

        LOG_INFO("renewOrder: order {} renewed, new_end_date={}, total_cost={}", order_id, new_end_date, total_cost);
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in renewOrder: {}", exp.what());
        return false;
    }
}

// ==================== 维保操作 ====================

int64_t MySQLDao::createMaintenance(int64_t vehicle_id, const std::string& type,
                                     const std::string& description, double cost,
                                     const std::string& technician, const std::string& start_date) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();
        std::unique_ptr<sql::PreparedStatement> insertStmt(
            sql_conn->prepareStatement(
                "INSERT INTO maintenance_records (vehicle_id, type, description, cost, technician, "
                "start_date, status) VALUES (?, ?, ?, ?, ?, ?, 'in_progress')"));
        insertStmt->setInt64(1, vehicle_id);
        insertStmt->setString(2, type);
        insertStmt->setString(3, description);
        insertStmt->setDouble(4, cost);
        insertStmt->setString(5, technician);
        insertStmt->setString(6, start_date);
        insertStmt->executeUpdate();

        std::unique_ptr<sql::Statement> stmtResult(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT LAST_INSERT_ID() AS id"));
        if (res && res->next()) {
            int64_t id = res->getInt64("id");
            LOG_DEBUG("new maintenance record id: {}", id);
            return id;
        }
        return -1;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in createMaintenance: {}", exp.what());
        return -1;
    }
}

bool MySQLDao::updateMaintenance(int64_t id, const std::string& description, double cost,
                                  const std::string& technician, const std::string& end_date,
                                  const std::string& status) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        // 先查询当前维保记录的车辆ID和状态
        std::unique_ptr<sql::PreparedStatement> selectStmt(
            sql_conn->prepareStatement("SELECT vehicle_id, status FROM maintenance_records WHERE id=?"));
        selectStmt->setInt64(1, id);
        std::unique_ptr<sql::ResultSet> res(selectStmt->executeQuery());
        if (!res || !res->next()) {
            LOG_WARN("updateMaintenance: record {} not found", id);
            return false;
        }
        int64_t vehicle_id = res->getInt64("vehicle_id");
        std::string old_status = res->getString("status");

        // 更新维保记录
        std::unique_ptr<sql::PreparedStatement> pstmt(
            sql_conn->prepareStatement(
                "UPDATE maintenance_records SET description=?, cost=?, technician=?, "
                "end_date=?, status=? WHERE id=?"));
        pstmt->setString(1, description);
        pstmt->setDouble(2, cost);
        pstmt->setString(3, technician);
        // 处理空日期字符串，设置为NULL
        if (end_date.empty()) {
            pstmt->setNull(4, sql::DataType::DATE);
        } else {
            pstmt->setString(4, end_date);
        }
        pstmt->setString(5, status);
        pstmt->setInt64(6, id);
        int affected = pstmt->executeUpdate();

        // 同步车辆状态
        if (affected > 0) {
            if (status == "completed" && old_status != "completed") {
                // 维保完成：车辆从maintenance改为available
                std::unique_ptr<sql::PreparedStatement> vehicleStmt(
                    sql_conn->prepareStatement(
                        "UPDATE vehicles SET status='available' WHERE id=? AND status='maintenance'"));
                vehicleStmt->setInt64(1, vehicle_id);
                vehicleStmt->executeUpdate();
                LOG_INFO("updateMaintenance: vehicle {} status changed to available", vehicle_id);
            } else if (old_status == "completed" && status != "completed") {
                // 维保重新打开：车辆从available改回maintenance
                std::unique_ptr<sql::PreparedStatement> vehicleStmt(
                    sql_conn->prepareStatement(
                        "UPDATE vehicles SET status='maintenance' WHERE id=? AND status='available'"));
                vehicleStmt->setInt64(1, vehicle_id);
                vehicleStmt->executeUpdate();
                LOG_INFO("updateMaintenance: vehicle {} status changed back to maintenance", vehicle_id);
            }
        }

        LOG_DEBUG("updateMaintenance id={} affected={}", id, affected);
        return affected > 0;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in updateMaintenance: {}", exp.what());
        return false;
    }
}

bool MySQLDao::getMaintenanceList(int page, int page_size, int64_t vehicle_id,
                                   const std::string& status, const std::string& type,
                                   std::vector<MaintenanceData>& records, int& total) {
    auto connection = ConnectionGuard(*pool_, pool_->getConnection());
    try {
        auto& sql_conn = connection.get()->getConn();

        std::string where = "1=1";
        std::vector<std::string> params;

        if (vehicle_id > 0) {
            where += " AND m.vehicle_id = ?";
            params.push_back(std::to_string(vehicle_id));
        }
        if (!status.empty()) {
            where += " AND m.status = ?";
            params.push_back(status);
        }
        if (!type.empty()) {
            where += " AND m.type = ?";
            params.push_back(type);
        }

        // 统计总数
        std::string countSql = "SELECT COUNT(*) AS cnt FROM maintenance_records m WHERE " + where;
        std::unique_ptr<sql::PreparedStatement> countStmt(sql_conn->prepareStatement(countSql));
        for (size_t i = 0; i < params.size(); ++i) {
            countStmt->setString(i + 1, params[i]);
        }
        std::unique_ptr<sql::ResultSet> countRes(countStmt->executeQuery());
        if (countRes && countRes->next()) {
            total = countRes->getInt("cnt");
        }

        // 分页查询
        std::string querySql = "SELECT m.id, m.vehicle_id, v.plate_number, m.type, m.description, "
                               "m.cost, m.technician, m.start_date, m.end_date, m.status, m.created_at "
                               "FROM maintenance_records m "
                               "LEFT JOIN vehicles v ON m.vehicle_id = v.id "
                               "WHERE " + where + " ORDER BY m.id DESC LIMIT ? OFFSET ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(sql_conn->prepareStatement(querySql));
        for (size_t i = 0; i < params.size(); ++i) {
            pstmt->setString(i + 1, params[i]);
        }
        pstmt->setInt(params.size() + 1, page_size);
        pstmt->setInt(params.size() + 2, (page - 1) * page_size);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res && res->next()) {
            MaintenanceData item;
            item.id = res->getInt64("id");
            item.vehicle_id = res->getInt64("vehicle_id");
            item.plate_number = res->getString("plate_number");
            item.type = res->getString("type");
            item.description = res->getString("description");
            item.cost = res->getDouble("cost");
            item.technician = res->getString("technician");
            item.start_date = res->getString("start_date");
            item.end_date = res->getString("end_date");
            item.status = res->getString("status");
            item.created_at = res->getString("created_at");
            records.push_back(item);
        }
        return true;
    } catch(const sql::SQLException& exp) {
        LOG_ERROR("SQLException in getMaintenanceList: {}", exp.what());
        return false;
    }
}
