#include "UMSGrpcServiceImpl.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "RedisManager.h"
#include <grpcpp/grpcpp.h>
#include <thread>
#include <chrono>
#include <atomic>

int main() {
    ConfigManager& config = ConfigManager::getInstance();
    std::string host = config["UMSServer"]["host"];
    std::string port = config["UMSServer"]["port"];
    std::string addr = host + ":" + port;

    Logger::getInstance();
    RedisManager::getInstance();
    LOG_INFO("UMSServer starting on {}", addr);

    UMSGrpcServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LOG_INFO("UMSServer gRPC listening on {}", addr);

    server->Wait();

    return 0;
}
