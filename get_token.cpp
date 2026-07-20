#include"get_token.h"
#include"sqlsave.h"
#include "othertool.h"
using namespace std;
/*状态码:
* 0:成功
* 1:token为空
* 2:token无效
* 3:token过期
*/
int gettoken(const std::string& token, std::string& out) {
    if (token.empty()) return 1;
    auto pool = g_db->getPool();
    auto search = pool->logintoken(token);
    if (!search.has_value()) return 2;
    if (search->expire_time < std::time(nullptr)) return 3;
    std::string new_token = encrypt::GenerateToken();
    if (!pool->s_usertoken(search->user_id, new_token)) return 5;
    out = new_token;
    return 0;
}
