#include "VehicleGrpcServiceImpl.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "RedisManager.h"
#include <grpcpp/grpcpp.h>
#include <thread>
#include <chrono>
#include <atomic>

int main() {
    ConfigManager& config = ConfigManager::getInstance();
    std::string host = config["VehicleServer"]["host"];
    std::string port = config["VehicleServer"]["port"];
    std::string addr = host + ":" + port;

    Logger::getInstance();
    RedisManager::getInstance();
    LOG_INFO("VehicleServer starting on {}", addr);

    VehicleGrpcServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LOG_INFO("VehicleServer gRPC listening on {}", addr);

    server->Wait();

    return 0;
}
