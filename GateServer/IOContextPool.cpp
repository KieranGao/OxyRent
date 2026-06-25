#include "IOContextPool.h"
#include "Logger.h"

IOContextPool::IOContextPool(size_t pool_size) : io_contexts_(pool_size), works_(pool_size)
    , next_io_context_(0)
{
    for(size_t i=0;i<pool_size;i++) {
        io_contexts_[i] = std::make_shared<IOContext>();
    }
    for(size_t i=0;i<pool_size;i++) {
        works_[i] = std::make_unique<Work>(boost::asio::make_work_guard(*io_contexts_[i]));
    }
    for(size_t i=0;i<pool_size;i++) {
        threads_.emplace_back([this, i](){
            io_contexts_[i]->run();
        });
    }
}

IOContextPool::~IOContextPool() {
    stop();
    LOG_DEBUG("IOContextPool is destroyed");
}

boost::asio::io_context& IOContextPool::getIOContext() {
    auto& io_context = *io_contexts_[next_io_context_];
    next_io_context_ = (next_io_context_ + 1) % io_contexts_.size();
    return io_context;
}

void IOContextPool::stop() {
    for(auto& work : works_) {
        work->get_executor().context().stop();
        work.reset();
    }

    for(auto& t : threads_) {
        if(t.joinable()) {
            t.join();
        }
    }
}
