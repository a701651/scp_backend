#include"register.h"
#include"common.h"
#include"sqlsave.h"
#include "othertool.h"
#include "config.h"
#include "LOG.h"
using namespace std;
/*
	* 状态码：
	* 0：成功
	* 1：用户名已存在
	* 2：用户名或密码为空
	* 3：用户名或密码长度不合法
	* 4：用户名或密码包含非法字符
	* 5: 插入失败
	*/
int reg(const string& username, const string& pass,
	const string& ip, const string& device,
	string& out_token)
{
	if (username.empty() || pass.empty()) return 2;
	auto pool = g_db->getPool();
	optional<user> search = pool->nickname_user(username);
	if (search.has_value()) return 1;

	if (username.length() < 6 || username.length() > 20) return 3;
	if (pass.length() < 6 || pass.length() > 20) return 3;

	if (username.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != string::npos) return 4;
	if (pass.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != string::npos) return 4;

	string md5pass = encrypt::MD5(pass);
	int64_t now = time(nullptr);
	int64_t user_id = 0;
	if (!pool->s_user(username, md5pass, now, user_id)) {
		return 5;
	}
	string token = encrypt::GenerateToken();
	int64_t expire = now + GlobalConfig->server.login_expire;
	if (!pool->s_logintoken(user_id, token, ip, device, now, expire)) {
		return 5;
	}

	out_token = token;
	return 0;
}