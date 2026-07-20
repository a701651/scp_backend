#pragma once
#include"common.h"
using namespace std;
using json = nlohmann::json;
/*
	* 状态码：
	* 0：成功
	* 1：token为空
	* 2：权限不足
	* 3: 查询失败
	*/
int adlist(const string& token, int page, json& out);