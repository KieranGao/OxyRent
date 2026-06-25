#ifndef IOCONTEXTPOOL_H
#define IOCONTEXTPOOL_H

#include <boost/asio.hpp>
#include "Singleton.h"
#include <thread>

class IOContextPool : public Singleton<IOContextPool> {
    friend class Singleton<IOContextPool>;
public:
    using IOContext = boost::asio::io_context;
    using Work = boost::asio::executor_work_guard<IOContext::executor_type>;
    using WorkPtr = std::unique_ptr<Work>;
    ~IOContextPool();
    boost::asio::io_context& getIOContext();
    void stop();
private:
    IOContextPool(size_t pool_size = 4);
    std::vector<std::shared_ptr<IOContext>> io_contexts_;
    std::vector<WorkPtr> works_;
    std::vector<std::thread> threads_;
    std::atomic<size_t> next_io_context_;
};

#endif /* IOCONTEXTPOOL_H */
