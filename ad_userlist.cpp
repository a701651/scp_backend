#include"common.h"
#include"ad_userlist.h"
#include "othertool.h"
#include "sqlsave.h"
using namespace std;
using json = nlohmann::json;
/*
	* 状态码：
	* 0：成功
	* 1：token为空
	* 2：权限不足
	* 3: 查询失败
	*/
int adlist(const string& token, int page, json& out) {
    if (token.empty()) return 1;
    if (!check_permission(token, { "get_user_list" })) return 2;
    if (page < 1) page = 1;
    int l = 30;
    int offset = (page - 1) * l;
    auto pool = g_db->getPool();
    auto users = pool->get_user_list(offset, l);
    json arr = json::array();
    for (const auto& u : users) {
        json obj;
        obj["id"] = u.id;
        obj["nickname"] = u.nickname;
        obj["username"] = u.username;
        obj["avatar"] = u.avatar;
        obj["sign"] = u.sign;
        obj["is_official"] = u.is_official;
        obj["permission_id"] = u.permission_id;
        obj["user_group"] = u.user_group;
        obj["registration_time"] = u.registration_time;
        obj["last_time"] = u.last_time;
        obj["ban_reason"] = u.ban_reason;
        obj["start_ban_time"] = u.start_ban_time;
        obj["end_ban_time"] = u.end_ban_time;
        arr.push_back(obj);
    }
    json rout;
    rout["users"] = arr;
    out = rout;
    return 0;
}

