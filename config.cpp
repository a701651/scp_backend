#include "config.h"
#include"common.h"
Config* GlobalConfig = nullptr;
//检查文件是否存在
bool file_ex(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}
bool creat_config(const std::string& path) {
    std::ofstream out(path);

    YAML::Emitter emitter;
    emitter << YAML::BeginMap;
    //sever
    emitter << YAML::Key << "server" << YAML::Value << YAML::BeginMap;
    emitter << YAML::Key << "port" << YAML::Value << ":3000";
    emitter << YAML::Key << "login_expire" << YAML::Value << 604800;
    emitter << YAML::Key << "mail_max_count" << YAML::Value << 50;
    emitter << YAML::EndMap;
    //data
    emitter << YAML::Key << "database" << YAML::Value << YAML::BeginMap;
    emitter << YAML::Key << "host" << YAML::Value << "127.0.0.1";
    emitter << YAML::Key << "port" << YAML::Value << 3306;
    emitter << YAML::Key << "user" << YAML::Value << "root";
    emitter << YAML::Key << "password" << YAML::Value << "your_password";
    emitter << YAML::Key << "dbname" << YAML::Value << "game";
    emitter << YAML::Key << "charset" << YAML::Value << "utf8mb4";
    emitter << YAML::Key << "max_open_conns" << YAML::Value << 300;
    emitter << YAML::Key << "max_idle_conns" << YAML::Value << 10;
    emitter << YAML::Key << "conn_max_lifetime" << YAML::Value << 600;
    emitter << YAML::Key << "conn_max_idle_time" << YAML::Value << 300;
    emitter << YAML::Key << "connect_timeout" << YAML::Value << 10;
    emitter << YAML::Key << "read_timeout" << YAML::Value << 30;
    emitter << YAML::Key << "write_timeout" << YAML::Value << 30;
    emitter << YAML::EndMap;
    emitter << YAML::EndMap;
    out << emitter.c_str();
    out.close();
    return true;
}

bool load_config(const std::string& path) {
    
    try {
        YAML::Node root = YAML::LoadFile(path);
        static Config instance;
        Config& cfg = instance;
        if (auto server = root["server"]) {
            if (server["port"])
                cfg.server.port = server["port"].as<std::string>();
            if (server["login_expire"])
                cfg.server.login_expire = server["login_expire"].as<int>();
            if (server["mail_max_count"])
                cfg.server.mail_max_count = server["mail_max_count"].as<int>();
        }
        if (auto db = root["database"]) {
            if (db["host"])     cfg.database.host = db["host"].as<std::string>();
            if (db["port"])     cfg.database.port = db["port"].as<int>();
            if (db["user"])     cfg.database.user = db["user"].as<std::string>();
            if (db["password"]) cfg.database.password = db["password"].as<std::string>();
            if (db["dbname"])   cfg.database.dbname = db["dbname"].as<std::string>();
            if (db["charset"])  cfg.database.charset = db["charset"].as<std::string>();
            if (db["max_open_conns"])    cfg.database.max_open_conns = db["max_open_conns"].as<int>();
            if (db["max_idle_conns"])    cfg.database.max_idle_conns = db["max_idle_conns"].as<int>();
            if (db["conn_max_lifetime"]) cfg.database.conn_max_lifetime = db["conn_max_lifetime"].as<int>();
            if (db["conn_max_idle_time"])cfg.database.conn_max_idle_time = db["conn_max_idle_time"].as<int>();
            if (db["connect_timeout"])   cfg.database.connect_timeout = db["connect_timeout"].as<int>();
            if (db["read_timeout"])      cfg.database.read_timeout = db["read_timeout"].as<int>();
            if (db["write_timeout"])     cfg.database.write_timeout = db["write_timeout"].as<int>();
        }
        GlobalConfig = &cfg;
        return true;
    }
    catch (const YAML::Exception& e) {
        return false;
    }
}

int port(const std::string& poth) {
    if (poth.empty()) return 3000;
    std::string s = poth;
    if (s[0] == ':') s.erase(0, 1);
    try {
        return std::stoi(s);
    }
    catch (...) {
        return 3000;
    }
}
