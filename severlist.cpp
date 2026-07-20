#include"severlist.h"
#include"common.h"
#include"sqlsave.h"
#include"gzpi.h"
using namespace std;
using json = nlohmann::json;

/*
* 0: ³É¹¦
*/
int severlist(string& out) {
    int64_t now = time(nullptr);
    json arr = json::array();
    for (const auto& [id, s] : g_db->servers_by_id) {
        if (s->last_updata < now) continue;
        json obj;
        obj["id"] = s->id;
        obj["user_id"] = s->user_id;
        obj["ip"] = s->ip;
        obj["name"] = s->name;
        obj["introduction"] = s->introduction;
        obj["type"] = s->type;
        obj["is_test"] = s->is_test;
        obj["last_updata"] = s->last_updata;
        arr.push_back(obj);
    }
    out = j2gzip(arr.dump());
    return 0;
}
