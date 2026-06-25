#include "MainServer.h"
#include "IOContextPool.h"
#include "Logger.h"

MainServer::MainServer(boost::asio::io_context& ioc, unsigned short port)
    : acceptor_(ioc, tcp::endpoint(tcp::v4(), port)),
      ioc_(ioc) {
        LOG_DEBUG("GateServer is listening on port: {}", acceptor_.local_endpoint().port());
      }

void MainServer::start()
{
    auto self = shared_from_this();
    auto& io_context_ = IOContextPool::getInstance().getIOContext();
    std::shared_ptr<HttpConnection> new_connection = std::make_shared<HttpConnection>(io_context_);
    acceptor_.async_accept(new_connection->socket(), [self, new_connection](beast::error_code ec) {
        try {
            if (ec) {
                self->start();
                return;
            }
            new_connection->start();
            self->start();
        }
        catch (std::exception& exp) {
            LOG_ERROR("exception is {}", exp.what());
            self->start();
        }
    });
}
