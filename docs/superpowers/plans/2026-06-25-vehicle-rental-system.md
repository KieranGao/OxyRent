# 车辆租赁管理系统 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a complete vehicle rental management system with C++ microservices backend, Vue3 frontend, MySQL/Redis data layer, and Docker deployment.

**Architecture:** 4 C++ microservices (GateServer, UMSServer, VehicleServer, FinanceServer) communicating via gRPC, with a Vue3 + Element Plus frontend. Reuses OxyTeamTasks architecture patterns (Singleton, ConnectionPool, Boost.Beast HTTP, jsoncpp). MySQL for persistence, Redis for caching/sessions. Docker Compose for deployment.

**Tech Stack:** C++17, Boost.Beast, gRPC/Protobuf, MySQL Connector/C++, hiredis, Vue 3, Element Plus, Pinia, Axios, Docker Compose

---

## File Structure

```
OxyPark/
├── docker-compose.yml                    # Docker orchestration
├── sql/
│   ├── vehicle_rental.sql                # All tables + indexes + seed data
├── GateServer/
│   ├── CMakeLists.txt
│   ├── config.ini
│   ├── message.proto                     # gRPC service definitions
│   ├── main.cpp
│   ├── Global.h                          # Error codes, constants
│   ├── Singleton.h                       # CRTP singleton base (reuse from OxyTeamTasks)
│   ├── ConfigManager.h / .cpp            # INI config loader (reuse)
│   ├── Logger.h / .cpp                   # Logging (reuse, without Kafka)
│   ├── MySQLConnectPool.h / .cpp         # MySQL pool (reuse)
│   ├── RedisConnectPool.h / .cpp         # Redis pool (reuse)
│   ├── RedisManager.h / .cpp             # Redis operations (reuse)
│   ├── ConnectionGuard.h                 # RAII MySQL guard (reuse)
│   ├── Defer.h                           # RAII scope guard (reuse)
│   ├── IOContextPool.h / .cpp            # Boost.IO context pool (reuse)
│   ├── MainServer.h / .cpp               # TCP acceptor (reuse)
│   ├── HttpConnection.h / .cpp           # HTTP handler + auth (modify: new public paths)
│   ├── LogicSystem.h / .cpp              # Route registry (new routes)
│   ├── AsyncTaskPool.h / .cpp            # Thread pool for blocking ops (reuse)
│   ├── UserGrpcClient.h / .cpp           # GateServer -> UMSServer
│   ├── VehicleGrpcClient.h / .cpp        # GateServer -> VehicleServer (NEW)
│   ├── FinanceGrpcClient.h / .cpp        # GateServer -> FinanceServer (NEW)
│   ├── RPCConnectPool.h / .cpp           # gRPC stub pool (reuse)
│   ├── jsoncpp/                          # Embedded jsoncpp (reuse)
│   └── CMakeLists.txt
├── UMSServer/
│   ├── CMakeLists.txt
│   ├── config.ini
│   ├── main.cpp
│   ├── Global.h
│   ├── Singleton.h / ConfigManager.h / Logger.h / etc.  # Shared utilities (reuse)
│   ├── MySQLConnectPool.h / .cpp
│   ├── MySQLDao.h / .cpp                 # User SQL operations
│   ├── MySQLManager.h / .cpp             # Singleton wrapper
│   ├── RedisConnectPool.h / .cpp
│   ├── RedisManager.h / .cpp
│   ├── UMSGrpcServiceImpl.h / .cpp       # gRPC handlers
│   └── message.proto
├── VehicleServer/
│   ├── CMakeLists.txt
│   ├── config.ini
│   ├── main.cpp
│   ├── Global.h
│   ├── Shared utilities (reuse from UMSServer)
│   ├── MySQLDao.h / .cpp                 # Vehicle, order, maintenance SQL
│   ├── MySQLManager.h / .cpp
│   ├── VehicleGrpcServiceImpl.h / .cpp   # gRPC handlers
│   └── message.proto
├── FinanceServer/
│   ├── CMakeLists.txt
│   ├── config.ini
│   ├── main.cpp
│   ├── Global.h
│   ├── Shared utilities (reuse)
│   ├── MySQLDao.h / .cpp                 # Payment, invoice SQL
│   ├── MySQLManager.h / .cpp
│   ├── FinanceGrpcServiceImpl.h / .cpp   # gRPC handlers
│   └── message.proto
├── Client/
│   ├── package.json
│   ├── vite.config.js
│   ├── index.html
│   ├── src/
│   │   ├── main.js
│   │   ├── App.vue
│   │   ├── config/index.js
│   │   ├── router/index.js
│   │   ├── stores/auth.js
│   │   ├── stores/app.js
│   │   ├── api/request.js
│   │   ├── api/user.js
│   │   ├── api/vehicle.js
│   │   ├── api/rental.js
│   │   ├── api/maintenance.js
│   │   ├── api/finance.js
│   │   ├── utils/auth.js
│   │   ├── layout/AuthLayout.vue
│   │   ├── layout/MainLayout.vue
│   │   ├── styles/variables.css
│   │   ├── styles/global.css
│   │   └── views/ (Login, Register, Dashboard, VehicleList, VehicleDetail,
│   │               RentalList, RentalCreate, RentalDetail, MaintenanceList,
│   │               PaymentList, InvoiceList, Statistics, UserProfile, UserList)
│   └── Dockerfile
├── script/
│   ├── build_all.sh
│   ├── start_all.sh
│   └── stop_all.sh
└── README.md
```

---

## Task 1: Database Schema + Shared C++ Utilities

**Goal:** Create the MySQL schema and copy/adapt shared C++ utilities from OxyTeamTasks.

**Files:**
- Create: `sql/vehicle_rental.sql`
- Create: `GateServer/Singleton.h`, `Defer.h`, `ConnectionGuard.h`
- Create: `GateServer/ConfigManager.h`, `ConfigManager.cpp`
- Create: `GateServer/Logger.h`, `Logger.cpp`
- Create: `GateServer/MySQLConnectPool.h`, `MySQLConnectPool.cpp`
- Create: `GateServer/RedisConnectPool.h`, `RedisConnectPool.cpp`
- Create: `GateServer/RedisManager.h`, `RedisManager.cpp`
- Create: `GateServer/IOContextPool.h`, `IOContextPool.cpp`
- Create: `GateServer/AsyncTaskPool.h`, `AsyncTaskPool.cpp`
- Create: `GateServer/Global.h`

- [ ] **Step 1: Create database schema**

Create `sql/vehicle_rental.sql`:

```sql
-- 车辆租赁管理系统数据库
CREATE DATABASE IF NOT EXISTS vehicle_rental CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE vehicle_rental;

-- 用户表
CREATE TABLE users (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    phone VARCHAR(20),
    email VARCHAR(100),
    id_card VARCHAR(18),
    driver_license VARCHAR(20),
    real_name VARCHAR(50),
    gender TINYINT DEFAULT 0 COMMENT '0=unknown,1=male,2=female',
    birth_date DATE,
    address VARCHAR(255),
    avatar_url VARCHAR(255),
    role ENUM('customer','staff','admin') DEFAULT 'customer',
    status ENUM('pending','active','banned') DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE INDEX idx_users_phone ON users(phone);
CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_role ON users(role);
CREATE INDEX idx_users_status ON users(status);

-- 车辆表
CREATE TABLE vehicles (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    plate_number VARCHAR(20) UNIQUE NOT NULL,
    brand VARCHAR(50) NOT NULL,
    model VARCHAR(50) NOT NULL,
    color VARCHAR(20),
    year INT,
    mileage INT DEFAULT 0,
    status ENUM('available','rented','maintenance') DEFAULT 'available',
    daily_rate DECIMAL(10,2) NOT NULL,
    deposit_amount DECIMAL(10,2) DEFAULT 0,
    image_url VARCHAR(255),
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE INDEX idx_vehicles_plate ON vehicles(plate_number);
CREATE INDEX idx_vehicles_status ON vehicles(status);
CREATE INDEX idx_vehicles_brand ON vehicles(brand);

-- 车辆证件表
CREATE TABLE vehicle_documents (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    vehicle_id BIGINT NOT NULL,
    doc_type ENUM('driving_license','insurance','inspection') NOT NULL,
    doc_url VARCHAR(255),
    doc_number VARCHAR(100),
    issue_date DATE,
    expiry_date DATE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (vehicle_id) REFERENCES vehicles(id) ON DELETE CASCADE
);

CREATE INDEX idx_vdocs_vehicle ON vehicle_documents(vehicle_id);

-- 租赁订单表
CREATE TABLE rental_orders (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    order_no VARCHAR(20) UNIQUE NOT NULL,
    user_id BIGINT NOT NULL,
    vehicle_id BIGINT NOT NULL,
    start_date DATE NOT NULL,
    end_date DATE NOT NULL,
    actual_return_date DATE,
    status ENUM('pending','active','completed','cancelled') DEFAULT 'pending',
    deposit DECIMAL(10,2) DEFAULT 0,
    daily_rate DECIMAL(10,2) DEFAULT 0,
    total_days INT DEFAULT 0,
    total_cost DECIMAL(10,2) DEFAULT 0,
    penalty DECIMAL(10,2) DEFAULT 0,
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (vehicle_id) REFERENCES vehicles(id)
);

CREATE INDEX idx_orders_user ON rental_orders(user_id);
CREATE INDEX idx_orders_vehicle ON rental_orders(vehicle_id);
CREATE INDEX idx_orders_status ON rental_orders(status);
CREATE INDEX idx_orders_date ON rental_orders(start_date, end_date);
CREATE INDEX idx_orders_no ON rental_orders(order_no);

-- 维保记录表
CREATE TABLE maintenance_records (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    vehicle_id BIGINT NOT NULL,
    type ENUM('repair','maintenance') NOT NULL,
    description TEXT,
    cost DECIMAL(10,2) DEFAULT 0,
    technician VARCHAR(50),
    start_date DATE,
    end_date DATE,
    status ENUM('pending','in_progress','completed') DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (vehicle_id) REFERENCES vehicles(id)
);

CREATE INDEX idx_maint_vehicle ON maintenance_records(vehicle_id);
CREATE INDEX idx_maint_status ON maintenance_records(status);

-- 支付记录表
CREATE TABLE payments (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    order_id BIGINT NOT NULL,
    amount DECIMAL(10,2) NOT NULL,
    type ENUM('deposit','rental','penalty','refund') NOT NULL,
    method ENUM('cash','card','wechat','alipay') DEFAULT 'cash',
    status ENUM('pending','paid','refunded') DEFAULT 'pending',
    remark VARCHAR(255),
    paid_at TIMESTAMP NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (order_id) REFERENCES rental_orders(id)
);

CREATE INDEX idx_pay_order ON payments(order_id);
CREATE INDEX idx_pay_status ON payments(status);

-- 账单表
CREATE TABLE invoices (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    invoice_no VARCHAR(30) UNIQUE NOT NULL,
    order_id BIGINT NOT NULL,
    user_id BIGINT NOT NULL,
    total_amount DECIMAL(10,2) NOT NULL,
    items JSON,
    generated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (order_id) REFERENCES rental_orders(id),
    FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE INDEX idx_inv_order ON invoices(order_id);
CREATE INDEX idx_inv_user ON invoices(user_id);

-- 管理员种子数据 (密码: admin123, SHA-256)
INSERT INTO users (username, password_hash, phone, email, real_name, role, status)
VALUES ('admin', '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9',
        '13800000000', 'admin@rental.com', '系统管理员', 'admin', 'active');

-- 测试员工账户 (密码: staff123)
INSERT INTO users (username, password_hash, phone, email, real_name, role, status)
VALUES ('staff01', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92',
        '13800000001', 'staff01@rental.com', '张三', 'staff', 'active');

-- 测试客户账户 (密码: customer123)
INSERT INTO users (username, password_hash, phone, email, real_name, id_card, driver_license, role, status)
VALUES ('customer01', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f',
        '13800000002', 'customer01@rental.com', '李四', '110101199001011234',
        'C1234567890', 'customer', 'active');

-- 测试车辆数据
INSERT INTO vehicles (plate_number, brand, model, color, year, mileage, status, daily_rate, deposit_amount, description)
VALUES
('京A12345', '丰田', '卡罗拉', '白色', 2023, 15000, 'available', 150.00, 2000.00, '经济型轿车，省油耐用'),
('京B67890', '本田', '雅阁', '黑色', 2022, 28000, 'available', 200.00, 3000.00, '中型轿车，舒适商务'),
('京C11111', '大众', '途观L', '灰色', 2023, 8000, 'available', 280.00, 4000.00, 'SUV，空间大适合家庭'),
('京D22222', '宝马', '3系', '蓝色', 2024, 3000, 'rented', 350.00, 5000.00, '豪华轿车，动力强劲'),
('京E33333', '别克', 'GL8', '白色', 2022, 45000, 'maintenance', 320.00, 5000.00, '商务MPV，7座');
```

- [ ] **Step 2: Verify schema loads correctly**

Run: `mysql -u root -p < sql/vehicle_rental.sql`
Expected: No errors, database created with all tables and seed data.

- [ ] **Step 3: Create shared C++ utility headers**

Copy from OxyTeamTasks and adapt (remove Kafka dependencies):

Create `GateServer/Singleton.h`:
```cpp
#pragma once

template<class T>
class Singleton {
protected:
    Singleton() = default;
    ~Singleton() = default;
public:
    static T& getInstance() {
        static T instance;
        return instance;
    }
};
```

Create `GateServer/Defer.h`:
```cpp
#pragma once
#include <functional>

class Defer {
public:
    Defer(std::function<void()> func) : func_(std::move(func)) {}
    ~Defer() { if (func_) func_(); }
    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;
private:
    std::function<void()> func_;
};
```

- [ ] **Step 4: Create Global.h with error codes**

Create `GateServer/Global.h`:
```cpp
#pragma once
#include <cstdint>

enum class ErrorCodes : int32_t {
    SUCCESS = 0,
    JSON_PARSE_ERROR = 1001,
    RPC_ERROR = 1002,
    VERIFY_CODE_EXPIRED = 1003,
    USER_ALREADY_EXISTS = 1004,
    USER_NOT_FOUND = 1005,
    PASSWORD_ERROR = 1006,
    USER_NOT_APPROVED = 1007,
    TOKEN_INVALID = 1008,
    PERMISSION_DENIED = 1009,
    VEHICLE_NOT_FOUND = 1010,
    VEHICLE_NOT_AVAILABLE = 1011,
    ORDER_NOT_FOUND = 1012,
    ORDER_ALREADY_COMPLETED = 1013,
    MAINTENANCE_NOT_FOUND = 1014,
    PAYMENT_FAILED = 1015,
    INVALID_PARAMETER = 1016,
};
```

- [ ] **Step 5: Copy ConfigManager from OxyTeamTasks**

Copy `OxyTeamTasks/GateServer/ConfigManager.h` and `ConfigManager.cpp` to `GateServer/`. No modifications needed.

- [ ] **Step 6: Copy Logger from OxyTeamTasks (remove Kafka)**

Copy `OxyTeamTasks/GateServer/Logger.h` and `Logger.cpp`. Remove `RemoteFlushCallback` and Kafka-related code. Keep local file logging only.

- [ ] **Step 7: Copy MySQL connection pool**

Copy from OxyTeamTasks: `MySQLConnectPool.h`, `MySQLConnectPool.cpp`, `ConnectionGuard.h`.

- [ ] **Step 8: Copy Redis connection pool and manager**

Copy from OxyTeamTasks: `RedisConnectPool.h`, `RedisConnectPool.cpp`, `RedisManager.h`, `RedisManager.cpp`.

- [ ] **Step 9: Copy IOContextPool, AsyncTaskPool**

Copy from OxyTeamTasks: `IOContextPool.h`, `IOContextPool.cpp`, `AsyncTaskPool.h`, `AsyncTaskPool.cpp`.

- [ ] **Step 10: Commit**

```bash
git add sql/ GateServer/Singleton.h GateServer/Defer.h GateServer/Global.h \
  GateServer/ConfigManager.* GateServer/Logger.* GateServer/MySQLConnectPool.* \
  GateServer/ConnectionGuard.h GateServer/RedisConnectPool.* GateServer/RedisManager.* \
  GateServer/IOContextPool.* GateServer/AsyncTaskPool.*
git commit -m "feat: add database schema and shared C++ utilities"
```

---

## Task 2: GateServer HTTP Gateway + UMSServer User Service

**Goal:** Build the HTTP gateway and user management service with registration, login, profile, and admin user management.

**Files:**
- Create: `GateServer/message.proto`
- Create: `GateServer/MainServer.h`, `MainServer.cpp`
- Create: `GateServer/HttpConnection.h`, `HttpConnection.cpp`
- Create: `GateServer/LogicSystem.h`, `LogicSystem.cpp`
- Create: `GateServer/RPCConnectPool.h`, `RPCConnectPool.cpp`
- Create: `GateServer/UserGrpcClient.h`, `UserGrpcClient.cpp`
- Create: `GateServer/main.cpp`
- Create: `GateServer/CMakeLists.txt`
- Create: `GateServer/config.ini`
- Create: `UMSServer/` (all files)
- Modify: All shared utilities copied to UMSServer/

- [ ] **Step 1: Create gRPC proto file**

Create `GateServer/message.proto` (and identical copy in UMSServer):

```protobuf
syntax = "proto3";
package message;
option cpp_namespace = "message";

message CommonResponse {
    int32 error = 1;
    string msg = 2;
    string data = 3;
}

// User service
message UserRegisterRequest {
    string username = 1;
    string password = 2;
    string phone = 3;
    string email = 4;
}

message UserLoginRequest {
    string username = 1;
    string password = 2;
}

message UserLoginResponse {
    int32 error = 1;
    string msg = 2;
    int64 uid = 3;
    string token = 4;
    string role = 5;
}

message UserProfileRequest {
    int64 uid = 1;
}

message UserProfileResponse {
    int32 error = 1;
    string msg = 2;
    int64 uid = 3;
    string username = 4;
    string phone = 5;
    string email = 6;
    string id_card = 7;
    string driver_license = 8;
    string real_name = 9;
    int32 gender = 10;
    string birth_date = 11;
    string address = 12;
    string avatar_url = 13;
    string role = 14;
    string status = 15;
}

message UpdateProfileRequest {
    int64 uid = 1;
    string phone = 2;
    string email = 3;
    string id_card = 4;
    string driver_license = 5;
    string real_name = 6;
    int32 gender = 7;
    string birth_date = 8;
    string address = 9;
    string avatar_url = 10;
}

message UserListRequest {
    int32 page = 1;
    int32 page_size = 2;
    string keyword = 3;
    string role = 4;
    string status = 5;
}

message UserListItem {
    int64 uid = 1;
    string username = 2;
    string phone = 3;
    string email = 4;
    string real_name = 5;
    string role = 6;
    string status = 7;
    string created_at = 8;
}

message UserListResponse {
    int32 error = 1;
    string msg = 2;
    repeated UserListItem users = 3;
    int32 total = 4;
}

message UpdateUserStatusRequest {
    int64 uid = 1;
    string status = 2;
}

message UpdateUserRoleRequest {
    int64 uid = 1;
    string role = 2;
}

service UserService {
    rpc UserRegister(UserRegisterRequest) returns (CommonResponse);
    rpc UserLogin(UserLoginRequest) returns (UserLoginResponse);
    rpc GetUserProfile(UserProfileRequest) returns (UserProfileResponse);
    rpc UpdateProfile(UpdateProfileRequest) returns (CommonResponse);
    rpc GetUserList(UserListRequest) returns (UserListResponse);
    rpc UpdateUserStatus(UpdateUserStatusRequest) returns (CommonResponse);
    rpc UpdateUserRole(UpdateUserRoleRequest) returns (CommonResponse);
}
```

- [ ] **Step 2: Create GateServer MainServer**

Copy `MainServer.h` and `MainServer.cpp` from OxyTeamTasks. This handles TCP accept and distributes to IOContextPool. No changes needed.

- [ ] **Step 3: Create GateServer HttpConnection**

Copy from OxyTeamTasks and modify `PUBLIC_PATHS`:

```cpp
// In HttpConnection.h, modify PUBLIC_PATHS:
static const std::unordered_set<std::string> PUBLIC_PATHS = {
    "/user/login",
    "/user/register",
    "/get_test"
};
```

- [ ] **Step 4: Create RPCConnectPool template**

Copy `RPCConnectPool.h` and `RPCConnectPool.cpp` from OxyTeamTasks. This is the generic gRPC stub pool.

- [ ] **Step 5: Create UserGrpcClient**

Copy from OxyTeamTasks and adapt for vehicle rental user service:

```cpp
// GateServer/UserGrpcClient.h
#pragma once
#include <memory>
#include "Singleton.h"
#include "RPCConnectPool.h"
#include "message.grpc.pb.h"

using namespace message;

class UserGrpcClient : public Singleton<UserGrpcClient> {
    friend class Singleton<UserGrpcClient>;
public:
    CommonResponse UserRegister(const std::string& username, const std::string& password,
                                const std::string& phone, const std::string& email);
    UserLoginResponse UserLogin(const std::string& username, const std::string& password);
    UserProfileResponse GetUserProfile(int64_t uid);
    CommonResponse UpdateProfile(int64_t uid, const std::string& phone, const std::string& email,
                                 const std::string& id_card, const std::string& driver_license,
                                 const std::string& real_name, int32_t gender,
                                 const std::string& birth_date, const std::string& address,
                                 const std::string& avatar_url);
    UserListResponse GetUserList(int32_t page, int32_t page_size, const std::string& keyword,
                                 const std::string& role, const std::string& status);
    CommonResponse UpdateUserStatus(int64_t uid, const std::string& status);
    CommonResponse UpdateUserRole(int64_t uid, const std::string& role);
private:
    UserGrpcClient();
    std::shared_ptr<RPCConnectPool<UserService::Stub>> rpc_pool_;
};
```

- [ ] **Step 6: Create LogicSystem with all GateServer routes**

Create `GateServer/LogicSystem.cpp` with route handlers. Each handler: parse JSON -> call gRPC client -> write JSON response.

Route handlers to implement:
- `POST /user/register` -> UserGrpcClient::UserRegister
- `POST /user/login` -> UserGrpcClient::UserLogin
- `GET /user/profile` -> UserGrpcClient::GetUserProfile (reads uid from header)
- `PUT /user/profile` -> UserGrpcClient::UpdateProfile
- `GET /user/list` -> UserGrpcClient::GetUserList
- `PUT /user/{id}/status` -> UserGrpcClient::UpdateUserStatus
- `PUT /user/{id}/role` -> UserGrpcClient::UpdateUserRole

- [ ] **Step 7: Create GateServer main.cpp**

```cpp
#include "MainServer.h"
#include "ConfigManager.h"
#include "Logger.h"

int main() {
    auto& config = ConfigManager::getInstance();
    Logger::getInstance().init(
        config["Log"]["level"],
        config["Log"]["file_path"],
        config["GateServer"]["service_name"]
    );
    LOG_INFO("GateServer starting on {}:{}", config["GateServer"]["host"], config["GateServer"]["port"]);
    MainServer::getInstance().start();
    return 0;
}
```

- [ ] **Step 8: Create GateServer CMakeLists.txt**

Adapt from OxyTeamTasks, link against Boost, gRPC, Protobuf, hiredis, MySQL Connector, jsoncpp.

- [ ] **Step 9: Create GateServer config.ini**

```ini
[GateServer]
host = 0.0.0.0
port = 8080
service_name = GateServer

[MySQL]
host = mysql
port = 3306
user = root
password = root123
dbName = vehicle_rental
poolSize = 5

[Redis]
host = redis
port = 6379
password =
poolSize = 5

[UMSServer]
host = ums-server
port = 50053

[Log]
level = INFO
file_path = ../logs/
service_name = GateServer
flush_interval = 5
max_buffer_size = 100
```

- [ ] **Step 10: Create UMSServer (all files)**

Copy shared utilities from GateServer. Create:

`UMSServer/MySQLDao.h` and `MySQLDao.cpp`:
- `registerUser(username, password_hash, phone, email)` -> INSERT
- `loginUser(username, password_hash)` -> SELECT with status check
- `getUserProfile(uid)` -> SELECT all fields
- `updateProfile(uid, fields...)` -> UPDATE
- `getUserList(page, page_size, keyword, role, status)` -> SELECT with pagination
- `updateUserStatus(uid, status)` -> UPDATE
- `updateUserRole(uid, role)` -> UPDATE

`UMSServer/MySQLManager.h` and `MySQLManager.cpp`: Singleton wrapper.

`UMSServer/UMSGrpcServiceImpl.h` and `UMSGrpcServiceImpl.cpp`:
- Implements all 7 RPC methods from the proto service.

`UMSServer/main.cpp`: Server bootstrap (load config, init logger, start gRPC server).

`UMSServer/CMakeLists.txt`: Build configuration.

`UMSServer/config.ini`: MySQL/Redis/gRPC server config.

- [ ] **Step 11: Build and test GateServer + UMSServer**

```bash
cd GateServer && mkdir build && cd build && cmake .. && make -j$(nproc)
cd ../../UMSServer && mkdir build && cd build && cmake .. && make -j$(nproc)
```

- [ ] **Step 12: Commit**

```bash
git add GateServer/ UMSServer/
git commit -m "feat: implement GateServer gateway and UMSServer user management"
```

---

## Task 3: VehicleServer (Vehicle + Rental + Maintenance)

**Goal:** Build the vehicle management, rental order, and maintenance record services.

**Files:**
- Create: `VehicleServer/` (all files)
- Create: `GateServer/VehicleGrpcClient.h`, `VehicleGrpcClient.cpp`
- Modify: `GateServer/LogicSystem.cpp` (add vehicle/rental/maintenance routes)
- Modify: `GateServer/HttpConnection.h` (update PUBLIC_PATHS)

- [ ] **Step 1: Create VehicleServer proto file**

Create `VehicleServer/message.proto` with vehicle, order, and maintenance service definitions:

```protobuf
syntax = "proto3";
package message;
option cpp_namespace = "message";

message CommonResponse {
    int32 error = 1;
    string msg = 2;
    string data = 3;
}

// Vehicle messages
message VehicleInfo {
    int64 id = 1;
    string plate_number = 2;
    string brand = 3;
    string model = 4;
    string color = 5;
    int32 year = 6;
    int32 mileage = 7;
    string status = 8;
    double daily_rate = 9;
    double deposit_amount = 10;
    string image_url = 11;
    string description = 12;
    string created_at = 13;
}

message VehicleListRequest {
    int32 page = 1;
    int32 page_size = 2;
    string keyword = 3;
    string status = 4;
    string brand = 5;
}

message VehicleListResponse {
    int32 error = 1;
    string msg = 2;
    repeated VehicleInfo vehicles = 3;
    int32 total = 4;
}

message VehicleDetailRequest {
    int64 id = 1;
}

message AddVehicleRequest {
    string plate_number = 1;
    string brand = 2;
    string model = 3;
    string color = 4;
    int32 year = 5;
    int32 mileage = 6;
    double daily_rate = 7;
    double deposit_amount = 8;
    string image_url = 9;
    string description = 10;
}

message UpdateVehicleRequest {
    int64 id = 1;
    string plate_number = 2;
    string brand = 3;
    string model = 4;
    string color = 5;
    int32 year = 6;
    int32 mileage = 7;
    string status = 8;
    double daily_rate = 9;
    double deposit_amount = 10;
    string image_url = 11;
    string description = 12;
}

// Rental order messages
message OrderInfo {
    int64 id = 1;
    string order_no = 2;
    int64 user_id = 3;
    string username = 4;
    int64 vehicle_id = 5;
    string plate_number = 6;
    string vehicle_info = 7;
    string start_date = 8;
    string end_date = 9;
    string actual_return_date = 10;
    string status = 11;
    double deposit = 12;
    double daily_rate = 13;
    int32 total_days = 14;
    double total_cost = 15;
    double penalty = 16;
    string notes = 17;
    string created_at = 18;
}

message CreateOrderRequest {
    int64 user_id = 1;
    int64 vehicle_id = 2;
    string start_date = 3;
    string end_date = 4;
    string notes = 5;
}

message OrderListRequest {
    int32 page = 1;
    int32 page_size = 2;
    int64 user_id = 3;
    int64 vehicle_id = 4;
    string status = 5;
    string keyword = 6;
}

message OrderListResponse {
    int32 error = 1;
    string msg = 2;
    repeated OrderInfo orders = 3;
    int32 total = 4;
}

message OrderDetailRequest {
    int64 id = 1;
}

message PickupRequest {
    int64 order_id = 1;
}

message ReturnRequest {
    int64 order_id = 1;
}

message RenewRequest {
    int64 order_id = 1;
    string new_end_date = 2;
}

// Maintenance messages
message MaintenanceInfo {
    int64 id = 1;
    int64 vehicle_id = 2;
    string plate_number = 3;
    string type = 4;
    string description = 5;
    double cost = 6;
    string technician = 7;
    string start_date = 8;
    string end_date = 9;
    string status = 10;
    string created_at = 11;
}

message CreateMaintenanceRequest {
    int64 vehicle_id = 1;
    string type = 2;
    string description = 3;
    double cost = 4;
    string technician = 5;
    string start_date = 6;
}

message UpdateMaintenanceRequest {
    int64 id = 1;
    string description = 2;
    double cost = 3;
    string technician = 4;
    string end_date = 5;
    string status = 6;
}

message MaintenanceListRequest {
    int32 page = 1;
    int32 page_size = 2;
    int64 vehicle_id = 3;
    string status = 4;
    string type = 5;
}

message MaintenanceListResponse {
    int32 error = 1;
    string msg = 2;
    repeated MaintenanceInfo records = 3;
    int32 total = 4;
}

service VehicleService {
    // Vehicle CRUD
    rpc GetVehicleList(VehicleListRequest) returns (VehicleListResponse);
    rpc GetVehicleDetail(VehicleDetailRequest) returns (VehicleInfo);
    rpc AddVehicle(AddVehicleRequest) returns (CommonResponse);
    rpc UpdateVehicle(UpdateVehicleRequest) returns (CommonResponse);
    rpc DeleteVehicle(VehicleDetailRequest) returns (CommonResponse);

    // Rental orders
    rpc CreateOrder(CreateOrderRequest) returns (OrderInfo);
    rpc GetOrderList(OrderListRequest) returns (OrderListResponse);
    rpc GetOrderDetail(OrderDetailRequest) returns (OrderInfo);
    rpc PickupVehicle(PickupRequest) returns (CommonResponse);
    rpc ReturnVehicle(ReturnRequest) returns (OrderInfo);
    rpc RenewOrder(RenewRequest) returns (OrderInfo);

    // Maintenance
    rpc CreateMaintenance(CreateMaintenanceRequest) returns (CommonResponse);
    rpc UpdateMaintenance(UpdateMaintenanceRequest) returns (CommonResponse);
    rpc GetMaintenanceList(MaintenanceListRequest) returns (MaintenanceListResponse);
}
```

- [ ] **Step 2: Create VehicleServer MySQLDao**

Implement SQL operations for vehicles, orders, and maintenance_records tables. Key methods:
- Vehicle: list/detail/add/update/delete with pagination and filtering
- Order: create (generate order_no), list, detail, pickup (set status=active, vehicle=status=rented), return (calculate cost+penalty, update vehicle=available), renew
- Maintenance: create, update status, list with filtering

- [ ] **Step 3: Create VehicleGrpcServiceImpl**

Implement all 13 RPC methods. Business logic:
- `CreateOrder`: Validate vehicle available, calculate deposit, generate order_no (format: R + YYYYMMDD + 4-digit seq)
- `PickupVehicle`: Set order status=active, vehicle status=rented
- `ReturnVehicle`: Calculate actual days, total_cost, penalty (if late), set vehicle=available, order=completed
- `RenewOrder`: Validate new end_date > current end_date, recalculate cost

- [ ] **Step 4: Create VehicleGrpcClient for GateServer**

Similar pattern to UserGrpcClient. Client methods for all 13 RPCs.

- [ ] **Step 5: Add vehicle/rental/maintenance routes to GateServer**

Update `LogicSystem.cpp` with new route handlers:

Vehicle routes:
- `GET /vehicle/list` -> GetVehicleList
- `GET /vehicle/{id}` -> GetVehicleDetail
- `POST /vehicle` -> AddVehicle (admin only)
- `PUT /vehicle/{id}` -> UpdateVehicle (admin only)
- `DELETE /vehicle/{id}` -> DeleteVehicle (admin only)

Rental routes:
- `POST /rental/create` -> CreateOrder
- `GET /rental/list` -> GetOrderList
- `GET /rental/{id}` -> GetOrderDetail
- `POST /rental/{id}/pickup` -> PickupVehicle
- `POST /rental/{id}/return` -> ReturnVehicle
- `POST /rental/{id}/renew` -> RenewOrder

Maintenance routes:
- `POST /maintenance/create` -> CreateMaintenance
- `PUT /maintenance/{id}` -> UpdateMaintenance
- `GET /maintenance/list` -> GetMaintenanceList

- [ ] **Step 6: Build and test VehicleServer**

```bash
cd VehicleServer && mkdir build && cd build && cmake .. && make -j$(nproc)
```

- [ ] **Step 7: Commit**

```bash
git add VehicleServer/ GateServer/VehicleGrpcClient.* GateServer/LogicSystem.*
git commit -m "feat: implement VehicleServer with vehicle, rental, and maintenance services"
```

---

## Task 4: FinanceServer + Statistics

**Goal:** Build the payment/invoice service and statistics reporting.

**Files:**
- Create: `FinanceServer/` (all files)
- Create: `GateServer/FinanceGrpcClient.h`, `FinanceGrpcClient.cpp`
- Modify: `GateServer/LogicSystem.cpp` (add finance/stats routes)

- [ ] **Step 1: Create FinanceServer proto file**

Define payment, invoice, and statistics service RPCs:

```protobuf
syntax = "proto3";
package message;
option cpp_namespace = "message";

message CommonResponse {
    int32 error = 1;
    string msg = 2;
    string data = 3;
}

message PaymentInfo {
    int64 id = 1;
    int64 order_id = 2;
    string order_no = 3;
    double amount = 4;
    string type = 5;
    string method = 6;
    string status = 7;
    string remark = 8;
    string paid_at = 9;
    string created_at = 10;
}

message CreatePaymentRequest {
    int64 order_id = 1;
    double amount = 2;
    string type = 3;
    string method = 4;
    string remark = 5;
}

message PaymentListRequest {
    int32 page = 1;
    int32 page_size = 2;
    int64 order_id = 3;
    string status = 4;
    string type = 5;
}

message PaymentListResponse {
    int32 error = 1;
    string msg = 2;
    repeated PaymentInfo payments = 3;
    int32 total = 4;
}

message InvoiceInfo {
    int64 id = 1;
    string invoice_no = 2;
    int64 order_id = 3;
    string order_no = 4;
    int64 user_id = 5;
    string username = 6;
    double total_amount = 7;
    string items = 8;
    string generated_at = 9;
}

message GenerateInvoiceRequest {
    int64 order_id = 1;
}

message StatsOverviewResponse {
    int32 error = 1;
    string msg = 2;
    int32 total_users = 3;
    int32 total_vehicles = 4;
    int32 available_vehicles = 5;
    int32 active_orders = 6;
    int32 completed_orders = 7;
    double total_revenue = 8;
    double month_revenue = 9;
}

message RevenueStatsRequest {
    string start_date = 1;
    string end_date = 2;
    string granularity = 3;  // daily, weekly, monthly
}

message RevenueStatsItem {
    string date = 1;
    double amount = 2;
    int32 count = 3;
}

message RevenueStatsResponse {
    int32 error = 1;
    string msg = 2;
    repeated RevenueStatsItem items = 3;
    double total = 4;
}

message VehicleStatsResponse {
    int32 error = 1;
    string msg = 2;
    int32 total = 3;
    int32 available = 4;
    int32 rented = 5;
    int32 maintenance = 6;
    double utilization_rate = 7;
    repeated VehicleStatsBrandItem by_brand = 8;
}

message VehicleStatsBrandItem {
    string brand = 1;
    int32 count = 2;
}

service FinanceService {
    rpc CreatePayment(CreatePaymentRequest) returns (PaymentInfo);
    rpc GetPaymentList(PaymentListRequest) returns (PaymentListResponse);
    rpc GetPaymentDetail(PaymentInfo) returns (PaymentInfo);
    rpc GenerateInvoice(GenerateInvoiceRequest) returns (InvoiceInfo);
    rpc GetInvoiceDetail(InvoiceInfo) returns (InvoiceInfo);
    rpc GetStatsOverview(CommonResponse) returns (StatsOverviewResponse);
    rpc GetRevenueStats(RevenueStatsRequest) returns (RevenueStatsResponse);
    rpc GetVehicleStats(CommonResponse) returns (VehicleStatsResponse);
}
```

- [ ] **Step 2: Create FinanceServer MySQLDao**

Implement SQL operations for payments and invoices tables. Statistics queries using GROUP BY, SUM, COUNT.

- [ ] **Step 3: Create FinanceGrpcServiceImpl**

Business logic:
- `GenerateInvoice`: Create invoice from order, aggregate payment items as JSON
- `GetStatsOverview`: Aggregate queries across users, vehicles, orders, payments
- `GetRevenueStats`: Time-series query with date range and granularity
- `GetVehicleStats`: Count by status, count by brand

- [ ] **Step 4: Create FinanceGrpcClient for GateServer**

- [ ] **Step 5: Add finance/stats routes to GateServer**

- `POST /payment/create` -> CreatePayment
- `GET /payment/list` -> GetPaymentList
- `GET /payment/{id}` -> GetPaymentDetail
- `POST /invoice/generate` -> GenerateInvoice
- `GET /invoice/{id}` -> GetInvoiceDetail
- `GET /stats/overview` -> GetStatsOverview
- `GET /stats/revenue` -> GetRevenueStats
- `GET /stats/vehicles` -> GetVehicleStats

- [ ] **Step 6: Build and test FinanceServer**

- [ ] **Step 7: Commit**

```bash
git add FinanceServer/ GateServer/FinanceGrpcClient.* GateServer/LogicSystem.*
git commit -m "feat: implement FinanceServer with payment, invoice, and statistics"
```

---

## Task 5: Vue3 Frontend

**Goal:** Build the complete Vue3 + Element Plus frontend with all pages.

**Files:**
- Create: `Client/` (entire Vue3 project)

- [ ] **Step 1: Initialize Vue3 project**

```bash
cd Client
npm create vite@latest . -- --template vue
npm install element-plus @element-plus/icons-vue pinia vue-router@4 axios echarts vue-echarts
```

- [ ] **Step 2: Create config/index.js**

```javascript
export const API_BASE_URL = import.meta.env.DEV ? '/api' : 'http://localhost:8080'
```

- [ ] **Step 3: Create api/request.js**

```javascript
import axios from 'axios'
import { API_BASE_URL } from '@/config'
import { ElMessage } from 'element-plus'
import router from '@/router'

const request = axios.create({
  baseURL: API_BASE_URL,
  timeout: 10000,
})

request.interceptors.request.use(config => {
  const token = localStorage.getItem('token')
  const uid = localStorage.getItem('uid')
  if (token) {
    config.headers.Authorization = `Bearer ${token}`
    config.headers['X-User-Id'] = uid
  }
  return config
})

request.interceptors.response.use(
  response => {
    const data = response.data
    if (data.error !== 0) {
      ElMessage.error(data.msg || '请求失败')
      return Promise.reject(data)
    }
    return data
  },
  error => {
    if (error.response?.status === 401) {
      localStorage.clear()
      router.push('/login')
    }
    ElMessage.error(error.message || '网络错误')
    return Promise.reject(error)
  }
)

export default request
```

- [ ] **Step 4: Create API modules**

Create `api/user.js`, `api/vehicle.js`, `api/rental.js`, `api/maintenance.js`, `api/finance.js` with thin wrappers around `request.post/get/put/delete`.

- [ ] **Step 5: Create stores (Pinia)**

`stores/auth.js`: user state, login/logout/register actions, role helpers.
`stores/app.js`: theme, sidebar, page title.

- [ ] **Step 6: Create router with guards**

Routes with `meta.requiresAuth` and `meta.roles` for role-based access.

- [ ] **Step 7: Create layout components**

`AuthLayout.vue`: Login/register wrapper.
`MainLayout.vue`: Sidebar + top bar + content area.

- [ ] **Step 8: Create auth pages**

`Login.vue`: Username/password form, login button.
`Register.vue`: Registration form with validation.

- [ ] **Step 9: Create Dashboard**

`Dashboard.vue`: Stats cards (total users, available vehicles, active orders, monthly revenue), recent orders table.

- [ ] **Step 10: Create Vehicle pages**

`VehicleList.vue`: Filterable table with search, status filter, brand filter.
`VehicleDetail.vue`: Vehicle info display.
`VehicleForm.vue`: Add/edit vehicle dialog.

- [ ] **Step 11: Create Rental pages**

`RentalList.vue`: Order list with status filter.
`RentalCreate.vue`: Create order form (select vehicle, dates).
`RentalDetail.vue`: Order detail with pickup/return/renew actions.

- [ ] **Step 12: Create Maintenance pages**

`MaintenanceList.vue`: Maintenance record list.
`MaintenanceForm.vue`: Add/edit maintenance record.

- [ ] **Step 13: Create Finance pages**

`PaymentList.vue`: Payment records.
`InvoiceList.vue`: Invoice list with generate button.

- [ ] **Step 14: Create Statistics page**

`Statistics.vue`: ECharts charts - revenue trend line chart, vehicle status pie chart, brand distribution bar chart.

- [ ] **Step 15: Create User pages**

`UserProfile.vue`: Edit personal info.
`UserList.vue`: Admin user management table.

- [ ] **Step 16: Create styles**

`variables.css`: CSS custom properties.
`global.css`: Global styles, font imports.

- [ ] **Step 17: Configure Vite proxy**

```javascript
// vite.config.js
export default defineConfig({
  server: {
    proxy: {
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true,
        rewrite: path => path.replace(/^\/api/, '')
      }
    }
  }
})
```

- [ ] **Step 18: Test frontend**

```bash
cd Client && npm run dev
```
Expected: App starts on localhost:5173, login page loads.

- [ ] **Step 19: Commit**

```bash
git add Client/
git commit -m "feat: implement Vue3 frontend with all pages"
```

---

## Task 6: Docker Deployment + Build Scripts

**Goal:** Containerize all services and create build/start scripts.

**Files:**
- Create: `docker-compose.yml`
- Create: `GateServer/Dockerfile`
- Create: `UMSServer/Dockerfile`
- Create: `VehicleServer/Dockerfile`
- Create: `FinanceServer/Dockerfile`
- Create: `Client/Dockerfile`
- Create: `script/build_all.sh`, `start_all.sh`, `stop_all.sh`

- [ ] **Step 1: Create C++ Dockerfiles**

Each C++ service gets a multi-stage Dockerfile:

```dockerfile
FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y g++ cmake make \
    libboost-all-dev libprotobuf-dev protobuf-compiler \
    libgrpc++-dev protobuf-compiler-grpc \
    libhiredis-dev libmysqlcppconn-dev
WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake .. && make -j$(nproc)

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y libboost-system1.74.0 \
    libprotobuf23 libgrpc++1 libhiredis0.14 libmysqlcppconn8
COPY --from=builder /app/build/server /usr/local/bin/
COPY config.ini /etc/server/config.ini
CMD ["server", "--config", "/etc/server/config.ini"]
```

- [ ] **Step 2: Create Vue3 Dockerfile**

```dockerfile
FROM node:18-alpine AS builder
WORKDIR /app
COPY package*.json ./
RUN npm install
COPY . .
RUN npm run build

FROM nginx:alpine
COPY --from=builder /app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf
EXPOSE 80
```

- [ ] **Step 3: Create docker-compose.yml**

```yaml
version: '3.8'
services:
  vue3-client:
    build: ./Client
    ports:
      - "3000:80"
    depends_on:
      - gate-server

  gate-server:
    build: ./GateServer
    ports:
      - "8080:8080"
    depends_on:
      - ums-server
      - vehicle-server
      - finance-server
    volumes:
      - ./GateServer/config.ini:/etc/server/config.ini

  ums-server:
    build: ./UMSServer
    depends_on:
      - mysql
      - redis
    volumes:
      - ./UMSServer/config.ini:/etc/server/config.ini

  vehicle-server:
    build: ./VehicleServer
    depends_on:
      - mysql
      - redis
    volumes:
      - ./VehicleServer/config.ini:/etc/server/config.ini

  finance-server:
    build: ./FinanceServer
    depends_on:
      - mysql
      - redis
    volumes:
      - ./FinanceServer/config.ini:/etc/server/config.ini

  mysql:
    image: mysql:8.0
    ports:
      - "3306:3306"
    environment:
      MYSQL_ROOT_PASSWORD: root123
      MYSQL_DATABASE: vehicle_rental
    volumes:
      - mysql-data:/var/lib/mysql
      - ./sql:/docker-entrypoint-initdb.d

  redis:
    image: redis:6.0
    ports:
      - "6379:6379"
    volumes:
      - redis-data:/data

volumes:
  mysql-data:
  redis-data:
```

- [ ] **Step 4: Create build/start/stop scripts**

`script/build_all.sh`:
```bash
#!/bin/bash
set -e
echo "Building all services..."
cd GateServer && mkdir -p build && cd build && cmake .. && make -j$(nproc) && cd ../..
cd UMSServer && mkdir -p build && cd build && cmake .. && make -j$(nproc) && cd ../..
cd VehicleServer && mkdir -p build && cd build && cmake .. && make -j$(nproc) && cd ../..
cd FinanceServer && mkdir -p build && cd build && cmake .. && make -j$(nproc) && cd ../..
echo "All services built successfully!"
```

- [ ] **Step 5: Test Docker deployment**

```bash
docker-compose up -d --build
docker-compose ps
curl -X POST http://localhost:8080/user/login -H "Content-Type: application/json" -d '{"username":"admin","password":"admin123"}'
```

- [ ] **Step 6: Commit**

```bash
git add docker-compose.yml */Dockerfile script/
git commit -m "feat: add Docker deployment and build scripts"
```

---

## Task 7: Integration Testing + Polish

**Goal:** End-to-end testing, bug fixes, and final polish.

- [ ] **Step 1: Test complete user flow**

1. Register new user -> pending status
2. Admin approves user -> active status
3. User logs in -> gets token
4. User browses vehicle list
5. User creates rental order
6. Staff picks up vehicle
7. Staff processes return
8. Finance generates invoice
9. Admin views statistics

- [ ] **Step 2: Test error cases**

- Invalid credentials
- Duplicate registration
- Unauthorized access
- Vehicle not available
- Late return penalty calculation

- [ ] **Step 3: Add seed data for demo**

Insert more test vehicles, orders, and payments for presentation.

- [ ] **Step 4: Create README.md**

Project overview, architecture diagram, quick start guide, API documentation.

- [ ] **Step 5: Final commit**

```bash
git add -A
git commit -m "feat: complete vehicle rental management system"
```
