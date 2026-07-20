#include "sqlsave.h"
#include "config.h"

void PermissionCache::loadAll(std::shared_ptr<ConnectionPool> pool) {
    auto conn = pool->getConnection();

    std::unique_ptr<sql::Statement> stmt(conn->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT * FROM permission"));

    sql::ResultSetMetaData* meta = res->getMetaData();
    unsigned int colCount = meta->getColumnCount();

    std::vector<std::string> colNames;
    colNames.reserve(colCount);
    for (unsigned int i = 1; i <= colCount; ++i) {
        colNames.push_back(meta->getColumnName(i));
    }

    std::unique_lock lock(mtx_);
    cache_.clear();

    while (res->next()) {
        int64_t id = res->getInt64("id");
        PermissionMap permMap;

        for (unsigned int i = 1; i <= colCount; ++i) {
            const std::string& colName = colNames[i - 1];
            if (colName == "id" || colName == "name") continue;
            permMap[colName] = res->getInt(i);  
        }
        cache_[id] = std::move(permMap);
    }

    pool->releaseConnection(std::move(conn));
}

const PermissionMap* PermissionCache::get(int64_t permission_id) const {
    std::shared_lock lock(mtx_);             
    auto it = cache_.find(permission_id);
    return (it != cache_.end()) ? &it->second : nullptr;
}

void PermissionCache::refresh(std::shared_ptr<ConnectionPool> pool) {
    loadAll(pool);                            
}

ConnectionPool::ConnectionPool(const std::string& host, int port,
    const std::string& user, const std::string& passwd,
    const std::string& schema, int poolSize)
    : host_(host), port_(port), user_(user), passwd_(passwd), schema_(schema),
    poolSize_(poolSize), driver_(nullptr) {
    driver_ = sql::mysql::get_driver_instance();
    for (int i = 0; i < poolSize_; ++i) {
        auto conn = driver_->connect("tcp://" + host_ + ":" + std::to_string(port_), user_, passwd_);
        conn->setSchema(schema_);
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        stmt->execute("SET NAMES utf8");
        pool_.emplace(std::move(conn));
    }
}

ConnectionPool::~ConnectionPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        pool_.pop();
    }
}

std::unique_ptr<sql::Connection> ConnectionPool::getConnection(int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (pool_.empty()) {
        if (!cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return !pool_.empty(); }))
            throw std::runtime_error("获取数据库连接超时");
    }
    auto conn = std::move(pool_.front());
    pool_.pop();
    try {
        if (!conn->isValid()) {
            conn.reset(driver_->connect("tcp://" + host_ + ":" + std::to_string(port_), user_, passwd_));
            conn->setSchema(schema_);
            std::unique_ptr<sql::Statement> stmt(conn->createStatement());
            stmt->execute("SET NAMES utf8");
        }
    }
    catch (...) {
        conn.reset(driver_->connect("tcp://" + host_ + ":" + std::to_string(port_), user_, passwd_));
        conn->setSchema(schema_);
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        stmt->execute("SET NAMES utf8");
    }
    return conn;
}

void ConnectionPool::releaseConnection(std::unique_ptr<sql::Connection> conn) {
    if (!conn) return;
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push(std::move(conn));
    cv_.notify_one();
}

namespace {
    std::string safeGetString(sql::ResultSet* res, const std::string& col) {
        std::string val = res->getString(col);
        return res->wasNull() ? "" : val;
    }
    int64_t safeGetInt64(sql::ResultSet* res, const std::string& col) {
        int64_t val = res->getInt64(col);
        return res->wasNull() ? 0 : val;
    }
    int safeGetInt(sql::ResultSet* res, const std::string& col) {
        int val = res->getInt(col);
        return res->wasNull() ? 0 : val;
    }

    user mapUser(sql::ResultSet* res) {
        user u;
        u.id = res->getInt64("id");
        u.nickname = safeGetString(res, "nickname");
        u.is_official = (res->getInt("is_official") != 0);
        u.avatar = safeGetString(res, "avatar");
        u.username = res->getString("username");
        u.password = res->getString("password");
        u.permission_id = res->getInt("permission_id");
        u.token = safeGetString(res, "token");
        u.api_key = safeGetString(res, "api_key");
        u.email = safeGetString(res, "email");
        u.sign = safeGetString(res, "sign");
        u.user_group = res->getInt("user_group");
        u.login_ip = safeGetString(res, "login_ip");
        u.registration_time = safeGetInt64(res, "registration_time");
        u.last_time = safeGetInt64(res, "last_time");
        u.ban_reason = safeGetString(res, "ban_reason");
        u.start_ban_time = safeGetInt64(res, "start_ban_time");
        u.end_ban_time = safeGetInt64(res, "end_ban_time");
        return u;
    }

    struct ConnGuard {
        ConnectionPool* pool;
        std::unique_ptr<sql::Connection> conn;
        ConnGuard(ConnectionPool* p, std::unique_ptr<sql::Connection> c) : pool(p), conn(std::move(c)) {}
        ~ConnGuard() { if (pool && conn) pool->releaseConnection(std::move(conn)); }
        sql::Connection* operator->() { return conn.get(); }
    };
}

std::optional<user> ConnectionPool::token_user(const std::string& token) {
    ConnGuard guard(this, getConnection());
    auto conn = guard.conn.get();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SELECT * FROM user WHERE token = ?")
    );
    pstmt->setString(1, token);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
        return mapUser(res.get());
    }
    return std::nullopt;
}

std::optional<user> ConnectionPool::nickname_user(const std::string& username) {
    ConnGuard guard(this, getConnection());
    auto conn = guard.conn.get();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SELECT * FROM user WHERE username = ?")
    );
    pstmt->setString(1, username);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
        return mapUser(res.get());
    }
    return std::nullopt;
}

std::optional<user> ConnectionPool::id_user(int64_t user_id) {
    ConnGuard guard(this, getConnection());
    auto conn = guard.conn.get();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SELECT * FROM user WHERE id = ?")
    );
    pstmt->setInt64(1, user_id);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
        return mapUser(res.get());
    }
    return std::nullopt;
}

std::optional<login_token> ConnectionPool::logintoken(const std::string& token) {
    ConnGuard guard(this, getConnection());
    auto conn = guard.conn.get();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SELECT * FROM login_token WHERE token = ?")
    );
    pstmt->setString(1, token);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
        login_token t;
        t.id = res->getInt64("id");
        t.user_id = res->getInt64("user_id");
        t.token = res->getString("token");
        t.ip = safeGetString(res.get(), "ip");
        t.create_time = safeGetInt64(res.get(), "create_time");
        t.device_name = safeGetString(res.get(), "device_name");
        t.expire_time = safeGetInt64(res.get(), "expire_time");
        return t;
    }
    return std::nullopt;
}

std::optional<verify_token> ConnectionPool::vetoken(const std::string& token) {
    ConnGuard guard(this, getConnection());
    auto conn = guard.conn.get();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SELECT * FROM user_verify_token WHERE token = ?")
    );
    pstmt->setString(1, token);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
        verify_token v;
        v.id = res->getInt64("id");
        v.user_id = res->getInt64("user_id");
        v.token = res->getString("token");
        v.is_use = res->getInt("is_use");
        return v;
    }
    return std::nullopt;
}

std::vector<mail> ConnectionPool::id_user(int64_t user_id, int limit) {
    std::vector<mail> mails;
    ConnGuard guard(this, getConnection());
    auto conn = guard.conn.get();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement(
            "SELECT * FROM mail "
            "WHERE user_id = ? AND out_time >= UNIX_TIMESTAMP() "
            "ORDER BY id DESC LIMIT ?"
        )
    );
    pstmt->setInt64(1, user_id);
    pstmt->setInt(2, limit);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
        mail m;
        m.id = res->getInt64("id");
        m.user_id = res->getInt64("user_id");
        m.title = safeGetString(res.get(), "title");
        m.content = safeGetString(res.get(), "content");
        m.send_time = safeGetInt64(res.get(), "send_time");
        m.is_new = res->getInt("is_new");
        m.form_name = safeGetString(res.get(), "form_name");
        m.out_time = safeGetInt64(res.get(), "out_time");
        m.item_json = safeGetString(res.get(), "item");
        m.is_claim = res->getInt("is_claim");
        mails.push_back(std::move(m));
    }
    return mails;
}

std::optional<user> ConnectionPool::token_id(const std::string& token) {
    return token_user(token);
}

std::optional<user> ConnectionPool::queryUserByUsername(const std::string& username) {
    return nickname_user(username);
}

std::vector<user> ConnectionPool::get_user_list(int offset, int limit) {
    std::vector<user> users;
    ConnGuard guard(this, getConnection());
    auto conn = guard.conn.get();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement(
            "SELECT id, nickname, username, avatar, sign, is_official, "
            "permission_id, user_group, registration_time, last_time, "
            "ban_reason, start_ban_time, end_ban_time "
            "FROM user ORDER BY id ASC LIMIT ? OFFSET ?"
        )
    );
    pstmt->setInt(1, limit);
    pstmt->setInt(2, offset);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
        user u;
        u.id = res->getInt64("id");
        u.nickname = safeGetString(res.get(), "nickname");
        u.username = res->getString("username");
        u.avatar = safeGetString(res.get(), "avatar");
        u.sign = safeGetString(res.get(), "sign");
        u.is_official = (res->getInt("is_official") != 0);
        u.permission_id = res->getInt("permission_id");
        u.user_group = res->getInt("user_group");
        u.registration_time = safeGetInt64(res.get(), "registration_time");
        u.last_time = safeGetInt64(res.get(), "last_time");
        u.ban_reason = safeGetString(res.get(), "ban_reason");
        u.start_ban_time = safeGetInt64(res.get(), "start_ban_time");
        u.end_ban_time = safeGetInt64(res.get(), "end_ban_time");
        users.push_back(std::move(u));
    }
    return users;
}

sqlsave::sqlsave(const std::string& host, int port,
    const std::string& user, const std::string& password,
    const std::string& dbname)
    : host_(host), port_(port), user_(user), passwd_(password), dbname_(dbname) {
}

bool sqlsave::start() {
    try {
        int pool_size = GlobalConfig->database.max_open_conns;
        pool_ = std::make_shared<ConnectionPool>(host_, port_, user_, passwd_, dbname_, pool_size);
        refreshServers();
        perm_cache.loadAll(pool_);
        return true;
    }
    catch (std::exception& e) {
        return false;
    }
}

void sqlsave::refreshServers() {
    auto conn = pool_->getConnection();
    std::unique_ptr<sql::Statement> stmt(conn->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
        "SELECT id, user_id, ip, name, introduction, type, is_test, last_updata FROM sever_list"
    ));
    servers_by_id.clear();
    while (res->next()) {
        auto s = std::make_unique<sever_list>();
        s->id = res->getInt64("id");
        s->user_id = res->getInt64("user_id");
        s->ip = res->getString("ip");
        s->name = res->getString("name");
        s->introduction = res->getString("introduction");
        s->type = res->getInt("type");
        s->is_test = res->getInt("is_test");
        s->last_updata = res->getInt64("last_updata");
        servers_by_id[s->id] = std::move(s);
    }
    pool_->releaseConnection(std::move(conn));
}

std::shared_ptr<ConnectionPool> sqlsave::getPool() {
    return pool_;
}

bool ConnectionPool::s_logintoken(int64_t user_id, const std::string& token,
    const std::string& ip, const std::string& device_name,
    int64_t create_time, int64_t expire_time) {
    try {
        ConnGuard guard(this, getConnection());
        auto* conn = guard.conn.get();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement(
                "INSERT INTO login_token (user_id, token, create_time, ip, device_name, expire_time) "
                "VALUES (?, ?, ?, ?, ?, ?)"
            )
        );
        pstmt->setInt64(1, user_id);
        pstmt->setString(2, token);
        pstmt->setInt64(3, create_time);
        pstmt->setString(4, ip);
        pstmt->setString(5, device_name);
        pstmt->setInt64(6, expire_time);
        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "[s_logintoken] SQL错误: " << e.what()
            << " | code=" << e.getErrorCode()
            << " | SQL状态=" << e.getSQLState() << std::endl;
        return false;
    }
    catch (std::exception& e) {                          // ← 加这个
        std::cerr << "[s_logintoken] 其他异常: " << e.what() << std::endl;
        return false;
    }
}


bool ConnectionPool::s_usertoken(int64_t user_id, const std::string& new_token) {
    try {
        ConnGuard guard(this, getConnection());
        auto* conn = guard.conn.get();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("UPDATE user SET token = ? WHERE id = ?")
        );
        pstmt->setString(1, new_token);
        pstmt->setInt64(2, user_id);
        int affected = pstmt->executeUpdate();
        return (affected > 0);
    }
    catch (sql::SQLException& e) {
        return false;
    }
}

bool ConnectionPool::s_user(const std::string& username, const std::string& password,
    int64_t reg_time, int64_t& out_user_id) {
    try {
        ConnGuard guard(this, getConnection());
        auto* conn = guard.conn.get();

        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("INSERT INTO user (username, password, registration_time) VALUES (?, ?, ?)")
        );
        pstmt->setString(1, username);
        pstmt->setString(2, password);
        pstmt->setInt64(3, reg_time);
        pstmt->executeUpdate();

        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID()"));
        if (res->next()) {
            out_user_id = res->getInt64(1);
            return true;
        }
        return false;
    }
    catch (sql::SQLException& e) {
        std::cerr << "[s_user] MySQL错误: " << e.what() << " (code " << e.getErrorCode() << ")" << std::endl;
        return false;
    }
}

bool ConnectionPool::s_verifytoken(int64_t user_id, const std::string& token) {
    try {
        ConnGuard guard(this, getConnection());
        auto* conn = guard.conn.get();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement(
                "INSERT INTO user_verify_token (user_id, token) VALUES (?, ?)"
            )
        );
        pstmt->setInt64(1, user_id);
        pstmt->setString(2, token);
        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException& e) {
        return false;
    }
}

bool ConnectionPool::use_verifytoken(const std::string& token) {
    try {
        ConnGuard guard(this, getConnection());
        auto* conn = guard.conn.get();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement(
                "UPDATE user_verify_token SET is_use = 1 WHERE token = ? AND is_use = 0"
            )
        );
        pstmt->setString(1, token);
        int affected = pstmt->executeUpdate();
        return (affected > 0);
    }
    catch (sql::SQLException& e) {
        return false;
    }
}
