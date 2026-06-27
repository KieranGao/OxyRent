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
    // 设置键值对
    bool set(const std::string& key, const std::string& value);
    // 获取值到输出参数
    bool get(const std::string& key, std::string& value);
    // 删除键
    bool del(const std::string& key);
    // 认证
    bool auth(const std::string& password);
    // 设置带过期时间的键值对（秒）
    bool setex(const std::string& key, const std::string& value, int seconds);
    // 设置键的过期时间（秒）
    bool expire(const std::string& key, int seconds);
    // 原子递增
    long incr(const std::string& key);
    // 原子递减
    long decr(const std::string& key);

    // 分布式锁：SET NX EX 原子获取
    bool acquireLock(const std::string& lock_key, const std::string& owner_id, int ttl_seconds = 30);
    // 分布式锁：Lua脚本原子释放（仅释放自己的锁）
    bool releaseLock(const std::string& lock_key, const std::string& owner_id);
    // 分布式锁：指数退避重试获取
    bool acquireLockWithRetry(const std::string& lock_key, const std::string& owner_id,
                              int ttl_seconds = 30, int max_retries = 3, int base_delay_ms = 50);

    // 执行Lua脚本
    long long evalScript(const std::string& lua_script,
                         const std::vector<std::string>& keys,
                         const std::vector<std::string>& args);

    void close();
private:
    RedisManager();
    std::unique_ptr<RedisConnectPool> conn_pool_;
};

#endif /* REDISMANAGER_H */
