#pragma once
#include "common.h"

using namespace std::chrono_literals;
extern bool isdebug;
inline std::string time_buf[2];
inline std::atomic<int> time_idx{ 0 };

inline void cctime() {
    while (true) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

        int write = 1 - time_idx.load(std::memory_order_acquire);
        time_buf[write] = buf;
        time_idx.store(write, std::memory_order_release);

        std::this_thread::sleep_for(1s);
    }
}

inline const std::string& log_time() {
    return time_buf[time_idx.load(std::memory_order_acquire)];
}

struct LogEntry {
    char msg[256];
    int  level;
};

constexpr size_t RING_SIZE = 4096;
static_assert((RING_SIZE& (RING_SIZE - 1)) == 0, "RING_SIZE must be power of 2");

inline std::atomic<size_t> ring_head{ 0 };
inline std::atomic<size_t> ring_tail{ 0 };
inline LogEntry           ring_buffer[RING_SIZE];

inline bool try_push(const char* data, size_t len, int level) {
    size_t head = ring_head.load(std::memory_order_relaxed);
    size_t next = (head + 1) & (RING_SIZE - 1);
    if (next == ring_tail.load(std::memory_order_acquire))
        return false;

    auto& entry = ring_buffer[head];
    size_t copy_len = (len < sizeof(entry.msg) - 1) ? len : (sizeof(entry.msg) - 1);
    std::memcpy(entry.msg, data, copy_len);
    entry.msg[copy_len] = '\0';
    entry.level = level;

    ring_head.store(next, std::memory_order_release);
    return true;
}

inline bool try_pop(LogEntry& out) {
    size_t tail = ring_tail.load(std::memory_order_relaxed);
    if (tail == ring_head.load(std::memory_order_acquire))
        return false;

    out = ring_buffer[tail];
    ring_tail.store((tail + 1) & (RING_SIZE - 1), std::memory_order_release);
    return true;
}

inline std::atomic<bool> log_running{ true };
inline std::thread       g_writer_thread;
inline std::thread       g_time_thread;

inline void log_writer() {
    LogEntry entry;
    while (log_running.load(std::memory_order_relaxed)) {
        while (try_pop(entry)) {
            const char* color_before = "";
            const char* level_tag = " LOG";
            FILE* out = stdout;

            switch (entry.level) {
            case 0: level_tag = " LOG";    out = stdout; break;
            case 1: level_tag = " WARN";   out = stderr; color_before = "\033[33m"; break;
            case 2: level_tag = " ERROR";  out = stderr; color_before = "\033[31m"; break;
            case 3: level_tag = " RUN";    out = stderr; color_before = "\033[32m"; break;
            case 4: level_tag = " CONFIG"; out = stdout; color_before = "\033[38;2;135;206;235m"; break;
            case 7: level_tag = " DEBUG";  out = stdout; color_before = "\033[90m"; break;
            case 5:
                fprintf(stdout, "%s\n", entry.msg);
                continue;
            case 6:
                fprintf(stdout, "\033[38;2;135;206;235m%s\033[0m\n", entry.msg);
                continue;
            }

            fprintf(out, "%s[%s%s] %s\033[0m\n",
                color_before, log_time().c_str(), level_tag, entry.msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    while (try_pop(entry)) {
        fprintf(stdout, "[%s LOG] %s\n", log_time().c_str(), entry.msg);
    }
}

inline void log_start() {
    static std::once_flag flag;
    std::call_once(flag, [] {
        g_time_thread = std::thread(cctime);
        g_writer_thread = std::thread(log_writer);
    });
}

inline void log_stop() {
    log_running.store(false, std::memory_order_release);
    if (g_writer_thread.joinable()) g_writer_thread.join();
    if (g_time_thread.joinable())   g_time_thread.detach();  
}

inline void logger(const std::string& str) { try_push(str.data(), str.size(), 0); }
inline void warn(const std::string& str) { try_push(str.data(), str.size(), 1); }
inline void error(const std::string& str) { try_push(str.data(), str.size(), 2); }
inline void run(const std::string& str) { try_push(str.data(), str.size(), 3); }
inline void info(const std::string& str) { try_push(str.data(), str.size(), 4); }

inline void st() {
    constexpr const char* line = "--------------------------------------------------------------------------";
    try_push(line, sizeof(line) - 1, 5);
}
inline void infost() {
    constexpr const char* line = "--------------------------------------------------------------------------";
    try_push(line, sizeof(line) - 1, 6);
}
inline void debug(const std::string& str) {
    if (!isdebug) return; 
    try_push(str.data(), str.size(), 7);
}