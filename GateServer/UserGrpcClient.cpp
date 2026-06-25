#include "UserGrpcClient.h"
#include "Logger.h"

UserGrpcClient::UserGrpcClient() {
    auto& g_config = ConfigManager::getInstance();
    std::string host = g_config["UMSServer"]["host"];
    std::string port = g_config["UMSServer"]["port"];
    rpc_pool_ = std::make_unique<UserConnectPool>(8, host, port);
}

CommonResponse UserGrpcClient::userRegister(const UserRegisterRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->UserRegister(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("UserService UserRegister RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

UserLoginResponse UserGrpcClient::userLogin(const UserLoginRequest& request) {
    UserLoginResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->UserLogin(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("UserService UserLogin RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

UserProfileResponse UserGrpcClient::getUserProfile(const UserProfileRequest& request) {
    UserProfileResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetUserProfile(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("UserService GetUserProfile RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

CommonResponse UserGrpcClient::updateProfile(const UpdateProfileRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->UpdateProfile(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("UserService UpdateProfile RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

UserListResponse UserGrpcClient::getUserList(const UserListRequest& request) {
    UserListResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->GetUserList(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("UserService GetUserList RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

CommonResponse UserGrpcClient::updateUserStatus(const UpdateUserStatusRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->UpdateUserStatus(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("UserService UpdateUserStatus RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

CommonResponse UserGrpcClient::updateUserRole(const UpdateUserRoleRequest& request) {
    CommonResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->UpdateUserRole(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("UserService UpdateUserRole RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}
