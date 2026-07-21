#pragma once
#include "common.h"
#include "API_tool.h"
#include "sqlsave.h"
using json = nlohmann::json;

// 加密工具
class encrypt {
public:
    static std::string MD5(const std::string& input);
    static std::string GenerateToken();
};
class TokenCache {
public:
    static std::string Get();
};
// 格式化
std::string FormatPercent(double value, double total);

// 系统状态
struct MemInfo {
    double percent;
    ULONGLONG usedBytes;
    ULONGLONG totalBytes;
};
double GetCpuUsage();
MemInfo GetMemoryInfo();
resjson status();

// 路径工具
std::wstring getpath();
//通用URL解码
std::string url_decode(const std::string& str);

bool check_permission(const std::string& user_token,const std::vector<std::string>& required_perms);
std::optional<user> check_permission_get_user(
    const std::string& user_token,
    const std::vector<std::string>& required_perms);
void StartCpuMonitor();
double GetCpuUsageCached();
std::string find_mysql_plugin_dir();