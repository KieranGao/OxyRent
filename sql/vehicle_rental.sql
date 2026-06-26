-- ============================================================
-- OxyPark Vehicle Rental System - Database Schema
-- ============================================================

CREATE DATABASE IF NOT EXISTS vehicle_rental
    DEFAULT CHARACTER SET utf8mb4
    DEFAULT COLLATE utf8mb4_unicode_ci;

USE vehicle_rental;

-- -----------------------------------------------------------
-- Users
-- -----------------------------------------------------------
CREATE TABLE IF NOT EXISTS user (
    uid             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    username        VARCHAR(64)     NOT NULL,
    password        VARCHAR(128)    NOT NULL,
    email           VARCHAR(128)    DEFAULT NULL,
    phone           VARCHAR(20)     DEFAULT NULL,
    real_name       VARCHAR(64)     DEFAULT NULL,
    id_card         VARCHAR(32)     DEFAULT NULL,
    driver_license  VARCHAR(64)     DEFAULT NULL,
    gender          INT             DEFAULT 0,
    birth_date      VARCHAR(20)     DEFAULT NULL,
    address         VARCHAR(256)    DEFAULT NULL,
    avatar_url      VARCHAR(512)    DEFAULT NULL,
    balance         DECIMAL(10,2)   DEFAULT 0.00 COMMENT '账户余额',
    role            VARCHAR(20)     NOT NULL DEFAULT 'customer',
    status          VARCHAR(20)     NOT NULL DEFAULT 'pending',
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (uid),
    UNIQUE KEY uk_username (username),
    KEY idx_role (role),
    KEY idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- -----------------------------------------------------------
-- Vehicles
-- -----------------------------------------------------------
CREATE TABLE IF NOT EXISTS vehicles (
    id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    plate_number    VARCHAR(20)     NOT NULL,
    brand           VARCHAR(64)     NOT NULL,
    model           VARCHAR(64)     NOT NULL,
    color           VARCHAR(32)     DEFAULT NULL,
    year            INT UNSIGNED    DEFAULT NULL,
    mileage         INT UNSIGNED    DEFAULT 0,
    daily_rate      DECIMAL(10,2)   NOT NULL DEFAULT 0.00,
    deposit_amount  DECIMAL(10,2)   NOT NULL DEFAULT 0.00,
    status          ENUM('available','rented','maintenance') NOT NULL DEFAULT 'available',
    image_url       VARCHAR(512)    DEFAULT NULL,
    description     TEXT            DEFAULT NULL,
    location        VARCHAR(128)    DEFAULT NULL,
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_plate_number (plate_number),
    KEY idx_status (status),
    KEY idx_brand_model (brand, model)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- -----------------------------------------------------------
-- Vehicle Documents (insurance, registration, inspection, etc.)
-- -----------------------------------------------------------
CREATE TABLE IF NOT EXISTS vehicle_documents (
    id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    vehicle_id      BIGINT UNSIGNED NOT NULL,
    doc_type        VARCHAR(32)     NOT NULL COMMENT 'insurance / registration / inspection / other',
    doc_name        VARCHAR(128)    NOT NULL,
    file_path       VARCHAR(512)    NOT NULL,
    expire_date     DATE            DEFAULT NULL,
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_vehicle_id (vehicle_id),
    CONSTRAINT fk_doc_vehicle FOREIGN KEY (vehicle_id) REFERENCES vehicles(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- -----------------------------------------------------------
-- Rental Orders
-- -----------------------------------------------------------
CREATE TABLE IF NOT EXISTS rental_orders (
    id                  BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    order_no            VARCHAR(32)     NOT NULL,
    user_id             BIGINT UNSIGNED NOT NULL,
    vehicle_id          BIGINT UNSIGNED NOT NULL,
    start_date          DATETIME        NOT NULL,
    end_date            DATETIME        NOT NULL,
    actual_return_date  DATETIME        DEFAULT NULL,
    status              ENUM('pending','active','completed','cancelled') NOT NULL DEFAULT 'pending',
    deposit             DECIMAL(10,2)   NOT NULL DEFAULT 0.00,
    daily_rate          DECIMAL(10,2)   NOT NULL DEFAULT 0.00,
    total_days          INT UNSIGNED    NOT NULL DEFAULT 0,
    total_cost          DECIMAL(10,2)   NOT NULL DEFAULT 0.00,
    penalty             DECIMAL(10,2)   NOT NULL DEFAULT 0.00,
    notes               TEXT            DEFAULT NULL,
    created_at          DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at          DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_order_no (order_no),
    KEY idx_user_id (user_id),
    KEY idx_vehicle_id (vehicle_id),
    KEY idx_status (status),
    KEY idx_start_date (start_date),
    CONSTRAINT fk_order_user FOREIGN KEY (user_id) REFERENCES user(uid),
    CONSTRAINT fk_order_vehicle FOREIGN KEY (vehicle_id) REFERENCES vehicles(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- -----------------------------------------------------------
-- Maintenance Records
-- -----------------------------------------------------------
CREATE TABLE IF NOT EXISTS maintenance_records (
    id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    vehicle_id      BIGINT UNSIGNED NOT NULL,
    type            ENUM('repair','maintenance') NOT NULL,
    description     TEXT            DEFAULT NULL,
    cost            DECIMAL(10,2)   NOT NULL DEFAULT 0.00,
    start_date      DATE            NOT NULL,
    end_date        DATE            DEFAULT NULL,
    technician      VARCHAR(64)     DEFAULT NULL,
    status          ENUM('pending','in_progress','completed') NOT NULL DEFAULT 'pending',
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_vehicle_id (vehicle_id),
    KEY idx_type (type),
    CONSTRAINT fk_maint_vehicle FOREIGN KEY (vehicle_id) REFERENCES vehicles(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- -----------------------------------------------------------
-- Payments
-- -----------------------------------------------------------
-- NOTE: payments.order_no references rental_orders.order_no but has no FK constraint
-- because payments may also apply to non-rental charges (e.g. penalties billed separately).
CREATE TABLE IF NOT EXISTS payments (
    id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    order_id        BIGINT UNSIGNED NOT NULL,
    order_no        VARCHAR(32)     NOT NULL,
    user_id         BIGINT UNSIGNED NOT NULL,
    amount          DECIMAL(10,2)   NOT NULL,
    type            ENUM('deposit','rental','penalty','refund') NOT NULL,
    method          VARCHAR(32)     DEFAULT 'online' COMMENT 'online / cash / other',
    status          ENUM('pending','success','failed') NOT NULL DEFAULT 'pending',
    remark          TEXT            DEFAULT NULL,
    paid_at         TIMESTAMP       NULL DEFAULT NULL,
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_order_id (order_id),
    KEY idx_order_no (order_no),
    KEY idx_user_id (user_id),
    KEY idx_type (type),
    KEY idx_status (status),
    CONSTRAINT fk_payment_user FOREIGN KEY (user_id) REFERENCES user(uid),
    CONSTRAINT fk_payment_order FOREIGN KEY (order_id) REFERENCES rental_orders(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- -----------------------------------------------------------
-- Invoices
-- -----------------------------------------------------------
-- NOTE: invoices.order_no references rental_orders.order_no but has no FK constraint
-- because invoices may be issued for payments that span multiple orders.
CREATE TABLE IF NOT EXISTS invoices (
    id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    invoice_no      VARCHAR(32)     NOT NULL,
    order_id        BIGINT UNSIGNED NOT NULL,
    order_no        VARCHAR(32)     NOT NULL,
    user_id         BIGINT UNSIGNED NOT NULL,
    username        VARCHAR(50)     DEFAULT NULL,
    total_amount    DECIMAL(10,2)   NOT NULL,
    items           JSON            DEFAULT NULL,
    title           VARCHAR(128)    DEFAULT NULL,
    tax_id          VARCHAR(64)     DEFAULT NULL,
    status          ENUM('pending','issued','cancelled') NOT NULL DEFAULT 'pending',
    issued_at       DATETIME        DEFAULT NULL,
    generated_at    TIMESTAMP       DEFAULT CURRENT_TIMESTAMP,
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_invoice_no (invoice_no),
    KEY idx_order_id (order_id),
    KEY idx_order_no (order_no),
    KEY idx_user_id (user_id),
    CONSTRAINT fk_invoice_user FOREIGN KEY (user_id) REFERENCES user(uid),
    CONSTRAINT fk_invoice_order FOREIGN KEY (order_id) REFERENCES rental_orders(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Balance Records
-- ============================================================
CREATE TABLE IF NOT EXISTS balance_records (
    id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    user_id         BIGINT UNSIGNED NOT NULL,
    amount          DECIMAL(10,2)   NOT NULL COMMENT '充值金额（正数）',
    type            ENUM('topup','consume') NOT NULL COMMENT 'topup=充值, consume=消费',
    operator_id     BIGINT UNSIGNED COMMENT '操作人ID（充值时）',
    remark          VARCHAR(255)    COMMENT '备注',
    created_at      TIMESTAMP       DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_user_id (user_id),
    CONSTRAINT fk_balance_user FOREIGN KEY (user_id) REFERENCES user(uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Seed Data
-- ============================================================
-- NOTE: Seed passwords below are stored as MD5 hashes for demo convenience.

-- Admin user (admin / admin123)
INSERT INTO user (username, password, email, real_name, role, status, balance)
VALUES ('admin', '0192023a7bbd73250516f069df18b500', 'admin@oxypark.com', 'System Admin', 'admin', 'active', 0.00);

-- Staff user (staff01 / staff123)
INSERT INTO user (username, password, email, real_name, role, status, balance)
VALUES ('staff01', 'de9bf5643eabf80f4a56fda3bbb84483', 'staff01@oxypark.com', 'Staff One', 'staff', 'active', 0.00);

-- Customer user (customer01 / customer123)
INSERT INTO user (username, password, email, real_name, role, status, balance)
VALUES ('customer01', 'f4ad231214cb99a985dff0f056a36242', 'customer01@example.com', 'Customer One', 'customer', 'active', 5000.00);

-- 5 Test Vehicles
INSERT INTO vehicles (plate_number, brand, model, color, year, mileage, daily_rate, deposit_amount, status, image_url, description, location) VALUES
('京A-12345', 'Toyota',    'Camry',     'White',  2023, 15000, 280.00, 5000.00, 'available', NULL, '经济实用型轿车', 'Slot-A01'),
('京B-67890', 'Honda',     'Civic',     'Black',  2022, 32000, 220.00, 4000.00, 'available', NULL, '运动型紧凑轿车', 'Slot-A02'),
('京C-11111', 'Tesla',     'Model 3',   'Red',    2024,  5000, 450.00, 8000.00, 'available', NULL, '纯电动智能轿车', 'Slot-B01'),
('京D-22222', 'BYD',       'Han EV',    'Blue',   2023, 20000, 350.00, 6000.00, 'maintenance', NULL, '国产新能源旗舰', 'Workshop'),
('京E-33333', 'Volkswagen','Lavida',     'Silver', 2021, 48000, 180.00, 3000.00, 'available', NULL, '经典家用轿车', 'Slot-C01');

-- Additional vehicles
INSERT INTO vehicles (plate_number, brand, model, color, year, mileage, daily_rate, deposit_amount, status, image_url, description, location) VALUES
('京F-44444', 'Mercedes-Benz','C-Class',   'White',  2024,  5000, 400.00, 6000.00, 'available',  NULL, '豪华轿车，商务首选', 'Slot-D01'),
('京G-55555', 'Audi',         'A4L',       'Black',  2023, 18000, 350.00, 5000.00, 'available',  NULL, '中型豪华轿车', 'Slot-D02'),
('京H-66666', 'Tesla',        'Model 3',   'Red',    2024,  2000, 300.00, 4000.00, 'available',  NULL, '纯电动，环保出行', 'Slot-E01'),
('京J-77777', 'BYD',          'Han EV',    'Blue',   2023, 12000, 250.00, 3000.00, 'rented',     NULL, '国产新能源旗舰', 'Slot-E02'),
('京K-88888', 'Volkswagen',   'Lavida',    'White',  2022, 35000, 120.00, 1500.00, 'available',  NULL, '经济实惠之选', 'Slot-F01'),
('京L-99999', 'Toyota',       'Camry',     'Silver', 2023, 22000, 180.00, 2500.00, 'maintenance',NULL, '中型轿车，舒适稳定', 'Workshop');

-- Additional rental orders
-- Order for completed rental (customer01 rented京A-12345, returned on time)
INSERT INTO rental_orders (order_no, user_id, vehicle_id, start_date, end_date, actual_return_date, status, deposit, daily_rate, total_days, total_cost, penalty, notes)
VALUES ('R20260620001', 3, 1, '2026-06-20 10:00:00', '2026-06-25 10:00:00', '2026-06-25 09:30:00', 'completed', 5000.00, 280.00, 5, 1400.00, 0.00, '正常归还，无逾期');

-- Order for active rental (customer01 rented京B-67890, currently out)
INSERT INTO rental_orders (order_no, user_id, vehicle_id, start_date, end_date, actual_return_date, status, deposit, daily_rate, total_days, total_cost, penalty, notes)
VALUES ('R20260622001', 3, 2, '2026-06-22 09:00:00', '2026-06-28 09:00:00', NULL, 'active', 4000.00, 220.00, 6, 0.00, 0.00, '商务出行，预计6月28日归还');

-- Order for completed rental with late return (customer01 rented京C-11111, returned 1 day late)
INSERT INTO rental_orders (order_no, user_id, vehicle_id, start_date, end_date, actual_return_date, status, deposit, daily_rate, total_days, total_cost, penalty, notes)
VALUES ('R20260623001', 3, 3, '2026-06-23 11:00:00', '2026-06-26 11:00:00', '2026-06-27 14:00:00', 'completed', 8000.00, 450.00, 4, 1770.00, 420.00, '逾期1天，已加收罚金');

-- Order pending pickup (customer01 reserved京G-55555)
INSERT INTO rental_orders (order_no, user_id, vehicle_id, start_date, end_date, actual_return_date, status, deposit, daily_rate, total_days, total_cost, penalty, notes)
VALUES ('R20260625001', 3, 7, '2026-06-25 10:00:00', '2026-06-30 10:00:00', NULL, 'pending', 5000.00, 350.00, 5, 0.00, 0.00, '待取车，客户已预约');

-- Order pending pickup (customer01 reserved京H-66666)
INSERT INTO rental_orders (order_no, user_id, vehicle_id, start_date, end_date, actual_return_date, status, deposit, daily_rate, total_days, total_cost, penalty, notes)
VALUES ('R20260625002', 3, 8, '2026-06-25 14:00:00', '2026-06-28 14:00:00', NULL, 'pending', 4000.00, 300.00, 3, 0.00, 0.00, '新能源体验，待取车');

-- Additional maintenance records
INSERT INTO maintenance_records (vehicle_id, type, description, cost, start_date, end_date, technician, status) VALUES
(1, 'maintenance', '常规保养：更换机油机滤，检查刹车系统', 500.00, '2026-06-15', '2026-06-15', '王师傅', 'completed'),
(4, 'repair',      '更换前后刹车片，检修制动管路',       800.00, '2026-06-20', '2026-06-21', '李师傅', 'completed'),
(9, 'maintenance', '空调系统检修，更换冷媒',             300.00, '2026-06-24', NULL,         '张师傅', 'in_progress'),
(10,'repair',      '发动机故障排查，更换火花塞',         2000.00,'2026-06-25', NULL,         '王师傅', 'pending');

-- Additional payments
INSERT INTO payments (order_id, order_no, user_id, amount, type, method, status, remark, paid_at) VALUES
(1, 'R20260620001', 3, 5000.00, 'deposit', 'wechat',  'success', '租车押金', '2026-06-20 10:05:00'),
(1, 'R20260620001', 3, 1400.00, 'rental',  'wechat',  'success', '租车费用（5天）', '2026-06-25 09:35:00'),
(2, 'R20260622001', 3, 4000.00, 'deposit', 'alipay',  'success', '租车押金', '2026-06-22 09:10:00'),
(3, 'R20260623001', 3, 8000.00, 'deposit', 'wechat',  'success', '租车押金', '2026-06-23 11:05:00'),
(3, 'R20260623001', 3, 1770.00, 'rental',  'wechat',  'success', '租车费用+逾期罚款', '2026-06-27 14:10:00'),
(4, 'R20260625001', 3, 5000.00, 'deposit', 'card',    'pending', '待支付押金', NULL);

-- Additional invoices
INSERT INTO invoices (invoice_no, order_id, order_no, user_id, username, total_amount, items, status, issued_at, generated_at) VALUES
('INV20260625001', 1, 'R20260620001', 3, 'customer01', 1400.00,
 '[{"type":"rental","amount":1400,"description":"租车费用5天（京A-12345）"}]',
 'issued', '2026-06-25 15:00:00', '2026-06-25 15:00:00'),
('INV20260627001', 3, 'R20260623001', 3, 'customer01', 1770.00,
 '[{"type":"rental","amount":1350,"description":"租车费用3天（京C-11111）"},{"type":"penalty","amount":420,"description":"逾期1天罚款"}]',
 'issued', '2026-06-27 17:00:00', '2026-06-27 17:00:00');

-- Balance records for customer01
INSERT INTO balance_records (user_id, amount, type, operator_id, remark) VALUES
(3, 5000.00, 'topup', 1, '初始充值');
