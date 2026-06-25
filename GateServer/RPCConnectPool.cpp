#include "RPCConnectPool.h"
#include "Logger.h"

UserConnectPool::UserConnectPool(size_t pool_size, std::string host, std::string port)
    : is_running_(true), host_(std::move(host)), port_(std::move(port)), pool_size_(pool_size)
{
    for(size_t i=0;i<pool_size;i++) {
        std::shared_ptr<Channel> channel = grpc::CreateChannel(host_ + ":" + port_, grpc::InsecureChannelCredentials());
        stubs_.emplace(UserService::NewStub(channel));
    }
}

UserConnectPool::~UserConnectPool() {
    stop();
    LOG_DEBUG("UserConnectPool destroyed");
}

std::unique_ptr<UserService::Stub> UserConnectPool::getStub() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cond_.wait_for(lock, std::chrono::seconds(5), [this](){ return !stubs_.empty() or !is_running_; })) {
        return nullptr;
    }
    if(!is_running_) return nullptr;
    auto stub = std::move(stubs_.front());
    stubs_.pop();
    return stub;
}

void UserConnectPool::returnStub(std::unique_ptr<UserService::Stub> stub) {
    std::lock_guard<std::mutex> lock(mutex_);
    if(is_running_) {
        stubs_.push(std::move(stub));
        cond_.notify_one();
    }
}

void UserConnectPool::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    is_running_ = false;
    cond_.notify_all();
    while(!stubs_.empty()) stubs_.pop();
}

// ==================== VehicleConnectPool ====================

VehicleConnectPool::VehicleConnectPool(size_t pool_size, std::string host, std::string port)
    : is_running_(true), host_(std::move(host)), port_(std::move(port)), pool_size_(pool_size)
{
    for(size_t i=0;i<pool_size;i++) {
        std::shared_ptr<Channel> channel = grpc::CreateChannel(host_ + ":" + port_, grpc::InsecureChannelCredentials());
        stubs_.emplace(VehicleService::NewStub(channel));
    }
}

VehicleConnectPool::~VehicleConnectPool() {
    stop();
    LOG_DEBUG("VehicleConnectPool destroyed");
}

std::unique_ptr<VehicleService::Stub> VehicleConnectPool::getStub() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cond_.wait_for(lock, std::chrono::seconds(5), [this](){ return !stubs_.empty() or !is_running_; })) {
        return nullptr;
    }
    if(!is_running_) return nullptr;
    auto stub = std::move(stubs_.front());
    stubs_.pop();
    return stub;
}

void VehicleConnectPool::returnStub(std::unique_ptr<VehicleService::Stub> stub) {
    std::lock_guard<std::mutex> lock(mutex_);
    if(is_running_) {
        stubs_.push(std::move(stub));
        cond_.notify_one();
    }
}

void VehicleConnectPool::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    is_running_ = false;
    cond_.notify_all();
    while(!stubs_.empty()) stubs_.pop();
}

// ==================== FinanceConnectPool ====================

FinanceConnectPool::FinanceConnectPool(size_t pool_size, std::string host, std::string port)
    : is_running_(true), host_(std::move(host)), port_(std::move(port)), pool_size_(pool_size)
{
    for(size_t i=0;i<pool_size;i++) {
        std::shared_ptr<Channel> channel = grpc::CreateChannel(host_ + ":" + port_, grpc::InsecureChannelCredentials());
        stubs_.emplace(FinanceService::NewStub(channel));
    }
}

FinanceConnectPool::~FinanceConnectPool() {
    stop();
    LOG_DEBUG("FinanceConnectPool destroyed");
}

std::unique_ptr<FinanceService::Stub> FinanceConnectPool::getStub() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cond_.wait_for(lock, std::chrono::seconds(5), [this](){ return !stubs_.empty() or !is_running_; })) {
        return nullptr;
    }
    if(!is_running_) return nullptr;
    auto stub = std::move(stubs_.front());
    stubs_.pop();
    return stub;
}

void FinanceConnectPool::returnStub(std::unique_ptr<FinanceService::Stub> stub) {
    std::lock_guard<std::mutex> lock(mutex_);
    if(is_running_) {
        stubs_.push(std::move(stub));
        cond_.notify_one();
    }
}

void FinanceConnectPool::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    is_running_ = false;
    cond_.notify_all();
    while(!stubs_.empty()) stubs_.pop();
}
