#include "FinanceGrpcServiceImpl.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "RedisManager.h"
#include <grpcpp/grpcpp.h>
#include <thread>
#include <chrono>
#include <atomic>

int main() {
    ConfigManager& config = ConfigManager::getInstance();
    std::string host = config["FinanceServer"]["host"];
    std::string port = config["FinanceServer"]["port"];
    std::string addr = host + ":" + port;

    Logger::getInstance();
    RedisManager::getInstance();
    LOG_INFO("FinanceServer starting on {}", addr);

    FinanceGrpcServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LOG_INFO("FinanceServer gRPC listening on {}", addr);

    server->Wait();

    return 0;
}
