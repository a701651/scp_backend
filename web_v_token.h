#pragma once
#include"common.h"
using namespace std;
using json = nlohmann::json;
/*状态码:
* 0:成功
* 1:token为空
* 2:token无效
* 3:token已使用
* 4:消费失败
* 5:用户不存在
*/
int web_v_token(const string& token, json& out);