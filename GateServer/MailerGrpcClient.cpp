#include "MailerGrpcClient.h"
#include "Logger.h"

MailerGrpcClient::MailerGrpcClient() {
    auto& g_config = ConfigManager::getInstance();
    std::string host = g_config["MailerServer"]["host"];
    std::string port = g_config["MailerServer"]["port"];
    rpc_pool_ = std::make_unique<MailerConnectPool>(8, host, port);
}

SendMailResponse MailerGrpcClient::sendMail(const SendMailRequest& request) {
    SendMailResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->SendMail(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("MailerService SendMail RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}

SendVerifyCodeResponse MailerGrpcClient::sendVerifyCode(const SendVerifyCodeRequest& request) {
    SendVerifyCodeResponse response;
    ClientContext context;
    auto stub = rpc_pool_->getStub();
    Defer defer([&stub, this](){ rpc_pool_->returnStub(std::move(stub)); });
    Status status = stub->SendVerifyCode(&context, request, &response);
    if(!status.ok()) {
        LOG_ERROR("MailerService SendVerifyCode RPC failed: {}", status.error_message());
        response.set_error(static_cast<int32_t>(ErrorCodes::RPC_ERROR));
    }
    return response;
}
