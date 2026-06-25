#ifndef REDISMANAGER_H
#define REDISMANAGER_H

#include <hiredis/hiredis.h>
#include "Singleton.h"
#include "Global.h"
#include "RedisConnectPool.h"
class RedisManager : public Singleton<RedisManager> {

    friend class Singleton<RedisManager>;
public:
    ~RedisManager();
    // Set key-value
    bool set(const std::string& key, const std::string& value);
    // Get value into out parameter
    bool get(const std::string& key, std::string& value);
    // Delete key
    bool del(const std::string& key);
    // Authenticate
    bool auth(const std::string& password);
    // Set key-value with expiry (seconds)
    bool setex(const std::string& key, const std::string& value, int seconds);
    // Set expiry on key (seconds)
    bool expire(const std::string& key, int seconds);
    // Atomic increment
    long incr(const std::string& key);
    // Atomic decrement
    long decr(const std::string& key);

    // Distributed lock: SET NX EX atomic acquire
    bool acquireLock(const std::string& lock_key, const std::string& owner_id, int ttl_seconds = 30);
    // Distributed lock: Lua script atomic release (only releases own lock)
    bool releaseLock(const std::string& lock_key, const std::string& owner_id);
    // Distributed lock: acquire with exponential backoff retry
    bool acquireLockWithRetry(const std::string& lock_key, const std::string& owner_id,
                              int ttl_seconds = 30, int max_retries = 3, int base_delay_ms = 50);

    // Execute Lua script
    long long evalScript(const std::string& lua_script,
                         const std::vector<std::string>& keys,
                         const std::vector<std::string>& args);

    void close();
private:
    RedisManager();
    std::unique_ptr<RedisConnectPool> conn_pool_;
};

#endif /* REDISMANAGER_H */
