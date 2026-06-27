#include "VehicleGrpcClient.h"
#include "Logger.h"

VehicleGrpcClient::VehicleGrpcClient() {
    auto& g_config = ConfigManager::getInstance();
    std::string host = g_config["VehicleServer"]["host"];
    std::string port = g_config["VehicleServer"]["port"];
    rpc_pool_ = std::make_unique<VehicleConnectPool>(8, host, port);
}

// ==================== 车辆增删改查 ====================

VehicleListResponse VehicleGrpcClient::getVehicleList(const VehicleListRequest& request) {
    VehicleListResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetVehicleList(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService GetVehicleList RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

VehicleInfo VehicleGrpcClient::getVehicleDetail(const VehicleDetailRequest& request) {
    VehicleInfo response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetVehicleDetail(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService GetVehicleDetail RPC failed: {}", status.error_message());
    }
    return response;
}

CommonResponse VehicleGrpcClient::addVehicle(const AddVehicleRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->AddVehicle(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService AddVehicle RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

CommonResponse VehicleGrpcClient::updateVehicle(const UpdateVehicleRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->UpdateVehicle(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService UpdateVehicle RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

CommonResponse VehicleGrpcClient::deleteVehicle(const VehicleDetailRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->DeleteVehicle(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService DeleteVehicle RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

// ==================== 租赁订单 ====================

OrderInfo VehicleGrpcClient::createOrder(const CreateOrderRequest& request) {
    OrderInfo response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->CreateOrder(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService CreateOrder RPC failed: {}", status.error_message());
    }
    return response;
}

OrderListResponse VehicleGrpcClient::getOrderList(const OrderListRequest& request) {
    OrderListResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetOrderList(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService GetOrderList RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

OrderInfo VehicleGrpcClient::getOrderDetail(const OrderDetailRequest& request) {
    OrderInfo response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetOrderDetail(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService GetOrderDetail RPC failed: {}", status.error_message());
    }
    return response;
}

CommonResponse VehicleGrpcClient::pickupVehicle(const PickupRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->PickupVehicle(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService PickupVehicle RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

OrderInfo VehicleGrpcClient::returnVehicle(const ReturnRequest& request) {
    OrderInfo response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->ReturnVehicle(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService ReturnVehicle RPC failed: {}", status.error_message());
    }
    return response;
}

OrderInfo VehicleGrpcClient::renewOrder(const RenewRequest& request) {
    OrderInfo response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->RenewOrder(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService RenewOrder RPC failed: {}", status.error_message());
    }
    return response;
}

CommonResponse VehicleGrpcClient::cancelOrder(const PickupRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->CancelOrder(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService CancelOrder RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

// ==================== 维保管理 ====================

CommonResponse VehicleGrpcClient::createMaintenance(const CreateMaintenanceRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->CreateMaintenance(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService CreateMaintenance RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

CommonResponse VehicleGrpcClient::updateMaintenance(const UpdateMaintenanceRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->UpdateMaintenance(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService UpdateMaintenance RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

MaintenanceListResponse VehicleGrpcClient::getMaintenanceList(const MaintenanceListRequest& request) {
    MaintenanceListResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetMaintenanceList(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService GetMaintenanceList RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

CommonResponse VehicleGrpcClient::deleteMaintenance(const VehicleDetailRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->DeleteMaintenance(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("VehicleService DeleteMaintenance RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}
