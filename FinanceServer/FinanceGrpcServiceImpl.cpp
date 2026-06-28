#include "FinanceGrpcServiceImpl.h"
#include "MySQLManager.h"
#include "RedisManager.h"
#include "Logger.h"
#include <sstream>
#include <iomanip>
#include <random>

FinanceGrpcServiceImpl::FinanceGrpcServiceImpl() {}

// 生成唯一的锁持有者ID（用于分布式锁）
static std::string generateLockOwnerId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);
    std::ostringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return ss.str();
}

// ==================== 支付操作 ====================

Status FinanceGrpcServiceImpl::CreatePayment(ServerContext* context, const CreatePaymentRequest* req, PaymentInfo* resp) {
    LOG_DEBUG("[Finance] CreatePayment order_id={} amount={}", req->order_id(), req->amount());

    // 分布式锁：锁定订单，防止重复支付
    std::string lock_key = "lock:payment:order:" + std::to_string(req->order_id());
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_id(0);
        LOG_WARN("[Finance] Failed to acquire lock for payment order: {}", req->order_id());
        return Status::OK;
    }

    int64_t id = MySQLManager::getInstance().createPayment(
        req->order_id(), req->amount(), req->type(), req->method(), req->remark());

    // 释放锁
    redis.releaseLock(lock_key, owner_id);

    if (id == -1) {
        resp->set_id(0);
        LOG_WARN("[Finance] CreatePayment failed: order not found or insert failed");
        return Status::OK;
    }

    // 返回创建的支付详情
    PaymentData payment;
    MySQLManager::getInstance().getPaymentDetail(id, payment);
    resp->set_id(payment.id);
    resp->set_order_id(payment.order_id);
    resp->set_order_no(payment.order_no);
    resp->set_amount(payment.amount);
    resp->set_type(payment.type);
    resp->set_method(payment.method);
    resp->set_status(payment.status);
    resp->set_remark(payment.remark);
    resp->set_paid_at(payment.paid_at);
    resp->set_created_at(payment.created_at);

    LOG_INFO("[Finance] CreatePayment success, id={}", id);
    return Status::OK;
}

Status FinanceGrpcServiceImpl::ConfirmPayment(ServerContext* context, const PaymentInfo* req, PaymentInfo* resp) {
    int64_t id = req->id();
    std::string paid_at = req->paid_at();
    LOG_DEBUG("[Finance] ConfirmPayment id={} paid_at={}", id, paid_at);

    // 分布式锁：锁定支付记录，防止重复确认
    std::string lock_key = "lock:payment:" + std::to_string(id);
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_id(0);
        LOG_WARN("[Finance] Failed to acquire lock for confirm payment: {}", id);
        return Status::OK;
    }

    bool ok = MySQLManager::getInstance().confirmPayment(id, paid_at);

    // 释放锁
    redis.releaseLock(lock_key, owner_id);

    if (!ok) {
        resp->set_id(0);
        LOG_WARN("[Finance] ConfirmPayment failed: payment {} not found or not pending", id);
        return Status::OK;
    }

    // 返回更新后的支付详情
    PaymentData payment;
    MySQLManager::getInstance().getPaymentDetail(id, payment);
    resp->set_id(payment.id);
    resp->set_order_id(payment.order_id);
    resp->set_order_no(payment.order_no);
    resp->set_amount(payment.amount);
    resp->set_type(payment.type);
    resp->set_method(payment.method);
    resp->set_status(payment.status);
    resp->set_remark(payment.remark);
    resp->set_paid_at(payment.paid_at);
    resp->set_created_at(payment.created_at);

    LOG_INFO("[Finance] ConfirmPayment success, id={}", id);
    return Status::OK;
}

Status FinanceGrpcServiceImpl::GetPaymentList(ServerContext* context, const PaymentListRequest* req, PaymentListResponse* resp) {
    LOG_DEBUG("[Finance] GetPaymentList page={} page_size={}", req->page(), req->page_size());

    std::vector<PaymentData> payments;
    int total = 0;
    bool ok = MySQLManager::getInstance().getPaymentList(
        req->page(), req->page_size(), req->order_id(),
        req->status(), req->type(), payments, total);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to fetch payment list");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_total(total);
    for (auto& p : payments) {
        auto* item = resp->add_payments();
        item->set_id(p.id);
        item->set_order_id(p.order_id);
        item->set_order_no(p.order_no);
        item->set_amount(p.amount);
        item->set_type(p.type);
        item->set_method(p.method);
        item->set_status(p.status);
        item->set_remark(p.remark);
        item->set_paid_at(p.paid_at);
        item->set_created_at(p.created_at);
    }
    return Status::OK;
}

Status FinanceGrpcServiceImpl::GetPaymentDetail(ServerContext* context, const PaymentInfo* req, PaymentInfo* resp) {
    int64_t id = req->id();
    LOG_DEBUG("[Finance] GetPaymentDetail id={}", id);

    PaymentData payment;
    bool ok = MySQLManager::getInstance().getPaymentDetail(id, payment);
    if (!ok) {
        resp->set_id(0);
        LOG_WARN("[Finance] Payment not found: {}", id);
        return Status::OK;
    }

    resp->set_id(payment.id);
    resp->set_order_id(payment.order_id);
    resp->set_order_no(payment.order_no);
    resp->set_amount(payment.amount);
    resp->set_type(payment.type);
    resp->set_method(payment.method);
    resp->set_status(payment.status);
    resp->set_remark(payment.remark);
    resp->set_paid_at(payment.paid_at);
    resp->set_created_at(payment.created_at);
    return Status::OK;
}

// ==================== 账单操作 ====================

Status FinanceGrpcServiceImpl::GenerateInvoice(ServerContext* context, const GenerateInvoiceRequest* req, InvoiceInfo* resp) {
    LOG_DEBUG("[Finance] GenerateInvoice order_id={}", req->order_id());

    // 分布式锁：锁定订单，防止重复生成发票
    std::string lock_key = "lock:invoice:order:" + std::to_string(req->order_id());
    std::string owner_id = generateLockOwnerId();
    auto& redis = RedisManager::getInstance();

    if (!redis.acquireLockWithRetry(lock_key, owner_id, 10, 3, 50)) {
        resp->set_id(0);
        LOG_WARN("[Finance] Failed to acquire lock for invoice order: {}", req->order_id());
        return Status::OK;
    }

    int64_t id = MySQLManager::getInstance().generateInvoice(req->order_id());

    // 释放锁
    redis.releaseLock(lock_key, owner_id);

    if (id <= 0) {
        resp->set_id(0);
        LOG_WARN("[Finance] GenerateInvoice failed for order: {}", req->order_id());
        return Status::OK;
    }

    InvoiceData invoice;
    MySQLManager::getInstance().getInvoiceDetail(id, invoice);
    resp->set_id(invoice.id);
    resp->set_invoice_no(invoice.invoice_no);
    resp->set_order_id(invoice.order_id);
    resp->set_order_no(invoice.order_no);
    resp->set_user_id(invoice.user_id);
    resp->set_username(invoice.username);
    resp->set_total_amount(invoice.total_amount);
    resp->set_items(invoice.items);
    resp->set_generated_at(invoice.generated_at);

    LOG_INFO("[Finance] GenerateInvoice success, id={} invoice_no={}", id, invoice.invoice_no);
    return Status::OK;
}

Status FinanceGrpcServiceImpl::GetInvoiceDetail(ServerContext* context, const InvoiceInfo* req, InvoiceInfo* resp) {
    int64_t id = req->id();
    LOG_DEBUG("[Finance] GetInvoiceDetail id={}", id);

    InvoiceData invoice;
    bool ok = MySQLManager::getInstance().getInvoiceDetail(id, invoice);
    if (!ok) {
        resp->set_id(0);
        LOG_WARN("[Finance] Invoice not found: {}", id);
        return Status::OK;
    }

    resp->set_id(invoice.id);
    resp->set_invoice_no(invoice.invoice_no);
    resp->set_order_id(invoice.order_id);
    resp->set_order_no(invoice.order_no);
    resp->set_user_id(invoice.user_id);
    resp->set_username(invoice.username);
    resp->set_total_amount(invoice.total_amount);
    resp->set_items(invoice.items);
    resp->set_generated_at(invoice.generated_at);
    return Status::OK;
}

Status FinanceGrpcServiceImpl::GetInvoiceList(ServerContext* context, const InvoiceListRequest* req, InvoiceListResponse* resp) {
    LOG_DEBUG("[Finance] GetInvoiceList page={} size={}", req->page(), req->page_size());

    std::vector<InvoiceData> invoices;
    int total = 0;

    bool ok = MySQLManager::getInstance().getInvoiceList(req->page(), req->page_size(), invoices, total);
    if (!ok) {
        resp->set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to get invoice list");
        return Status::OK;
    }

    resp->set_error(0);
    resp->set_total(total);
    for (const auto& inv : invoices) {
        auto* item = resp->add_invoices();
        item->set_id(inv.id);
        item->set_invoice_no(inv.invoice_no);
        item->set_order_id(inv.order_id);
        item->set_order_no(inv.order_no);
        item->set_user_id(inv.user_id);
        item->set_username(inv.username);
        item->set_total_amount(inv.total_amount);
        item->set_items(inv.items);
        item->set_generated_at(inv.generated_at);
    }
    return Status::OK;
}

// ==================== 统计操作 ====================

Status FinanceGrpcServiceImpl::GetStatsOverview(ServerContext* context, const CommonResponse* req, StatsOverviewResponse* resp) {
    LOG_DEBUG("[Finance] GetStatsOverview");

    int total_users = 0, total_vehicles = 0, available_vehicles = 0;
    int active_orders = 0, completed_orders = 0;
    double total_revenue = 0.0, month_revenue = 0.0;

    bool ok = MySQLManager::getInstance().getStatsOverview(
        total_users, total_vehicles, available_vehicles,
        active_orders, completed_orders, total_revenue, month_revenue);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to fetch stats overview");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_total_users(total_users);
    resp->set_total_vehicles(total_vehicles);
    resp->set_available_vehicles(available_vehicles);
    resp->set_active_orders(active_orders);
    resp->set_completed_orders(completed_orders);
    resp->set_total_revenue(total_revenue);
    resp->set_month_revenue(month_revenue);
    return Status::OK;
}

Status FinanceGrpcServiceImpl::GetRevenueStats(ServerContext* context, const RevenueStatsRequest* req, RevenueStatsResponse* resp) {
    LOG_DEBUG("[Finance] GetRevenueStats start={} end={} granularity={}",
              req->start_date(), req->end_date(), req->granularity());

    std::vector<RevenueStatsItemData> items;
    double total = 0.0;
    bool ok = MySQLManager::getInstance().getRevenueStats(
        req->start_date(), req->end_date(), req->granularity(), items, total);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to fetch revenue stats");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_total(total);
    for (auto& item : items) {
        auto* rsi = resp->add_items();
        rsi->set_date(item.date);
        rsi->set_amount(item.amount);
        rsi->set_rental(item.rental);
        rsi->set_maintenance(item.maintenance);
        rsi->set_count(item.count);
    }
    return Status::OK;
}

Status FinanceGrpcServiceImpl::GetVehicleStats(ServerContext* context, const CommonResponse* req, VehicleStatsResponse* resp) {
    LOG_DEBUG("[Finance] GetVehicleStats");

    int total = 0, available = 0, rented = 0, maintenance = 0;
    double utilization_rate = 0.0;
    std::vector<VehicleStatsBrandData> by_brand;

    bool ok = MySQLManager::getInstance().getVehicleStats(
        total, available, rented, maintenance, utilization_rate, by_brand);

    if (!ok) {
        resp->set_error(static_cast<int>(ErrorCodes::RPC_ERROR));
        resp->set_msg("Failed to fetch vehicle stats");
        return Status::OK;
    }

    resp->set_error(static_cast<int>(ErrorCodes::SUCCESS));
    resp->set_total(total);
    resp->set_available(available);
    resp->set_rented(rented);
    resp->set_maintenance(maintenance);
    resp->set_utilization_rate(utilization_rate);
    for (auto& brand : by_brand) {
        auto* item = resp->add_by_brand();
        item->set_brand(brand.brand);
        item->set_count(brand.count);
    }
    return Status::OK;
}
