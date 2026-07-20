#pragma once
#include"common.h"
using namespace std;
/*状态码:
* 0:成功
* 1:token为空
* 2:token无效
* 3:生成失败
* 4:无权限
*/
int v_token(const string& token,string& out);