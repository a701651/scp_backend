#include"web_get_token.h"
#include"sqlsave.h"
#include "othertool.h"
using namespace std;
/*状态码:
* 0:成功
* 1:token为空
* 2:token无效
* 3:token过期
*/
int webgettoken(const std::string& token, std::string& out) {
    if (token.empty()) return 1;
    auto pool = g_db->getPool();
    auto search = pool->logintoken(token);
    if (!search.has_value()) return 2;
    if (search->expire_time < std::time(nullptr)) return 3;
    auto user = pool->id_user(search->user_id);
    if (!user.has_value()) return 5;
    out = user->token;
    return 0;
}

