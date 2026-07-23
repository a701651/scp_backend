#include"get_token.h"
#include"sqlsave.h"
#include "othertool.h"
using namespace std;

/*
* 状态码:
* 0: 成功
* 1: token为空
* 2: token无效
* 3: token过期
* 5: 插入失败
*/
int gettoken(const std::string& token, std::string& out) {
    if (token.empty()) return 1;

    auto pool = g_db->getPool();
    constexpr int kMaxRetries = 3;
    constexpr int kBackoffMs[] = { 50, 100, 200 };

    for (int attempt = 0; attempt <= kMaxRetries; ++attempt) {
        std::unique_ptr<sql::Connection> conn;
        try {
            conn = pool->getConnection();   
            auto* c = conn.get();
            std::unique_ptr<sql::Statement> stmt(c->createStatement());
            std::unique_ptr<sql::ResultSet> res(
                stmt->executeQuery(
                    "SELECT user_id, expire_time FROM login_token WHERE token = '"
                    + token + "'"));

            if (!res->next()) {
                pool->releaseConnection(std::move(conn));
                return 2;              
            }
            int64_t user_id = res->getInt64("user_id");
            if (res->getInt64("expire_time") < std::time(nullptr)) {
                pool->releaseConnection(std::move(conn));
                return 3;                 
            }
            res.reset();
            std::string new_token = TokenCache::Get();
            stmt->executeUpdate(
                "UPDATE user SET token = '" + new_token
                + "' WHERE id = " + std::to_string(user_id));

            pool->releaseConnection(std::move(conn));
            out = std::move(new_token);
            return 0;
        }
        catch (sql::SQLException& e) {
            if (conn) pool->releaseConnection(std::move(conn));

            int errCode = e.getErrorCode();
            bool isConnErr = (errCode == 2002 || errCode == 2003 ||
                errCode == 2006 || errCode == 2013);
            if (isConnErr && attempt < kMaxRetries) {
                std::this_thread::sleep_for(std::chrono::milliseconds(kBackoffMs[attempt]));
                continue;
            }
            return 5;
        }
        catch (...) {
            if (conn) pool->releaseConnection(std::move(conn));
            if (attempt < kMaxRetries) {
                std::this_thread::sleep_for(std::chrono::milliseconds(kBackoffMs[attempt]));
                continue;
            }
            return 5;
        }
    }
    return 5;
}
