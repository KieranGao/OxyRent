<h1 align="center">OxyRent</h1>
<p align="center">
  <strong>Система управления прокатом автомобилей — управление автопарком, заказами, обслуживанием и_billingом</strong>
  <br />
  <em>Микросервисы · C++ gRPC · Vue 3 · MySQL · Redis · Docker</em>
</p>

<p align="center">
  <a href="#быстрый-старт"><img src="https://img.shields.io/badge/Быстрый_старт-4CAF50?style=for-the-badge" alt="Quick Start" /></a>
  <a href="../LICENSE"><img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" alt="License" /></a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C++17-00599C?style=flat&logo=cplusplus&logoColor=white" alt="C++17" />
  <img src="https://img.shields.io/badge/Vue_3-4FC08D?style=flat&logo=vuedotjs&logoColor=white" alt="Vue 3" />
  <img src="https://img.shields.io/badge/gRPC-244c5a?style=flat&logo=grpc&logoColor=white" alt="gRPC" />
  <img src="https://img.shields.io/badge/MySQL-4479A1?style=flat&logo=mysql&logoColor=white" alt="MySQL" />
  <img src="https://img.shields.io/badge/Redis-DC382D?style=flat&logo=redis&logoColor=white" alt="Redis" />
  <img src="https://img.shields.io/badge/Docker-2496ED?style=flat&logo=docker&logoColor=white" alt="Docker" />
  <img src="https://img.shields.io/badge/Ubuntu-E95420?style=flat&logo=ubuntu&logoColor=white" alt="Ubuntu" />
  <img src="https://img.shields.io/badge/Element_Plus-409EFF?style=flat&logo=element&logoColor=white" alt="Element Plus" />
  <img src="https://img.shields.io/badge/Vite-646CFF?style=flat&logo=vite&logoColor=white" alt="Vite" />
</p>

<p align="center">
  <a href="../README.md">中文</a> · <a href="README-en.md">English</a> · <a href="README-ja.md">日本語</a> · Русский
</p>

---

## Возможности

| Функция | Описание |
|---|---|
| Управление автомобилями | CRUD-операции, отслеживание статуса (доступен/арендован/на обслуживании), фильтрация по бренду |
| Заказы проката | Онлайн-бронирование, получение/возврат, продление, отмена, автоматический расчёт стоимости и штрафов |
| Обслуживание | Создание и отслеживание записей ТО, автоматическое восстановление статуса автомобиля |
| Финансы | Записи платежей, генерация счетов, статистика доходов, анализ_utilization |
| Управление пользователями | Три роли (администратор/сотрудник/клиент), пополнение баланса, управление профилем |
| Панель мониторинга | Статистика в реальном времени: количество пользователей, автопарк, статус заказов, тренды доходов |

## Быстрый старт

### Предварительные требования

- Docker 20.10+
- Docker Compose 2.0+
- **Локальное развёртывание требует Linux-среды** (рекомендуется Ubuntu 22.04+). Пользователи macOS / Windows должны запускать через Docker или WSL2.

### Запуск сервисов

```bash
git clone https://github.com/KieranGao/OxyRent.git
cd OxyRent
docker-compose up -d
```

### Доступ к системе

```bash
# Фронтенд
http://localhost:3000

# API-шлюз
http://localhost:8080
```

### Управление скриптами

```bash
# Сборка всех сервисов
./script/build_all.sh

# Запуск всех сервисов
./script/start_all.sh

# Остановка всех сервисов
./script/stop_all.sh
```

## Примеры использования

### Регистрация пользователя

```bash
curl -X POST http://localhost:8080/user/register \
  -H "Content-Type: application/json" \
  -d '{"username": "testuser", "password": "123456", "email": "test@example.com"}'
```

### Авторизация

```bash
curl -X POST http://localhost:8080/user/login \
  -H "Content-Type: application/json" \
  -d '{"username": "testuser", "password": "123456"}'
```

### Список автомобилей

```bash
curl -X GET http://localhost:8080/vehicle/list?page=1&page_size=10
```

### Создание заказа проката

```bash
curl -X POST http://localhost:8080/rental/create \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"user_id": 1, "vehicle_id": 1, "start_date": "2026-07-01", "end_date": "2026-07-07"}'
```

## Архитектура

```mermaid
graph TB
    subgraph Client["Фронтенд"]
        Vue["Vue 3 + Vite<br/>Element Plus + Pinia"]
    end

    subgraph Gateway["Шлюз"]
        Gate["GateServer<br/>C++ Boost.Beast<br/>HTTP :8080"]
    end

    subgraph Services["Микросервисы"]
        UMS["UMSServer<br/>Управление пользователями<br/>gRPC :50053"]
        Vehicle["VehicleServer<br/>Автомобили и прокат<br/>gRPC :50054"]
        Finance["FinanceServer<br/>Финансы и статистика<br/>gRPC :50055"]
        Mailer["MailerServer<br/>Почтовый сервис<br/>gRPC :50056"]
    end

    subgraph Storage["Уровень данных"]
        MySQL[("MySQL<br/>vehicle_rental")]
        Redis[("Redis<br/>Кэш")]
    end

    subgraph External["Внешние сервисы"]
        SMTP["SMTP<br/>Почтовый сервер"]
    end

    Vue -->|HTTP/REST| Gate
    Gate -->|gRPC| UMS
    Gate -->|gRPC| Vehicle
    Gate -->|gRPC| Finance
    Gate -->|gRPC| Mailer
    UMS --> MySQL
    UMS --> Redis
    Vehicle --> MySQL
    Vehicle --> Redis
    Finance --> MySQL
    Finance --> Redis
    Mailer -->|SendMail| SMTP
    Mailer --> Redis
```

### Поток запросов

```mermaid
sequenceDiagram
    participant C as Vue Фронтенд
    participant G as GateServer
    participant U as UMSServer
    participant V as VehicleServer
    participant F as FinanceServer
    participant M as MailerServer

    C->>G: POST /user/register
    G->>M: SendVerifyCode (email верификация)
    M-->>G: Код отправлен
    G->>U: UserRegister
    U-->>G: CommonResponse
    G-->>C: JSON Response

    C->>G: POST /rental/create
    G->>V: CreateOrder
    V-->>G: OrderInfo
    G->>F: CreatePayment
    F-->>G: PaymentInfo
    G-->>C: Информация о заказе и оплате
```

### Модель данных

```mermaid
erDiagram
    USER ||--o{ ORDER : creates
    USER ||--o{ BALANCE_RECORD : has
    VEHICLE ||--o{ ORDER : "rented in"
    VEHICLE ||--o{ MAINTENANCE : "undergoes"
    ORDER ||--o{ PAYMENT : "has"
    ORDER ||--o| INVOICE : "generates"

    USER {
        bigint id PK
        string username
        string password
        string phone
        string email
        string role
        string status
        double balance
    }

    VEHICLE {
        bigint id PK
        string plate_number
        string brand
        string model
        string status
        double daily_rate
        double deposit_amount
    }

    ORDER {
        bigint id PK
        string order_no
        bigint user_id FK
        bigint vehicle_id FK
        string start_date
        string end_date
        string status
        double total_cost
    }

    PAYMENT {
        bigint id PK
        bigint order_id FK
        double amount
        string type
        string status
    }

    MAINTENANCE {
        bigint id PK
        bigint vehicle_id FK
        string type
        double cost
        string status
    }

    INVOICE {
        bigint id PK
        string invoice_no
        bigint order_id FK
        double total_amount
    }
```

## Конфигурация

Каждый сервис использует INI-файлы конфигурации, монтируемые в `/etc/server/config.ini` внутри контейнера.

### Конфигурация шлюза (gate-config.ini)

| Ключ | Описание | Пример |
|---|---|---|
| `GateServer.host` | Адрес прослушивания | `0.0.0.0` |
| `GateServer.port` | Порт прослушивания | `8080` |
| `MySQL.host` | Адрес MySQL | `mysql` |
| `Redis.host` | Адрес Redis | `redis` |
| `UMSServer.host` | Адрес сервиса пользователей | `ums-server` |
| `UMSServer.port` | Порт сервиса пользователей | `50053` |
| `VehicleServer.host` | Адрес сервиса автомобилей | `vehicle-server` |
| `VehicleServer.port` | Порт сервиса автомобилей | `50054` |
| `FinanceServer.host` | Адрес финансового сервиса | `finance-server` |
| `FinanceServer.port` | Порт финансового сервиса | `50055` |
| `MailerServer.host` | Адрес почтового сервиса | `mailer-server` |
| `MailerServer.port` | Порт почтового сервиса | `50056` |

## API

### Публичные эндпоинты (без авторизации)

| Метод | Путь | Описание |
|---|---|---|
| POST | `/user/register` | Регистрация пользователя |
| POST | `/user/login` | Авторизация |

### Эндпоинты пользователей

| Метод | Путь | Описание | Роль |
|---|---|---|---|
| GET | `/user/profile` | Получить профиль | Все |
| PUT | `/user/profile` | Обновить профиль | Все |
| GET | `/user/list` | Список пользователей | Администратор |
| POST | `/balance/topup` | Пополнить баланс | Сотрудник/Администратор |

### Эндпоинты автомобилей

| Метод | Путь | Описание | Роль |
|---|---|---|---|
| GET | `/vehicle/list` | Список автомобилей | Все |
| GET | `/vehicle/detail` | Детали автомобиля | Все |
| POST | `/vehicle/add` | Добавить автомобиль | Администратор |
| PUT | `/vehicle/update` | Обновить автомобиль | Администратор |
| DELETE | `/vehicle/delete` | Удалить автомобиль | Администратор |

### Эндпоинты проката

| Метод | Путь | Описание | Роль |
|---|---|---|---|
| POST | `/rental/create` | Создать заказ | Все |
| GET | `/rental/list` | Список заказов | Все |
| POST | `/rental/pickup` | Получить автомобиль | Сотрудник/Администратор |
| POST | `/rental/return` | Вернуть автомобиль | Сотрудник/Администратор |
| POST | `/rental/renew` | Продлить аренду | Все |
| POST | `/rental/cancel` | Отменить заказ | Все |

## Структура проекта

```
OxyRent/
├── Client/                  # Vue 3 фронтенд
├── GateServer/              # HTTP-шлюз (C++ Boost.Beast)
├── UMSServer/               # Управление пользователями (C++ gRPC)
├── VehicleServer/           # Автомобили и прокат (C++ gRPC)
├── FinanceServer/           # Финансы и статистика (C++ gRPC)
├── MailerServer/            # Почтовый сервис (Node.js gRPC, Planned)
├── docker/                  # Файлы конфигурации сервисов
├── sql/                     # Скрипты инициализации БД
├── script/                  # Скрипты сборки/запуска/остановки
├── docker-compose.yml       # Оркестрация контейнеров
└── DESIGN.md                # Дизайн-система (Noir Elegance)
```

## Технологический стек

### Фронтенд

| Технология | Назначение |
|---|---|
| Vue 3 | UI-фреймворк |
| Vite | Инструмент сборки |
| Element Plus | Библиотека компонентов |
| Pinia | Управление состоянием |
| Vue Router | Маршрутизация |
| Axios | HTTP-клиент |
| ECharts | Визуализация данных |

### Бэкенд

| Технология | Назначение |
|---|---|
| C++17 | Серверный язык |
| Boost.Beast | HTTP-сервер (GateServer) |
| gRPC | Межсервисное взаимодействие |
| Protobuf | Протокол сериализации |
| Hiredis | Клиент Redis |
| MySQL Connector/C++ | Драйвер БД |
| JsonCpp | Парсинг JSON |

### Инфраструктура

| Технология | Назначение |
|---|---|
| MySQL | Реляционная БД |
| Redis | Кэширование и управление сессиями |
| Docker | Контейнеризация |
| Docker Compose | Оркестрация контейнеров |
| Ubuntu 22.04 | Базовый образ контейнера и рекомендуемая среда |
| CMake | Система сборки C++ |

## Развёртывание

### Docker Compose (рекомендуется)

```bash
docker-compose up -d
```

| Сервис | Порт | Описание |
|---|---|---|
| vue3-client | 3000 | Фронтенд |
| gate-server | 8080 | API-шлюз |
| ums-server | 50053 | Управление пользователями |
| vehicle-server | 50054 | Автомобили и прокат |
| finance-server | 50055 | Финансы и статистика |
| mysql | 3307 | База данных |
| redis | 6380 | Кэш |

## Участие в разработке

1. Fork репозитория
2. Создайте ветку функции (`git checkout -b feature/your-feature`)
3. Зафиксируйте изменения (`git commit -m 'feat: add your feature'`)
4. Отправьте ветку (`git push origin feature/your-feature`)
5. Откройте Pull Request

## Лицензия

[MIT](../LICENSE)
