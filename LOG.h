#pragma once
#include "common.h"
#include <atomic>
#include <thread>
using namespace std::chrono_literals;

//日志系统
inline std::string time_buf[2];
inline std::atomic<int> time_idx{ 0 };

inline void cctime() {
    while (true) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &t);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

        int write = 1 - time_idx.load(std::memory_order_acquire);
        time_buf[write] = buf;
        time_idx.store(write, std::memory_order_release);

        std::this_thread::sleep_for(1s);
    }
}

inline std::string log_time() {
    return time_buf[time_idx.load(std::memory_order_acquire)];
}

inline void logger(const std::string& str) {
    std::cout << "[" << log_time() << " LOG] " << str << std::endl;
}
inline void warn(const std::string& str) {
    std::cerr << "\033[33m[" << log_time() << " WARN] " << str << "\033[0m" << std::endl;
}
inline void error(const std::string& str) {
    std::cerr << "\033[31m[" << log_time() << " ERROR] " << str << "\033[0m" << std::endl;
}

inline void run(const std::string& str) {
    std::cerr << "\033[32m[" << log_time() << " RUN] " << str << "\033[0m" << std::endl;
}

inline void st() {
    std::cout << "--------------------------------------------------------------------------" << std::endl;
}

inline void infost() {
    std::cout << "\033[38;2;135;206;235m"
        << "--------------------------------------------------------------------------"
        << "\033[0m" << std::endl;
}

inline void info(const std::string& str) {
    std::cout << "\033[38;2;135;206;235m" << log_time()
        << " CONFIG] " << str << "\033[0m" << std::endl;
}
