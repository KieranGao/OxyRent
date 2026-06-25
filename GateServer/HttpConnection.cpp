#include "HttpConnection.h"
#include "Logger.h"
#include "RedisManager.h"

HttpConnection::HttpConnection(boost::asio::io_context& ioc)
    : socket_(ioc) {}

void HttpConnection::start() {
    auto self = shared_from_this();
    http::async_read(socket_, buffer_, req_, [self](beast::error_code ec, std::size_t
    byte_transferred) {
        try {
            if(ec) {
                LOG_ERROR("read error: {}", ec.message());
                return;
            }
            boost::ignore_unused(byte_transferred);
            self->handleRequest_();
            self->checkDeadline_();
        }
        catch (std::exception& exp) {
            LOG_ERROR("exception is {}", exp.what());
        }
    });
}

unsigned char ToHex(unsigned char x) {
    return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x) {
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else return 0;
    return y;
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) or
            (str[i] == '-') or
            (str[i] == '_') or
            (str[i] == '.') or
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] & 0x0F);
        }
    }
    return strTemp;
}

std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

void HttpConnection::PreParseGetParam() {
    std::string uri = req_.target().to_string();
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos) {
        get_url_ = uri;
        return;
    }

    get_url_ = uri.substr(0, query_pos);
    std::string query_string = uri.substr(query_pos + 1);
    std::string key;
    std::string value;
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(pair.substr(0, eq_pos));
            value = UrlDecode(pair.substr(eq_pos + 1));
            get_params_[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(query_string.substr(0, eq_pos));
            value = UrlDecode(query_string.substr(eq_pos + 1));
            get_params_[key] = value;
        }
    }
}

bool HttpConnection::authenticateRequest_() {
    std::string target = req_.target().to_string();
    auto query_pos = target.find('?');
    std::string path = (query_pos != std::string::npos) ? target.substr(0, query_pos) : target;
    if(PUBLIC_PATHS.count(path)) return true;

    auto write401 = [this](int error_code) {
        resp_.result(http::status::unauthorized);
        resp_.set(http::field::content_type, "application/json");
        Json::Value err;
        err["error"] = error_code;
        beast::ostream(resp_.body()) << err.toStyledString();
    };

    auto auth_it = req_.find(http::field::authorization);
    if (auth_it == req_.end()) {
        write401(static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING));
        return false;
    }
    std::string auth_value(auth_it->value().data(), auth_it->value().size());
    std::string token;
    const std::string bearer_prefix = "Bearer ";
    if (auth_value.compare(0, bearer_prefix.size(), bearer_prefix) == 0)
        token = auth_value.substr(bearer_prefix.size());
    else
        token = auth_value;
    if (token.empty()) {
        write401(static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING));
        return false;
    }

    auto uid_it = req_.find("X-User-Id");
    if (uid_it == req_.end()) {
        write401(static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING));
        return false;
    }
    int uid = 0;
    try {
        uid = std::stoi(std::string(uid_it->value().data(), uid_it->value().size()));
    } catch (...) {
        write401(static_cast<int>(ErrorCodes::AUTH_TOKEN_MISSING));
        return false;
    }

    std::string token_key = USER_TOKEN_PREFIX + std::to_string(uid);
    std::string stored_token;
    if (!RedisManager::getInstance().get(token_key, stored_token) || stored_token != token) {
        write401(static_cast<int>(ErrorCodes::AUTH_TOKEN_INVALID));
        return false;
    }
    LOG_DEBUG("[GateServer] uid = {} passed token authorization for url {}",uid, path);
    return true;
}

void HttpConnection::handleRequest_() {
    resp_.version(req_.version());
    resp_.keep_alive(false);
    if (!authenticateRequest_()) {
        makeResponse_();
        return;
    }

    if(req_.method() == http::verb::get) {
        PreParseGetParam();
        bool ok = LogicSystem::getInstance().handleGet(get_url_, shared_from_this());
        if (!ok) {
            resp_.result(http::status::not_found);
            resp_.set(http::field::content_type, "text/plain");
            beast::ostream(resp_.body()) << "url not found\r\n";
            makeResponse_();
            return;
        }
        resp_.result(http::status::ok);
        resp_.set(http::field::server, "GateServer");
        makeResponse_();
        return;
    }
    if(req_.method() == http::verb::post) {
        std::string target = req_.target().to_string();
        auto qpos = target.find('?');
        std::string url = (qpos != std::string::npos) ? target.substr(0, qpos) : target;
        bool ok = LogicSystem::getInstance().handlePost(url, shared_from_this());
        if(!ok) {
            resp_.result(http::status::not_found);
            resp_.set(http::field::content_type, "text/plain");
            beast::ostream(resp_.body()) << "url not found\r\n";
            makeResponse_();
            return;
        }
        resp_.result(http::status::ok);
        resp_.set(http::field::server, "GateServer");
        makeResponse_();
        return;
    }
    if(req_.method() == http::verb::put) {
        std::string target = req_.target().to_string();
        auto qpos = target.find('?');
        std::string url = (qpos != std::string::npos) ? target.substr(0, qpos) : target;
        bool ok = LogicSystem::getInstance().handlePut(url, shared_from_this());
        if(!ok) {
            resp_.result(http::status::not_found);
            resp_.set(http::field::content_type, "text/plain");
            beast::ostream(resp_.body()) << "url not found\r\n";
            makeResponse_();
            return;
        }
        resp_.result(http::status::ok);
        resp_.set(http::field::server, "GateServer");
        makeResponse_();
        return;
    }

    // Unsupported HTTP method
    resp_.result(http::status::method_not_allowed);
    resp_.set(http::field::content_type, "text/plain");
    beast::ostream(resp_.body()) << "Method Not Allowed\r\n";
    makeResponse_();
}

void HttpConnection::makeResponse_() {
    auto self = shared_from_this();
    resp_.content_length(resp_.body().size());
    http::async_write(socket_, resp_, [self](beast::error_code ec, std::size_t byte_transferred) {
        self->socket_.shutdown(tcp::socket::shutdown_send, ec);
        self->deadline_.cancel();
    });
}

void HttpConnection::checkDeadline_() {
    auto self = shared_from_this();
    deadline_.async_wait([self](beast::error_code ec) {
        if (ec) {
            return;
        }
        self->socket_.close(ec);
    });
}
