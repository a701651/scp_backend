#include"login.h"
#include"common.h"
#include"sqlsave.h"
#include "othertool.h"
#include "config.h"
#include"log-system.h"
using namespace std;

namespace {
    constexpr array<bool, 256> makeCharTable() {
        array<bool, 256> t{};
        for (char c : "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789_") {
            t[static_cast<unsigned char>(c)] = true;
        }
        return t;
    }
    static constexpr auto kValidChar = makeCharTable();

    // [FIX] device_name 白名单：允许 [a-zA-Z0-9_ .:\-]
    //       保证文本 SQL 拼接安全（login_once 用 Statement 拼接 device_name）
    constexpr array<bool, 256> makeDeviceCharTable() {
        array<bool, 256> t = makeCharTable();
        t[static_cast<unsigned char>(' ')] = true;
        t[static_cast<unsigned char>('.')] = true;
        t[static_cast<unsigned char>(':')] = true;
        t[static_cast<unsigned char>('-')] = true;
        return t;
    }
    static constexpr auto kValidDeviceChar = makeDeviceCharTable();
}

/*
    * 状态码：
    * 0：成功
    * 1：用户名不存在
    * 2：用户名或密码为空
    * 3：用户名或密码长度不合法
    * 4：用户名或密码包含非法字符
    * 5: 插入失败
    * 6: 用户名或密码错误
    */
int log(const string& username, const string& pass,
    const string& ip, const string& device,
    string& out_token) {

    if (username.empty() || pass.empty()) return 2;

    const size_t ulen = username.size();
    const size_t plen = pass.size();
    if (ulen < 6 || ulen > 20 || plen < 6 || plen > 20) return 3;

    for (size_t i = 0; i < ulen; ++i)
        if (!kValidChar[static_cast<unsigned char>(username[i])]) return 4;
    for (size_t i = 0; i < plen; ++i)
        if (!kValidChar[static_cast<unsigned char>(pass[i])]) return 4;

    // [FIX] device_name 白名单校验 — 文本 SQL 安全前提
    if (!device.empty()) {
        for (size_t i = 0; i < device.size(); ++i)
            if (!kValidDeviceChar[static_cast<unsigned char>(device[i])]) return 4;
    }

    string md5pass = encrypt::MD5(pass);
    string token = TokenCache::Get();       // [FIX] 预生成池，避免 BCryptGenRandom
    int64_t now = time(nullptr);
    int64_t expire = now + GlobalConfig->server.login_expire;

    auto pool = g_db->getPool();
    int ret = pool->login_once(username, md5pass, ip, device, now, expire, token);

    if (ret == 0) {
        out_token = std::move(token);
    }
    return ret;
}
