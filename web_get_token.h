#pragma once
#include"common.h"
using namespace std;
/*状态码:
* 0:成功
* 1:token为空
* 2:token无效
* 3:token过期
* 4:token错误
*/
int webgettoken(const string& token, string& out);