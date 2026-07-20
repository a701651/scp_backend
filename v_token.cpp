#include"v_token.h"
#include"common.h"
#include"sqlsave.h"
#include"othertool.h"
using namespace std;
/*状态码:
* 0:成功
* 1:token为空
* 2:token无效
* 3:生成失败
* 4:无权限
*/
int v_token(const std::string& token, std::string& out) {
    if (token.empty()) return 1;
    if (!check_permission(token, { "get_user_verify_token" })) return 4;

    auto pool = g_db->getPool();
    auto user = pool->token_user(token);
    if (!user.has_value()) return 2;
    std::string vtoken = encrypt::GenerateToken();
    if (!pool->s_verifytoken(user->id, vtoken)) return 3;
    out = vtoken;
    return 0;
}
