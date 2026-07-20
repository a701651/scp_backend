#pragma once
#include <string>
struct ServerConfig {
    std::string port = ":3000";
    int         login_expire = 604800;
    int         mail_max_count = 50;
};

struct DatabaseConfig {
    std::string host;
    int         port = 3306;
    std::string user;
    std::string password;
    std::string dbname;
    std::string charset = "utf8mb4";
    int         max_open_conns = 20;
    int         max_idle_conns = 10;
    int         conn_max_lifetime = 600;
    int         conn_max_idle_time = 300;
    int         connect_timeout = 10;
    int         read_timeout = 30;
    int         write_timeout = 30;
};

struct Config {
    ServerConfig   server;
    DatabaseConfig database;
};
bool creat_config(const std::string& path);
bool load_config(const std::string& path);
int  port(const std::string& poth);
bool file_ex(const std::string& path);
extern Config* GlobalConfig;          // 声明，不是定义



