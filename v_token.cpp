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
    auto opt_user = check_permission_get_user(token, { "get_user_verify_token" });
    if (!opt_user.has_value()) return 4;   // 无权限 或 token无效
    auto pool = g_db->getPool();
    std::string vtoken = encrypt::GenerateToken();
    if (!pool->s_verifytoken(opt_user->id, vtoken)) return 3;
    out = vtoken;
    return 0;
}
