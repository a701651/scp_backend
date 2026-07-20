#include"getmail.h"
#include"common.h"
#include"sqlsave.h"
#include "othertool.h"
#include "config.h"
#include "gzpi.h"
using namespace std;
using json = nlohmann::json;
/*
* 0: 成功
* 1: token为空
* 2: 权限不足
* 3: token无效
* 4: 查询失败
*/
int maillist(const string& token, string& out) {
    if (token.empty()) return 1;

    auto opt_user = check_permission_get_user(token, { "get_mail" });
    if (!opt_user.has_value()) return 2;

    auto pool = g_db->getPool();
    int limit = GlobalConfig->server.mail_max_count;
    auto mails = pool->id_user(opt_user->id, limit);

    json arr = json::array();
    for (const auto& m : mails) {
        json obj;
        obj["id"] = m.id;
        obj["user_id"] = m.user_id;
        obj["title"] = m.title;
        obj["content"] = m.content;
        obj["send_time"] = m.send_time;
        obj["is_new"] = m.is_new;
        obj["form_name"] = m.form_name;
        obj["out_time"] = m.out_time;
        obj["is_claim"] = m.is_claim;
        if (!m.item_json.empty()) {
            try { obj["item"] = json::parse(m.item_json); }
            catch (...) { obj["item"] = m.item_json; }
        }
        else {
            obj["item"] = nullptr;
        }
        arr.push_back(obj);
    }

    std::string raw = arr.dump();
    out = j2gzip(raw);
    return 0;
}
