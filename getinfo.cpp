#include"getinfo.h"
#include"common.h"
#include"sqlsave.h"
using namespace std;
using json = nlohmann::json;

int getinfo(const string& token, json& out) {
    if (token.empty()) return 1;
    auto pool = g_db->getPool();

    auto user = pool->token_user(token);
    if (!user.has_value()) return 2;

    json obj;
    obj["id"] = user->id;
    obj["nickname"] = user->nickname;
    obj["is_official"] = user->is_official;
    obj["avatar"] = user->avatar;
    obj["username"] = user->username;
    obj["sign"] = user->sign;
    obj["email"] = user->email;
    obj["permission_id"] = user->permission_id;
    obj["user_group"] = user->user_group;

    out = obj;
    return 0;
}
