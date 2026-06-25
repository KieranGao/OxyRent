# 车辆租赁管理系统设计文档

## 1. 项目概述

### 1.1 项目背景
本项目为大学软件工程课程综合实践项目，要求开发一个完整的车辆租赁管理系统。系统需要实现用户管理、车辆档案管理、租赁业务管理、车辆维保管理、收费结算管理、统计报表和客户功能等核心模块。

### 1.2 技术选型
- **后端架构：** 基于OxyTeamTasks的C++微服务架构
- **前端技术：** Vue3 + Element Plus + Pinia
- **数据库：** MySQL 8.0 + Redis 6.0+
- **通信协议：** gRPC（服务间） + HTTP/JSON（前后端）
- **部署方案：** Docker Compose容器化

### 1.3 开发周期
4周，分阶段实施

## 2. 系统架构设计

### 2.1 架构方案
采用简化微服务架构，包含以下服务：

```
┌─────────────────────────────────────────────────────────────┐
│                    前端层 (Vue3 + Element Plus)              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐      │
│  │ 用户管理 │ │ 车辆管理 │ │ 租赁业务 │ │ 统计报表 │      │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘      │
└─────────────────────────────────────────────────────────────┘
                              │ HTTP POST
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              网关层 (GateServer - C++)                      │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐                   │
│  │ HTTP处理 │ │ 请求路由 │ │ 协议转换 │                   │
│  └──────────┘ └──────────┘ └──────────┘                   │
└─────────────────────────────────────────────────────────────┘
                              │ gRPC
          ┌───────────────────┼───────────────────┐
          ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│   UMSServer     │ │  VehicleServer  │ │  FinanceServer  │
│   用户管理      │ │  车辆+租赁+维保 │ │  收费+报表      │
│   端口: 50053   │ │  端口: 50054    │ │  端口: 50055    │
└─────────────────┘ └─────────────────┘ └─────────────────┘
          │                   │                   │
          ▼                   ▼                   ▼
┌─────────────────────────────────────────────────────────────┐
│                      数据层                                 │
│  ┌─────────────────┐              ┌─────────────────┐      │
│  │    MySQL 8.0    │              │    Redis 6.0+   │      │
│  │   业务数据      │              │   缓存/会话     │      │
│  └─────────────────┘              └─────────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 服务职责

#### GateServer（网关服务）
- **端口：** 8080
- **职责：** HTTP请求处理、请求路由、协议转换（HTTP→gRPC）
- **复用：** OxyTeamTasks的GateServer代码

#### UMSServer（用户管理服务）
- **端口：** 50053
- **职责：** 用户注册、登录、个人信息管理、角色权限控制
- **数据库表：** users, user_profiles

#### VehicleServer（车辆服务）
- **端口：** 50054
- **职责：** 车辆档案管理、租赁订单管理、维修保养记录
- **数据库表：** vehicles, vehicle_documents, rental_orders, maintenance_records

#### FinanceServer（财务服务）
- **端口：** 50055
- **职责：** 收费结算、统计报表、账单生成
- **数据库表：** payments, invoices

### 2.3 通信机制
- **前端↔网关：** HTTP POST请求，JSON格式
- **网关↔微服务：** gRPC协议，Protobuf序列化
- **微服务↔数据库：** MySQL Connector/C++
- **缓存：** Redis用于会话管理、分布式锁

## 3. 数据库设计

### 3.1 实体关系图

```
┌─────────────┐     ┌─────────────┐
│    users    │────<│user_profiles│
└─────────────┘     └─────────────┘
       │
       │ 1:N
       ▼
┌─────────────┐     ┌─────────────┐
│rental_orders│────<│   payments  │
└─────────────┘     └─────────────┘
       │
       │ N:1
       ▼
┌─────────────┐     ┌─────────────┐
│   vehicles  │────<│vehicle_docs │
└─────────────┘     └─────────────┘
       │
       │ 1:N
       ▼
┌─────────────────┐
│maintenance_records│
└─────────────────┘
```

### 3.2 核心表结构

#### users（用户表）
```sql
CREATE TABLE users (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    phone VARCHAR(20),
    email VARCHAR(100),
    id_card VARCHAR(18),
    driver_license VARCHAR(20),
    role ENUM('customer', 'staff', 'admin') DEFAULT 'customer',
    status ENUM('pending', 'active', 'banned') DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

#### vehicles（车辆表）
```sql
CREATE TABLE vehicles (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    plate_number VARCHAR(20) UNIQUE NOT NULL,
    brand VARCHAR(50) NOT NULL,
    model VARCHAR(50) NOT NULL,
    color VARCHAR(20),
    year INT,
    mileage INT DEFAULT 0,
    status ENUM('available', 'rented', 'maintenance') DEFAULT 'available',
    daily_rate DECIMAL(10,2) NOT NULL,
    image_url VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

#### rental_orders（租赁订单表）
```sql
CREATE TABLE rental_orders (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    order_no VARCHAR(20) UNIQUE NOT NULL,
    user_id BIGINT NOT NULL,
    vehicle_id BIGINT NOT NULL,
    start_date DATE NOT NULL,
    end_date DATE NOT NULL,
    actual_return_date DATE,
    status ENUM('pending', 'active', 'completed', 'cancelled') DEFAULT 'pending',
    deposit DECIMAL(10,2) DEFAULT 0,
    total_cost DECIMAL(10,2) DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (vehicle_id) REFERENCES vehicles(id)
);
```

#### maintenance_records（维保记录表）
```sql
CREATE TABLE maintenance_records (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    vehicle_id BIGINT NOT NULL,
    type ENUM('repair', 'maintenance') NOT NULL,
    description TEXT,
    cost DECIMAL(10,2) DEFAULT 0,
    start_date DATE,
    end_date DATE,
    status ENUM('pending', 'in_progress', 'completed') DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (vehicle_id) REFERENCES vehicles(id)
);
```

#### payments（支付记录表）
```sql
CREATE TABLE payments (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    order_id BIGINT NOT NULL,
    amount DECIMAL(10,2) NOT NULL,
    type ENUM('deposit', 'rental', 'penalty') NOT NULL,
    method ENUM('cash', 'card', 'wechat', 'alipay') DEFAULT 'cash',
    status ENUM('pending', 'paid', 'refunded') DEFAULT 'pending',
    paid_at TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (order_id) REFERENCES rental_orders(id)
);
```

### 3.3 索引设计
```sql
-- 用户表索引
CREATE INDEX idx_users_phone ON users(phone);
CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_role ON users(role);

-- 车辆表索引
CREATE INDEX idx_vehicles_plate ON vehicles(plate_number);
CREATE INDEX idx_vehicles_status ON vehicles(status);
CREATE INDEX idx_vehicles_brand ON vehicles(brand);

-- 订单表索引
CREATE INDEX idx_orders_user ON rental_orders(user_id);
CREATE INDEX idx_orders_vehicle ON rental_orders(vehicle_id);
CREATE INDEX idx_orders_status ON rental_orders(status);
CREATE INDEX idx_orders_date ON rental_orders(start_date, end_date);
```

## 4. API接口设计

### 4.1 统一响应格式
```json
{
    "code": 0,
    "message": "success",
    "data": {}
}
```

### 4.2 认证机制
- JWT Token认证
- 通过Authorization头传递：`Authorization: Bearer <token>`
- Token有效期：24小时
- Redis存储会话信息

### 4.3 接口列表

#### 用户管理接口
| 接口 | 方法 | 说明 | 权限 |
|------|------|------|------|
| /user/register | POST | 用户注册 | 公开 |
| /user/login | POST | 用户登录 | 公开 |
| /user/profile | GET | 获取个人信息 | 登录用户 |
| /user/profile | PUT | 更新个人信息 | 登录用户 |
| /user/list | GET | 用户列表 | 管理员 |
| /user/{id}/status | PUT | 更新用户状态 | 管理员 |

#### 车辆管理接口
| 接口 | 方法 | 说明 | 权限 |
|------|------|------|------|
| /vehicle/list | GET | 车辆列表 | 登录用户 |
| /vehicle/{id} | GET | 车辆详情 | 登录用户 |
| /vehicle | POST | 添加车辆 | 管理员 |
| /vehicle/{id} | PUT | 更新车辆 | 管理员 |
| /vehicle/{id} | DELETE | 删除车辆 | 管理员 |

#### 租赁业务接口
| 接口 | 方法 | 说明 | 权限 |
|------|------|------|------|
| /rental/create | POST | 创建订单 | 客户 |
| /rental/{id}/pickup | POST | 取车登记 | 员工 |
| /rental/{id}/return | POST | 还车结算 | 员工 |
| /rental/{id}/renew | POST | 续租处理 | 员工 |
| /rental/list | GET | 订单列表 | 登录用户 |
| /rental/{id} | GET | 订单详情 | 登录用户 |

#### 维保管理接口
| 接口 | 方法 | 说明 | 权限 |
|------|------|------|------|
| /maintenance/create | POST | 创建维保记录 | 员工 |
| /maintenance/{id} | PUT | 更新维保状态 | 员工 |
| /maintenance/list | GET | 维保记录列表 | 员工 |
| /maintenance/{vehicle_id} | GET | 车辆维保历史 | 员工 |

#### 财务接口
| 接口 | 方法 | 说明 | 权限 |
|------|------|------|------|
| /payment/create | POST | 创建支付记录 | 员工 |
| /payment/{id} | GET | 支付详情 | 员工 |
| /invoice/generate | POST | 生成账单 | 员工 |
| /invoice/{id} | GET | 获取账单 | 员工 |
| /stats/overview | GET | 统计概览 | 管理员 |
| /stats/revenue | GET | 收入统计 | 管理员 |
| /stats/vehicles | GET | 车辆统计 | 管理员 |

## 5. 前端设计

### 5.1 技术栈
- **框架：** Vue 3 (Composition API + script setup)
- **UI库：** Element Plus
- **状态管理：** Pinia (持久化到localStorage)
- **路由：** Vue Router 4 (Hash模式)
- **图表：** ECharts
- **HTTP客户端：** Axios

### 5.2 页面结构
```
src/
├── views/
│   ├── auth/
│   │   ├── Login.vue          # 登录页
│   │   └── Register.vue       # 注册页
│   ├── dashboard/
│   │   └── Dashboard.vue      # 工作台
│   ├── vehicle/
│   │   ├── VehicleList.vue    # 车辆列表
│   │   ├── VehicleDetail.vue  # 车辆详情
│   │   └── VehicleForm.vue    # 车辆表单
│   ├── rental/
│   │   ├── RentalList.vue     # 订单列表
│   │   ├── RentalCreate.vue   # 创建订单
│   │   └── RentalDetail.vue   # 订单详情
│   ├── maintenance/
│   │   ├── MaintenanceList.vue # 维保列表
│   │   └── MaintenanceForm.vue # 维保表单
│   ├── finance/
│   │   ├── PaymentList.vue    # 支付记录
│   │   └── InvoiceList.vue    # 账单列表
│   ├── stats/
│   │   └── Statistics.vue     # 统计报表
│   └── user/
│       ├── UserProfile.vue    # 个人信息
│       └── UserList.vue       # 用户管理
├── stores/
│   ├── auth.js                # 认证状态
│   └── app.js                 # 应用状态
├── api/
│   ├── request.js             # Axios封装
│   ├── user.js                # 用户API
│   ├── vehicle.js             # 车辆API
│   ├── rental.js              # 租赁API
│   └── finance.js             # 财务API
├── router/
│   └── index.js               # 路由配置
└── utils/
    └── auth.js                # 认证工具
```

### 5.3 权限控制
- **路由守卫：** 根据用户角色动态生成路由
- **按钮权限：** 自定义指令控制按钮显示
- **角色定义：**
  - customer：客户，可查看车辆、创建订单
  - staff：员工，可管理车辆、处理订单
  - admin：管理员，所有权限

## 6. 开发计划

### 6.1 第1周：基础架构搭建
**目标：** 完成环境配置和基础服务

| 任务 | 交付物 | 验证方式 |
|------|--------|----------|
| Docker环境搭建 | docker-compose.yml | 服务可启动 |
| MySQL/Redis配置 | 数据库初始化脚本 | 数据库可连接 |
| GateServer搭建 | HTTP网关服务 | 可转发请求 |
| UMSServer开发 | 用户注册/登录 | 接口可调用 |
| Vue3项目初始化 | 前端项目框架 | 可访问登录页 |
| 登录/注册页面 | 认证界面 | 可完成登录 |

### 6.2 第2周：核心业务开发
**目标：** 完成车辆管理和租赁流程

| 任务 | 交付物 | 验证方式 |
|------|--------|----------|
| VehicleServer开发 | 车辆管理服务 | CRUD接口可用 |
| 车辆档案管理 | 车辆列表/详情页 | 可查看车辆 |
| 租赁订单管理 | 订单CRUD | 可创建订单 |
| 维保记录管理 | 维保CRUD | 可记录维保 |
| 前端页面开发 | 管理界面 | 可操作业务 |
| 前后端联调 | 完整流程 | 业务可运行 |

### 6.3 第3周：财务与统计
**目标：** 完成收费结算和统计报表

| 任务 | 交付物 | 验证方式 |
|------|--------|----------|
| FinanceServer开发 | 财务服务 | 收费接口可用 |
| 收费结算模块 | 结算流程 | 可完成结算 |
| 统计报表模块 | 报表接口 | 可查询统计 |
| ECharts图表 | 可视化报表 | 图表可展示 |
| 客户功能 | Web端预约 | 客户可下单 |
| 数据导出 | Excel导出 | 可导出数据 |

### 6.4 第4周：测试与文档
**目标：** 完成测试、部署和文档

| 任务 | 交付物 | 验证方式 |
|------|--------|----------|
| 功能测试 | 测试用例 | 功能正常 |
| 接口测试 | API测试 | 接口稳定 |
| Docker部署 | 部署脚本 | 可一键部署 |
| 需求规格说明书 | 需求文档 | 文档完整 |
| 概要设计说明书 | 设计文档 | 文档完整 |
| 详细设计说明书 | 设计文档 | 文档完整 |

## 7. 部署方案

### 7.1 Docker Compose配置
```yaml
version: '3.8'

services:
  # 前端
  vue3-client:
    build: ./Client
    ports:
      - "3000:80"
    depends_on:
      - gate-server

  # 网关服务
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

  # 用户管理服务
  ums-server:
    build: ./UMSServer
    ports:
      - "50053:50053"
    depends_on:
      - mysql
      - redis
    volumes:
      - ./UMSServer/config.ini:/etc/server/config.ini

  # 车辆服务
  vehicle-server:
    build: ./VehicleServer
    ports:
      - "50054:50054"
    depends_on:
      - mysql
      - redis
    volumes:
      - ./VehicleServer/config.ini:/etc/server/config.ini

  # 财务服务
  finance-server:
    build: ./FinanceServer
    ports:
      - "50055:50055"
    depends_on:
      - mysql
      - redis
    volumes:
      - ./FinanceServer/config.ini:/etc/server/config.ini

  # MySQL
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

  # Redis
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

### 7.2 启动命令
```bash
# 构建并启动所有服务
docker-compose up -d --build

# 查看服务状态
docker-compose ps

# 查看日志
docker-compose logs -f

# 停止所有服务
docker-compose down
```

## 8. 风险控制

### 8.1 技术风险
- **C++微服务复杂度：** 预留缓冲时间，优先实现核心功能
- **gRPC调试困难：** 使用grpcurl工具，添加详细日志
- **Docker环境问题：** 准备本地开发环境作为备选

### 8.2 进度风险
- **每周进度检查：** 及时调整计划
- **核心功能优先：** 优先实现用户管理和租赁流程
- **文档同步编写：** 开发过程中同步记录

### 8.3 集成风险
- **尽早联调：** 第2周开始前后端联调
- **接口先行：** 先定义接口，再实现功能
- **Mock数据：** 后端未完成时使用Mock数据

## 9. 五人分工方案（报告用）

### 9.1 角色分配
| 角色 | 人数 | 职责 |
|------|------|------|
| 项目经理 | 1 | 统筹协调、进度管理、文档审核 |
| 后端开发 | 2 | C++微服务开发、数据库设计 |
| 前端开发 | 1 | Vue3界面开发、交互实现 |
| 测试工程师 | 1 | 功能测试、接口测试、文档编写 |

### 9.2 详细分工

#### 项目经理（1人）
- 项目整体规划和进度管理
- 需求分析和文档审核
- 团队协调和风险控制
- 答辩准备和演示

#### 后端开发工程师1（1人）
- GateServer网关服务开发
- UMSServer用户管理服务开发
- 数据库设计和优化
- API接口定义

#### 后端开发工程师2（1人）
- VehicleServer车辆服务开发
- FinanceServer财务服务开发
- gRPC服务间通信
- Docker容器化部署

#### 前端开发工程师（1人）
- Vue3项目架构搭建
- 所有页面组件开发
- 前后端接口联调
- 用户体验优化

#### 测试工程师（1人）
- 测试用例设计和执行
- 接口自动化测试
- 需求规格说明书编写
- 概要设计和详细设计文档

## 10. 附录

### 10.1 项目结构
```
OxyPark/
├── Client/              # Vue3前端项目
├── GateServer/          # HTTP网关服务
├── UMSServer/           # 用户管理服务
├── VehicleServer/       # 车辆服务
├── FinanceServer/       # 财务服务
├── sql/                 # 数据库脚本
├── script/              # 运维脚本
├── docs/                # 项目文档
├── docker-compose.yml   # Docker编排配置
└── README.md            # 项目说明
```

### 10.2 环境要求
- Docker 20.10+
- Docker Compose 2.0+
- Node.js 18+（前端开发）
- C++17编译器（后端开发）

### 10.3 参考资料
- OxyTeamTasks项目架构
- Vue3官方文档
- Element Plus组件库
- gRPC官方文档
- Docker官方文档