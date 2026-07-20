#include"web_v_token.h"
#include"sqlsave.h"
#include "othertool.h"
int web_v_token(const string& token, json& out) {
    if (token.empty()) return 1;
    auto pool = g_db->getPool();
    auto vt = pool->vetoken(token);
    if (!vt.has_value()) return 2;
    if (vt->is_use == 1) return 3;
    if (!pool->use_verifytoken(token)) return 4;
    auto user = pool->id_user(vt->user_id);
    if (!user.has_value()) return 5;

    out["id"] = user->id;
    out["nickname"] = user->nickname;
    return 0;
}
