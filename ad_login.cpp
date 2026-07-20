#include"ad_login.h"
#include"common.h"
#include"sqlsave.h"
#include "othertool.h"
#include "config.h"
#include "LOG.h"
using namespace std;
/*
	* 状态码：
	* 0：成功
	* 1：用户名不存在
	* 2：用户名或密码为空
	* 3：用户名或密码长度不合法
	* 4：用户名或密码包含非法字符
	* 5: 插入失败
	* 6: 用户名或密码错误
	* 7:权限不足
	*/
int adlog(const string& username, const string& pass,const string& ip, const string& device,string& out_token)
{
    if (username.empty() || pass.empty()) return 2;

    if (username.length() < 6 || username.length() > 20) return 3;
    if (pass.length() < 6 || pass.length() > 20) return 3;

    const string allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    if (username.find_first_not_of(allowed) != string::npos) return 4;
    if (pass.find_first_not_of(allowed) != string::npos) return 4;

    auto pool = g_db->getPool();
    auto search = pool->nickname_user(username);
    if (!search.has_value()) return 6;

    string md5pass = encrypt::MD5(pass);
    if (search->password != md5pass) return 6;

    if (!search->is_official) return 7;

    const PermissionMap* perms = g_db->perm_cache.get(search->permission_id);
    if (!perms) return 7;
    auto it = perms->find("login_admin");
    if (it == perms->end() || it->second != 1) return 7;

    int64_t now = time(nullptr);
    string token = encrypt::GenerateToken();
    int64_t expire = now + GlobalConfig->server.login_expire;
    if (!pool->s_logintoken(search->id, token, ip, "Admin Login", now, expire))
        return 5;

    out_token = token;
    return 0;
}
