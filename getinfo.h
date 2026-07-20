#pragma once
#include"common.h"
using namespace std;

using json = nlohmann::json;
/*状态码:
* 0:成功
* 1:token为空
* 2:token无效
* 3:token错误
*/
int getinfo(const string& token, json& out);
