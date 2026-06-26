#include "FinanceGrpcClient.h"
#include "Logger.h"

FinanceGrpcClient::FinanceGrpcClient() {
    auto& g_config = ConfigManager::getInstance();
    std::string host = g_config["FinanceServer"]["host"];
    std::string port = g_config["FinanceServer"]["port"];
    rpc_pool_ = std::make_unique<FinanceConnectPool>(8, host, port);
}

PaymentInfo FinanceGrpcClient::createPayment(const CreatePaymentRequest& request) {
    PaymentInfo response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->CreatePayment(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("FinanceService CreatePayment RPC failed: {}", status.error_message());
        response.set_id(0);
    }
    return response;
}

PaymentListResponse FinanceGrpcClient::getPaymentList(const PaymentListRequest& request) {
    PaymentListResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetPaymentList(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("FinanceService GetPaymentList RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

PaymentInfo FinanceGrpcClient::getPaymentDetail(const PaymentInfo& request) {
    PaymentInfo response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetPaymentDetail(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("FinanceService GetPaymentDetail RPC failed: {}", status.error_message());
    }
    return response;
}

InvoiceInfo FinanceGrpcClient::generateInvoice(const GenerateInvoiceRequest& request) {
    InvoiceInfo response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GenerateInvoice(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("FinanceService GenerateInvoice RPC failed: {}", status.error_message());
        response.set_id(0);
    }
    return response;
}

InvoiceInfo FinanceGrpcClient::getInvoiceDetail(const InvoiceInfo& request) {
    InvoiceInfo response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetInvoiceDetail(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("FinanceService GetInvoiceDetail RPC failed: {}", status.error_message());
    }
    return response;
}

InvoiceListResponse FinanceGrpcClient::getInvoiceList(const InvoiceListRequest& request) {
    InvoiceListResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetInvoiceList(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("FinanceService GetInvoiceList RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

StatsOverviewResponse FinanceGrpcClient::getStatsOverview(const CommonResponse& request) {
    StatsOverviewResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetStatsOverview(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("FinanceService GetStatsOverview RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

RevenueStatsResponse FinanceGrpcClient::getRevenueStats(const RevenueStatsRequest& request) {
    RevenueStatsResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetRevenueStats(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("FinanceService GetRevenueStats RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

VehicleStatsResponse FinanceGrpcClient::getVehicleStats(const CommonResponse& request) {
    VehicleStatsResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetVehicleStats(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("FinanceService GetVehicleStats RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}
