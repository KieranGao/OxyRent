#ifndef MAINSERVER_H
#define MAINSERVER_H

#include "Global.h"
#include "HttpConnection.h"

class MainServer : public std::enable_shared_from_this<MainServer>
{
public:
    MainServer(boost::asio::io_context& ioc, unsigned short port);
    void start();

private:
    tcp::acceptor acceptor_;
    boost::asio::io_context& ioc_;
};

#endif /* MAINSERVER_H */
