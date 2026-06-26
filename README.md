# OxyRent - Vehicle Rental Management System

A microservices-based vehicle rental management system built with C++ backend services and a Vue 3 frontend.

## System Architecture

```
+---------------------------------------------------------------+
|                 Frontend (Vue3 + Element Plus)                 |
+---------------------------------------------------------------+
                              | HTTP POST
                              v
+---------------------------------------------------------------+
|               Gateway (GateServer - C++)                       |
+---------------------------------------------------------------+
                              | gRPC
          +-------------------+-------------------+
          v                   v                   v
+-----------------+ +-----------------+ +-----------------+
|   UMSServer     | |  VehicleServer  | |  FinanceServer  |
|  User Mgmt      | |  Vehicle+Rental | |  Billing+Stats  |
+-----------------+ +-----------------+ +-----------------+
          |                   |                   |
          v                   v                   v
+---------------------------------------------------------------+
|                        Data Layer                             |
|  +-----------------+              +-----------------+        |
|  |    MySQL 8.0    |              |    Redis 6.0+   |        |
|  +-----------------+              +-----------------+        |
+---------------------------------------------------------------+
```

## Features

1. **User Management** - Registration, login, profile management, role-based access control (admin/staff/customer)
2. **Vehicle Management** - Vehicle CRUD, status tracking, document management
3. **Rental Business** - Reservation, pickup, return, renewal, order management
4. **Maintenance Management** - Repair and maintenance records, work orders
5. **Billing & Settlement** - Fee calculation, deposit management, invoice generation
6. **Statistics & Reports** - Revenue analytics, vehicle utilization, visual charts

## Quick Start

### Docker Deployment (Recommended)

```bash
# Build and start all services
docker-compose up -d --build

# Check service status
docker-compose ps

# View logs
docker-compose logs -f

# Access the system
# Frontend: http://localhost:3000
# Gateway:  http://localhost:8080
```

### Local Development

```bash
# 1. Start MySQL and Redis
docker-compose up -d mysql redis

# 2. Initialize the database
mysql -u root -proot123 < sql/vehicle_rental.sql

# 3. Build C++ services
bash script/build_all.sh

# 4. Start all backend services
bash script/start_all.sh

# 5. Start the frontend dev server
cd Client && npm install && npm run dev
```

## Default Accounts

| Username   | Password    | Role     |
|------------|-------------|----------|
| admin      | admin123    | Admin    |
| staff01    | staff123    | Staff    |
| customer01 | customer123 | Customer |

## Tech Stack

### Backend
- C++17
- Boost.Beast (HTTP)
- gRPC + Protobuf
- MySQL Connector/C++
- hiredis (Redis)

### Frontend
- Vue 3 (Composition API)
- Element Plus
- Pinia
- Vue Router 4
- ECharts
- Axios

### Infrastructure
- MySQL 8.0
- Redis 6.0+
- Docker + Docker Compose
- Nginx

## Project Structure

```
OxyPark/
+-- Client/              # Vue3 frontend
+-- GateServer/          # HTTP gateway service
+-- UMSServer/           # User management service
+-- VehicleServer/       # Vehicle + rental service
+-- FinanceServer/       # Billing + statistics service
+-- MailerServer/        # Email notification service
+-- sql/                 # Database schema and seed data
+-- script/              # Build and运维 scripts
+-- docker-compose.yml   # Docker orchestration
+-- README.md            # This file
```

## API Reference

All API endpoints are proxied through the Gateway (port 8080). Most endpoints use POST method; query parameters are used for GET requests.

### Authentication

| Method | Endpoint           | Description         | Auth Required |
|--------|--------------------|---------------------|---------------|
| POST   | /user/register     | Register new user   | No            |
| POST   | /user/login        | User login          | No            |

### User Management

| Method | Endpoint           | Description              | Auth Required |
|--------|--------------------|--------------------------|---------------|
| GET    | /user/profile      | Get current user profile | Yes           |
| PUT    | /user/profile      | Update profile           | Yes           |
| GET    | /user/list         | List users (admin)       | Admin         |
| PUT    | /user/status       | Update user status       | Admin         |
| PUT    | /user/role         | Update user role         | Admin         |

### Vehicle Management

| Method | Endpoint           | Description              | Auth Required |
|--------|--------------------|--------------------------|---------------|
| GET    | /vehicle/list      | List vehicles            | Yes           |
| GET    | /vehicle/detail    | Get vehicle details      | Yes           |
| POST   | /vehicle           | Add vehicle (admin)      | Admin         |
| PUT    | /vehicle           | Update vehicle (admin)   | Admin         |
| POST   | /vehicle/delete    | Delete vehicle (admin)   | Admin         |

### Rental Business

| Method | Endpoint           | Description              | Auth Required |
|--------|--------------------|--------------------------|---------------|
| GET    | /rental/list       | List rental orders       | Yes           |
| GET    | /rental/detail     | Get order details        | Yes           |
| POST   | /rental/create     | Create rental order      | Yes           |
| POST   | /rental/pickup     | Pickup vehicle           | Staff/Admin   |
| POST   | /rental/return     | Return vehicle           | Staff/Admin   |
| POST   | /rental/renew      | Renew rental             | Yes           |

### Maintenance

| Method | Endpoint               | Description              | Auth Required |
|--------|------------------------|--------------------------|---------------|
| GET    | /maintenance/list      | List maintenance records | Admin         |
| POST   | /maintenance/create    | Create record            | Admin         |
| PUT    | /maintenance           | Update record            | Admin         |

### Finance

| Method | Endpoint               | Description              | Auth Required |
|--------|------------------------|--------------------------|---------------|
| GET    | /payment/list          | List payments            | Admin         |
| GET    | /payment/detail        | Get payment details      | Admin         |
| POST   | /payment/create        | Create payment           | Yes           |
| GET    | /invoice/detail        | Get invoice details      | Admin         |
| POST   | /invoice/generate      | Generate invoice         | Admin         |

### Statistics

| Method | Endpoint               | Description              | Auth Required |
|--------|------------------------|--------------------------|---------------|
| GET    | /stats/overview        | Dashboard overview       | Admin         |
| GET    | /stats/revenue         | Revenue analytics        | Admin         |
| GET    | /stats/vehicles        | Vehicle utilization      | Admin         |

## Database Schema

The system uses MySQL 8.0 with the following tables:

- **user** - User accounts with role-based access (admin/staff/customer)
- **vehicles** - Vehicle inventory with status tracking
- **vehicle_documents** - Vehicle documents (insurance, registration, inspection)
- **rental_orders** - Rental transactions with lifecycle tracking
- **maintenance_records** - Repair and maintenance history
- **payments** - Payment transactions
- **invoices** - Generated invoices for completed rentals

## License

Internal project - not for distribution.
