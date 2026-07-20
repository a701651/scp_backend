#pragma once
#include "common.h"
#include <shared_mutex>

//---------- 数据模型 ----------
struct user {
    int64_t id = 0;
    std::string nickname;
    bool is_official = false;
    std::string avatar;
    std::string username;
    std::string password;
    int permission_id = 1;
    std::string token;
    std::string api_key;
    std::string email;
    std::string sign;
    int user_group = 1;
    std::string login_ip;
    int64_t registration_time = 0;
    int64_t last_time = 0;
    std::string ban_reason;
    int64_t start_ban_time = 0;
    int64_t end_ban_time = 0;
    bool dirty = false;
};

struct login_token {
    int64_t id = 0;
    int64_t user_id = 0;
    std::string token;
    std::string ip;
    int64_t create_time = 0;
    std::string device_name;
    int64_t expire_time = 0;
    bool dirty = false;
};

using PermissionMap = std::unordered_map<std::string, int>;
class PermissionCache {
public:
    void loadAll(std::shared_ptr<class ConnectionPool> pool);
    const PermissionMap* get(int64_t permission_id) const;
    void refresh(std::shared_ptr<class ConnectionPool> pool);

private:
    mutable std::shared_mutex mtx_;
    std::unordered_map<int64_t, PermissionMap> cache_;
};

//连接池
class ConnectionPool {
public:
    ConnectionPool(const std::string& host, int port,
        const std::string& user, const std::string& passwd,
        const std::string& schema, int poolSize = 10);
    ~ConnectionPool();

    //通过 user.token 查询用户
    std::optional<user> token_user(const std::string& token);

    //通过用户名查询用户
    std::optional<user> nickname_user(const std::string& username);

    //通过用户ID查询用户
    std::optional<user> id_user(int64_t user_id);

    //通过 login_token 字符串查询会话记录
    std::optional<login_token> logintoken(const std::string& token);

    //通过 verify_token 查询一次性令牌
    std::optional<struct verify_token> vetoken(const std::string& token);

    //查询某用户的邮件列表
    std::vector<struct mail> id_user(int64_t user_id, int limit);

    //通过 token 查用户
    std::optional<user> token_id(const std::string& token);

    //通过用户名查用户
    std::optional<user> queryUserByUsername(const std::string& username);

    //管理员查全量用户列表
    std::vector<user> get_user_list(int offset, int limit);

    //获取一个数据库连接
    std::unique_ptr<sql::Connection> getConnection(int timeout_ms = 5000);

    //归还连接到池
    void releaseConnection(std::unique_ptr<sql::Connection> conn);

    //写

    //插入login_token
    bool s_logintoken(int64_t user_id, const std::string& token,
        const std::string& ip, const std::string& device_name,
        int64_t create_time, int64_t expire_time);

    //更新user表的token
    bool s_usertoken(int64_t user_id, const std::string& new_token);

    //插入新用户
    bool s_user(const std::string& username, const std::string& password,
        int64_t reg_time, int64_t& out_user_id);

    //插入user_verify_token
    bool s_verifytoken(int64_t user_id, const std::string& token);

    //删verify_token
    bool use_verifytoken(const std::string& token);

private:
    sql::ConnectOptionsMap opts_;
    std::queue<std::unique_ptr<sql::Connection>> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::string host_, user_, passwd_, schema_;
    int port_;
    int poolSize_;
    sql::Driver* driver_;
};

class sqlsave {
public:
    std::unordered_map<int64_t, std::unique_ptr<struct sever_list>> servers_by_id;
    PermissionCache perm_cache;
    sqlsave(const std::string& host, int port,
        const std::string& user, const std::string& password,
        const std::string& dbname);
    bool start();                         
    std::shared_ptr<ConnectionPool> getPool(); 
    void refreshServers();                 

private:
    std::shared_ptr<ConnectionPool> pool_;
    std::string host_, user_, passwd_, dbname_;
    int port_;
};

extern std::unique_ptr<sqlsave> g_db;

struct verify_token {
    int64_t id = 0;
    int64_t user_id = 0;
    std::string token;
    int is_use = 0;
    bool dirty = false;
};

struct mail {
    int64_t id = 0;
    int64_t user_id = 0;
    std::string title;
    std::string content;
    int64_t send_time = 0;
    int is_new = 1;
    std::string form_name;
    int64_t out_time = 0;
    std::string item_json;
    int is_claim = 0;
    bool dirty = false;
};

struct sever_list {
    int64_t id = 0;
    int64_t user_id = 0;
    std::string ip;
    std::string name;
    std::string introduction;
    int type = 0;
    int is_test = 0;
    int64_t last_updata = 0;
    bool dirty = false;
};

struct ban_history {
    int64_t id = 0;
    int64_t user_id = 0;
    int64_t start_ban_time = 0;
    int64_t end_ban_time = 0;
    int ban_reason = 0;
    bool dirty = false;
};
