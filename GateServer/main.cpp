#include "Global.h"
#include "MainServer.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "AsyncTaskPool.h"
#include "RedisManager.h"
#include <thread>
#include <chrono>

int main() {
    ConfigManager& configManager = ConfigManager::getInstance();
    std::string gate_port_str = configManager["GateServer"]["port"];
    unsigned short gate_port = std::atoi(gate_port_str.c_str());

    Logger::getInstance();
    AsyncTaskPool::getInstance();
    RedisManager::getInstance();
    LOG_INFO("GateServer starting on port {}", gate_port);

    try
    {
        unsigned short port = static_cast<unsigned short>(gate_port);
        net::io_context ioc{ 1 };
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (error) return;
            LOG_INFO("GateServer shutting down");
            AsyncTaskPool::getInstance().stop();
            Logger::getInstance().stop();
            ioc.stop();
        });
        std::make_shared<MainServer>(ioc, port)->start();
        ioc.run();
    }
    catch (std::exception const& e)
    {
        LOG_ERROR("Fatal error: {}", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}
