#include "common.h"
#include "othertool.h"
#include "sqlsave.h"
//MD5
namespace {
    constexpr uint32_t S11 = 7, S12 = 12, S13 = 17, S14 = 22;
    constexpr uint32_t S21 = 5, S22 = 9, S23 = 14, S24 = 20;
    constexpr uint32_t S31 = 4, S32 = 11, S33 = 16, S34 = 23;
    constexpr uint32_t S41 = 6, S42 = 10, S43 = 15, S44 = 21;

    inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
    inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
    inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
    inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }
    inline uint32_t ROTATE_LEFT(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

    inline void FF(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
        a += F(b, c, d) + x + ac;
        a = ROTATE_LEFT(a, s);
        a += b;
    }
    inline void GG(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
        a += G(b, c, d) + x + ac;
        a = ROTATE_LEFT(a, s);
        a += b;
    }
    inline void HH(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
        a += H(b, c, d) + x + ac;
        a = ROTATE_LEFT(a, s);
        a += b;
    }
    inline void II(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
        a += I(b, c, d) + x + ac;
        a = ROTATE_LEFT(a, s);
        a += b;
    }
    void MD5Transform(uint32_t state[4], const uint8_t block[64]) {
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t x[16];
        for (int i = 0; i < 16; ++i)
            x[i] = (uint32_t)(block[i * 4]) |
            ((uint32_t)(block[i * 4 + 1]) << 8) |
            ((uint32_t)(block[i * 4 + 2]) << 16) |
            ((uint32_t)(block[i * 4 + 3]) << 24);

        FF(a, b, c, d, x[0], S11, 0xd76aa478);
        FF(d, a, b, c, x[1], S12, 0xe8c7b756);
        FF(c, d, a, b, x[2], S13, 0x242070db);
        FF(b, c, d, a, x[3], S14, 0xc1bdceee);   // ← S22 → S14
        FF(a, b, c, d, x[4], S11, 0xf57c0faf);
        FF(d, a, b, c, x[5], S12, 0x4787c62a);
        FF(c, d, a, b, x[6], S13, 0xa8304613);
        FF(b, c, d, a, x[7], S14, 0xfd469501);   // ← S22 → S14
        FF(a, b, c, d, x[8], S11, 0x698098d8);
        FF(d, a, b, c, x[9], S12, 0x8b44f7af);
        FF(c, d, a, b, x[10], S13, 0xffff5bb1);
        FF(b, c, d, a, x[11], S14, 0x895cd7be);   // ← S22 → S14
        FF(a, b, c, d, x[12], S11, 0x6b901122);
        FF(d, a, b, c, x[13], S12, 0xfd987193);
        FF(c, d, a, b, x[14], S13, 0xa679438e);
        FF(b, c, d, a, x[15], S14, 0x49b40821);   // ← S22 → S14

        GG(a, b, c, d, x[1], S21, 0xf61e2562);
        GG(d, a, b, c, x[6], S22, 0xc040b340);
        GG(c, d, a, b, x[11], S23, 0x265e5a51);
        GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);
        GG(a, b, c, d, x[5], S21, 0xd62f105d);
        GG(d, a, b, c, x[10], S22, 0x02441453);
        GG(c, d, a, b, x[15], S23, 0xd8a1e681);
        GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);
        GG(a, b, c, d, x[9], S21, 0x21e1cde6);
        GG(d, a, b, c, x[14], S22, 0xc33707d6);
        GG(c, d, a, b, x[3], S23, 0xf4d50d87);
        GG(b, c, d, a, x[8], S24, 0x455a14ed);
        GG(a, b, c, d, x[13], S21, 0xa9e3e905);
        GG(d, a, b, c, x[2], S22, 0xfcefa3f8);
        GG(c, d, a, b, x[7], S23, 0x676f02d9);
        GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);

        HH(a, b, c, d, x[5], S31, 0xfffa3942);
        HH(d, a, b, c, x[8], S32, 0x8771f681);
        HH(c, d, a, b, x[11], S33, 0x6d9d6122);
        HH(b, c, d, a, x[14], S34, 0xfde5380c);
        HH(a, b, c, d, x[1], S31, 0xa4beea44);
        HH(d, a, b, c, x[4], S32, 0x4bdecfa9);
        HH(c, d, a, b, x[7], S33, 0xf6bb4b60);
        HH(b, c, d, a, x[10], S34, 0xbebfbc70);
        HH(a, b, c, d, x[13], S31, 0x289b7ec6);
        HH(d, a, b, c, x[0], S32, 0xeaa127fa);
        HH(c, d, a, b, x[3], S33, 0xd4ef3085);
        HH(b, c, d, a, x[6], S34, 0x04881d05);
        HH(a, b, c, d, x[9], S31, 0xd9d4d039);
        HH(d, a, b, c, x[12], S32, 0xe6db99e5);
        HH(c, d, a, b, x[15], S33, 0x1fa27cf8);
        HH(b, c, d, a, x[2], S34, 0xc4ac5665);

        II(a, b, c, d, x[0], S41, 0xf4292244);
        II(d, a, b, c, x[7], S42, 0x432aff97);
        II(c, d, a, b, x[14], S43, 0xab9423a7);
        II(b, c, d, a, x[5], S44, 0xfc93a039);
        II(a, b, c, d, x[12], S41, 0x655b59c3);
        II(d, a, b, c, x[3], S42, 0x8f0ccc92);
        II(c, d, a, b, x[10], S43, 0xffeff47d);
        II(b, c, d, a, x[1], S44, 0x85845dd1);
        II(a, b, c, d, x[8], S41, 0x6fa87e4f);
        II(d, a, b, c, x[15], S42, 0xfe2ce6e0);
        II(c, d, a, b, x[6], S43, 0xa3014314);
        II(b, c, d, a, x[13], S44, 0x4e0811a1);
        II(a, b, c, d, x[4], S41, 0xf7537e82);
        II(d, a, b, c, x[11], S42, 0xbd3af235);
        II(c, d, a, b, x[2], S43, 0x2ad7d2bb);
        II(b, c, d, a, x[9], S44, 0xeb86d391);

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
    }

}

std::string encrypt::MD5(const std::string& input) {
    uint32_t state[4] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };
    std::vector<uint8_t> msg(input.begin(), input.end());
    uint64_t bitLen = msg.size() * 8;
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) msg.push_back(0x00);
    for (int i = 0; i < 8; ++i) {
        msg.push_back((bitLen >> (i * 8)) & 0xff);
    }
    for (size_t i = 0; i < msg.size(); i += 64)
        MD5Transform(state, &msg[i]);
    std::ostringstream oss;
    for (int i = 0; i < 4; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(2) << ((state[i] >> 0) & 0xff);
        oss << std::hex << std::setfill('0') << std::setw(2) << ((state[i] >> 8) & 0xff);
        oss << std::hex << std::setfill('0') << std::setw(2) << ((state[i] >> 16) & 0xff);
        oss << std::hex << std::setfill('0') << std::setw(2) << ((state[i] >> 24) & 0xff);
    }
    return oss.str();
}
std::string encrypt::GenerateToken() {
    uint8_t buffer[32];
    NTSTATUS status = BCryptGenRandom(nullptr, buffer, sizeof(buffer), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (status != 0) {
        for (int i = 0; i < sizeof(buffer); ++i)
            buffer[i] = (uint8_t)(std::rand() % 256);
    }
    std::ostringstream oss;
    for (auto b : buffer) oss << std::hex << std::setfill('0') << std::setw(2) << (int)b;
    return oss.str();
}

//格式化
std::string FormatPercent(double value, double total) {
    if (total == 0) return "0.00%";
    double percent = (value / total) * 100.0;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << percent << "%";
    return oss.str();
}

//CPU 监控
double GetCpuUsage() {
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
        return 0.0;
    auto FileTimeToUInt64 = [](const FILETIME& ft) -> ULONGLONG {
        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        return uli.QuadPart;
    };
    ULONGLONG idle1 = FileTimeToUInt64(idleTime);
    ULONGLONG kernel1 = FileTimeToUInt64(kernelTime);
    ULONGLONG user1 = FileTimeToUInt64(userTime);
    Sleep(1000);
    FILETIME idleTime2, kernelTime2, userTime2;
    if (!GetSystemTimes(&idleTime2, &kernelTime2, &userTime2))
        return 0.0;
    ULONGLONG idle2 = FileTimeToUInt64(idleTime2);
    ULONGLONG kernel2 = FileTimeToUInt64(kernelTime2);
    ULONGLONG user2 = FileTimeToUInt64(userTime2);
    ULONGLONG total1 = kernel1 + user1;
    ULONGLONG total2 = kernel2 + user2;
    ULONGLONG idleDelta = idle2 - idle1;
    ULONGLONG totalDelta = total2 - total1;
    if (totalDelta == 0) return 0.0;
    return ((totalDelta - idleDelta) * 100.0) / totalDelta;
}

// CPU 缓存（避免 /status 阻塞 1 秒）
static std::atomic<double> g_cpu_usage{ 0.0 };
static std::atomic<bool>   g_cpu_monitor_running{ false };

void StartCpuMonitor() {
    if (g_cpu_monitor_running.exchange(true)) return;
    std::thread([]() {
        while (true) {
            g_cpu_usage.store(GetCpuUsage(), std::memory_order_relaxed);
        }
    }).detach();
}

double GetCpuUsageCached() {
    return g_cpu_usage.load(std::memory_order_relaxed);
}

//内存监控
MemInfo GetMemoryInfo() {
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (!GlobalMemoryStatusEx(&memStatus)) {
        return { 0.0, 0, 0 };
    }
    double percent = static_cast<double>(memStatus.dwMemoryLoad);
    ULONGLONG used = memStatus.ullTotalPhys - memStatus.ullAvailPhys;
    ULONGLONG total = memStatus.ullTotalPhys;
    return { percent, used, total };
}

//服务器状态
resjson status() {
    resjson result;
    try {
        double cpuUsage = GetCpuUsageCached();
        MemInfo mem = GetMemoryInfo();
        json obj;
        obj["CPUUsage"] = FormatPercent(cpuUsage, 100.0);
        obj["MemUsagePercent"] = FormatPercent(mem.percent, 100.0);
        obj["MemUsage"] = mem.usedBytes;
        obj["MemMax"] = mem.totalBytes;
        result.data = json::array({ obj });
        result.code = 200;
        result.msg = "success";
    }
    catch (...) {
        result.code = 500;
        result.msg = "获取状态失败";
        result.data = json::object();
    }
    return result;
}

//路径工具
std::wstring getpath() {
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW(NULL, exe_path, MAX_PATH);
    std::wstring exe_dir(exe_path);
    exe_dir = exe_dir.substr(0, exe_dir.find_last_of(L"\\/") + 1);
    std::wstring lib_path = exe_dir + L"Lib";
    return lib_path;
}
//URL解码
std::string url_decode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            // 尝试解析 % 后面的两位十六进制数
            int high = 0, low = 0;
            auto h = str[i + 1], l = str[i + 2];
            if (h >= '0' && h <= '9') high = h - '0';
            else if (h >= 'A' && h <= 'F') high = h - 'A' + 10;
            else if (h >= 'a' && h <= 'f') high = h - 'a' + 10;
            else goto normal_char;

            if (l >= '0' && l <= '9') low = l - '0';
            else if (l >= 'A' && l <= 'F') low = l - 'A' + 10;
            else if (l >= 'a' && l <= 'f') low = l - 'a' + 10;
            else goto normal_char;

            result.push_back(static_cast<char>((high << 4) | low));
            i += 2;  // 跳过两个 hex 字符
        }
        else {
        normal_char:
            if (str[i] == '+')  // 可选的：+ 通常表示空格
                result.push_back(' ');
            else
                result.push_back(str[i]);
        }
    }
    return result;
}

//权限检查
bool check_permission(const std::string& user_token,
    const std::vector<std::string>& required_perms) {
    auto pool = g_db->getPool();
    auto opt_user = pool->token_user(user_token);
    if (!opt_user) return false;
    //封禁检查
    if (opt_user->end_ban_time > std::time(nullptr)) return false;
    const PermissionMap* perms = g_db->perm_cache.get(opt_user->permission_id);
    if (!perms) return false;
    for (const auto& perm : required_perms) {
        auto it = perms->find(perm);
        if (it == perms->end() || it->second != 1) return false;
    }
    return true;
}
std::optional<user> check_permission_get_user(
    const std::string& user_token,
    const std::vector<std::string>& required_perms) {
    auto pool = g_db->getPool();
    auto opt_user = pool->token_user(user_token);
    if (!opt_user) return std::nullopt;
    if (opt_user->end_ban_time > std::time(nullptr)) return std::nullopt;
    const PermissionMap* perms = g_db->perm_cache.get(opt_user->permission_id);
    if (!perms) return std::nullopt;
    for (const auto& perm : required_perms) {
        auto it = perms->find(perm);
        if (it == perms->end() || it->second != 1) return std::nullopt;
    }
    return opt_user;
}
std::string find_mysql_plugin_dir() {
    namespace fs = std::filesystem;

    constexpr const char* TARGET_DLL = "mysql_native_password.dll";

    // 策略 1：exe 目录\Lib\plugin
    fs::path lib_plugin = fs::path(getpath()) / "plugin";
    if (fs::exists(lib_plugin / TARGET_DLL)) {
        return lib_plugin.string();
    }

    // 策略 2：扫描 MySQL Connector 安装目录
    auto try_add = [](std::vector<fs::path>& paths, const char* env_name) {
        char buf[MAX_PATH];
        DWORD len = GetEnvironmentVariableA(env_name, buf, MAX_PATH);
        if (len > 0 && len < MAX_PATH) {
            paths.push_back(fs::path(buf) / "MySQL");
        }
    };

    std::vector<fs::path> search_bases;
    try_add(search_bases, "ProgramFiles");
    try_add(search_bases, "ProgramFiles(x86)");
    try_add(search_bases, "ProgramW6432");
    search_bases.push_back("C:/Program Files/MySQL");
    search_bases.push_back("C:/Program Files (x86)/MySQL");

    for (const auto& base : search_bases) {
        if (!fs::exists(base)) continue;
        try {
            for (const auto& entry : fs::directory_iterator(base)) {
                if (!entry.is_directory()) continue;
                std::string name = entry.path().filename().string();
                if (name.find("onnector") == std::string::npos &&
                    name.find("ONNECTOR") == std::string::npos) continue;

                for (const char* sub : { "lib64/plugin", "lib/plugin" }) {
                    fs::path candidate = entry.path() / sub;
                    if (fs::exists(candidate / TARGET_DLL)) {
                        return candidate.string();
                    }
                }
            }
        }
        catch (...) {}
    }

    return "";
}
