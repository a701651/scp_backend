#pragma once
#include"common.h"
using namespace std;

/*
* 0: 成功
* 1: token为空
* 2: 权限不足
* 3: token无效
* 4: 查询失败
*/
int maillist(const string& token, string& out);