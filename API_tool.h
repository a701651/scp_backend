#pragma once
#include"common.h"
using json = nlohmann::json;
using namespace std;
//返回JSON结果
struct resjson {
    int code;
    string msg;
    json data;
};

//http解析用通用结构体
struct httpplt
{
    string username;
    string password;
    string token;
    string device_name;
    bool isget;
    int code;
    string path;
    /*code是api操作码
    * 管理员登录		1
    * 用户列表			2
    * 登录账号			3
    * 注册账号			4
    * 获取用户邮件		5
    * 获取用户token		6
    * 获取信息			7
    * 获取验证token		8
    * 服务器验证用户	9
    * 获取用户token		10
    * 获取服务器列表	11
    * 获取API是否正常	12
    * 获取服务器状态	13
    */
};