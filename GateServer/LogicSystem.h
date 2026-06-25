#ifndef LOGICSYSTEM_H
#define LOGICSYSTEM_H

#include "Global.h"
#include "Singleton.h"
#include <unordered_map>
#include <functional>
#include <string>

class HttpConnection;

typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem>
{
public:
    friend class Singleton<LogicSystem>;
    ~LogicSystem() = default;
    bool handleGet(std::string url, std::shared_ptr<HttpConnection> connection);
    void registerGet(std::string url, HttpHandler handler);
    bool handlePost(std::string url, std::shared_ptr<HttpConnection> connection);
    void registerPost(std::string url, HttpHandler handler);
    bool handlePut(std::string url, std::shared_ptr<HttpConnection> connection);
    void registerPut(std::string url, HttpHandler handler);
private:
    LogicSystem();
    std::unordered_map<std::string, HttpHandler> getHandlers_;
    std::unordered_map<std::string, HttpHandler> postHandlers_;
    std::unordered_map<std::string, HttpHandler> putHandlers_;
};

#endif /* LOGICSYSTEM_H */
