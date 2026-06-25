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
CREATE TABLE IF NOT EXISTS users (
    id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    username    VARCHAR(64)     NOT NULL,
    password    VARCHAR(128)    NOT NULL,
    email       VARCHAR(128)    DEFAULT NULL,
    phone       VARCHAR(20)     DEFAULT NULL,
    real_name   VARCHAR(64)     DEFAULT NULL,
    id_card     VARCHAR(32)     DEFAULT NULL,
    role        ENUM('customer','staff','admin') NOT NULL DEFAULT 'customer',
    status      ENUM('pending','active','banned') NOT NULL DEFAULT 'pending',
    created_at  DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at  DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
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
    deposit         DECIMAL(10,2)   NOT NULL DEFAULT 0.00,
    status          ENUM('available','rented','maintenance') NOT NULL DEFAULT 'available',
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
    id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    order_no        VARCHAR(32)     NOT NULL,
    user_id         BIGINT UNSIGNED NOT NULL,
    vehicle_id      BIGINT UNSIGNED NOT NULL,
    start_date      DATETIME        NOT NULL,
    end_date        DATETIME        NOT NULL,
    actual_return   DATETIME        DEFAULT NULL,
    total_amount    DECIMAL(10,2)   NOT NULL DEFAULT 0.00,
    status          ENUM('pending','active','completed','cancelled') NOT NULL DEFAULT 'pending',
    remark          TEXT            DEFAULT NULL,
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_order_no (order_no),
    KEY idx_user_id (user_id),
    KEY idx_vehicle_id (vehicle_id),
    KEY idx_status (status),
    KEY idx_start_date (start_date),
    CONSTRAINT fk_order_user FOREIGN KEY (user_id) REFERENCES users(id),
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
    operator        VARCHAR(64)     DEFAULT NULL,
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
    CONSTRAINT fk_payment_user FOREIGN KEY (user_id) REFERENCES users(id),
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
    CONSTRAINT fk_invoice_user FOREIGN KEY (user_id) REFERENCES users(id),
    CONSTRAINT fk_invoice_order FOREIGN KEY (order_id) REFERENCES rental_orders(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Seed Data
-- ============================================================
-- NOTE: Seed passwords below are stored as plaintext for demo convenience.
-- In production, all passwords MUST be stored as SHA-256 hashes.

-- Admin user (admin / admin123)
INSERT INTO users (username, password, email, real_name, role, status)
VALUES ('admin', 'admin123', 'admin@oxypark.com', 'System Admin', 'admin', 'active');

-- Staff user (staff01 / staff123)
INSERT INTO users (username, password, email, real_name, role, status)
VALUES ('staff01', 'staff123', 'staff01@oxypark.com', 'Staff One', 'staff', 'active');

-- Customer user (customer01 / customer123)
INSERT INTO users (username, password, email, real_name, role, status)
VALUES ('customer01', 'customer123', 'customer01@example.com', 'Customer One', 'customer', 'active');

-- 5 Test Vehicles
INSERT INTO vehicles (plate_number, brand, model, color, year, mileage, daily_rate, deposit, status, location) VALUES
('京A-12345', 'Toyota',    'Camry',     'White',  2023, 15000, 280.00, 5000.00, 'available', 'Slot-A01'),
('京B-67890', 'Honda',     'Civic',     'Black',  2022, 32000, 220.00, 4000.00, 'available', 'Slot-A02'),
('京C-11111', 'Tesla',     'Model 3',   'Red',    2024,  5000, 450.00, 8000.00, 'available', 'Slot-B01'),
('京D-22222', 'BYD',       'Han EV',    'Blue',   2023, 20000, 350.00, 6000.00, 'maintenance', 'Workshop'),
('京E-33333', 'Volkswagen','Lavida',     'Silver', 2021, 48000, 180.00, 3000.00, 'available', 'Slot-C01');
