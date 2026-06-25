#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "Global.h"
#include "LogicSystem.h"
#include <unordered_set>

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
public:
    friend class LogicSystem;
    HttpConnection(boost::asio::io_context& ioc);
    void start();
    tcp::socket& socket() { return socket_; }
private:
    void checkDeadline_();
    void handleRequest_();
    void makeResponse_();
    void PreParseGetParam();
    bool authenticateRequest_();
    tcp::socket socket_;
    beast::flat_buffer buffer_ {8192};
    http::request<http::dynamic_body> req_;
    http::response<http::dynamic_body> resp_;
    net::steady_timer deadline_ {
        socket_.get_executor(),
        std::chrono::seconds(60)
    };
    std::string get_url_;
    std::unordered_map<std::string, std::string> get_params_;
    inline static const std::unordered_set<std::string> PUBLIC_PATHS = {
        "/user/register",
        "/user/login"
    };
};

#endif /* HTTPCONNECTION_H */
