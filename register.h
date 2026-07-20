#pragma once
#include"common.h"
using namespace std;
/*
	* 状态码：
	* 0：成功
	* 1：用户名已存在
	* 2：用户名或密码为空
	* 3：用户名或密码长度不合法
	* 4：用户名或密码包含非法字符
	*/
int reg(const string& username, const string& pass, const string& ip, const string& device, string& out_token);