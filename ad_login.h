#pragma once
#include"common.h"
using namespace std;
/*
	* 状态码：
	* 0：成功
	* 1：用户名不存在
	* 2：用户名或密码为空
	* 3：用户名或密码长度不合法
	* 4：用户名或密码包含非法字符
	* 5: 插入失败
	* 6: 用户名或密码错误
	* 7: 权限不足
	*/
int adlog(const string& username, const string& pass,const string& ip, const string& device,string& out_token);
