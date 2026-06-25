#include "Logger.h"
#include "ConfigManager.h"
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

Logger::Logger() : min_level_(LogLevel::DEBUG) {
    auto& cfg = ConfigManager::getInstance();
    std::string level_str = cfg["Log"]["level"];
    log_dir_  = cfg["Log"]["file_path"];
    service_name_ = cfg["Log"]["service_name"];

    if (log_dir_.empty()) log_dir_ = "logs/";
    if (service_name_.empty()) service_name_ = "Unknown";

    if (level_str == "ERROR")      min_level_ = LogLevel::ERROR;
    else if (level_str == "WARN")  min_level_ = LogLevel::WARN;
    else if (level_str == "INFO")  min_level_ = LogLevel::INFO;
    else                           min_level_ = LogLevel::DEBUG;

    mkdir(log_dir_.c_str(), 0755);

    rotateIfNeeded();
    if (!file_.is_open()) {
        file_.open(log_dir_ + service_name_ + ".log", std::ios::app);
    }
}

void Logger::stop() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (file_.is_open()) file_.close();
}

Logger::~Logger() {
    stop();
}

std::string Logger::timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm;
    localtime_r(&time_t, &tm);
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec, (int)ms.count());
    return buf;
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

void Logger::rotateIfNeeded() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&tt, &tm);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    std::string today(buf);
    if (today != current_date_) {
        if (file_.is_open()) file_.close();
        current_date_ = today;
        std::string path = log_dir_ + service_name_ + "_" + current_date_ + ".log";
        file_.open(path, std::ios::app);
    }
}

void Logger::writeToFile(const std::string& line) {
    rotateIfNeeded();
    if (file_.is_open()) {
        file_ << line << std::endl;
        file_.flush();
    }
}

void Logger::log(LogLevel level, const std::string& msg) {
    if (level < min_level_) return;

    std::string line = "[" + timestamp() + "] [" + levelToString(level) + "] " + msg;
    std::lock_guard<std::mutex> lock(mtx_);
    writeToFile(line);
}
