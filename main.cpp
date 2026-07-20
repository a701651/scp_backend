#include"common.h"
#include"LOG.h"
#include "othertool.h"
#include "API_tool.h"
#include "config.h"
#include "sqlsave.h"
#include "register.h"
#include "login.h"
#include "web_get_token.h"
#include "get_token.h"
#include "getinfo.h"
#include "web_v_token.h"
#include"v_token.h"
#include "ad_login.h"
#include "ad_userlist.h"
#include "getmail.h"
#include "severlist.h"

extern "C" {
#include "llhttp.h"
}

using asio::ip::tcp;
using namespace std;
using json = nlohmann::json;
//签名
bool isdebug = true;
std::string project_name = "SCP:Completely Out of Control";
std::string project_type = "Local";
std::string project_author = "68575";
std::string project_version = "0.3.0";
std::string ls_IP = "0.0.0.0";
//全局
std::atomic<bool> running{ true };
std::unique_ptr<sqlsave> g_db;
std::atomic<int> active_threads{ 0 };
const int MAX_THREADS = 100;
std::unique_ptr<asio::thread_pool> g_business_pool;//线程池
std::unique_ptr<asio::thread_pool> g_db_pool;

//状态码映射
inline static const char* text(int code) {
    switch (code) {
    case 200: return "OK";
    case 404: return "Not Found";
    case 500: return "Internal Server Error";
    default:  return "Unknown";
    }
}
//拼接HTTP
inline string make_response(
    int code,
    const string& type,
    const string& body
) {
    /*
    * 需要：
    * HTTP/1.1 状态 值
    * Content-Type: 类型
    * Content-Length: 大小
    * Connection: close
    * 返回主体
    */
    const char* status = text(code);
    char len_buf[32];
    int len_len = snprintf(len_buf, sizeof(len_buf), "%zu", body.size());
    size_t len = 70 + strlen(status) + type.size() + len_len + body.size();
    string response;
    //+=可能致慢,为此直接字符串固定长度拼接
    response.reserve(len);
    response.append("HTTP/1.1 ");
    response.append(std::to_string(code));
    response.push_back(' ');
    response.append(status);
    response.append("\r\n");
    response.append("Content-Type: ");
    response.append(type);
    response.append("\r\n");
    response.append("Content-Length: ");
    response.append(len_buf, len_len);
    response.append("\r\n");
    response.append("Connection: close\r\n");
    response.append("\r\n");
    response.append(body);
    return response;
}

struct session : public std::enable_shared_from_this<session> {
    tcp::socket socket;
    std::array<char, 1024> buffer;
    std::string ip;

    // llhttp 相关
    llhttp_t parser;
    llhttp_settings_t settings;
    std::string tmp_header_field;
    std::map<std::string, std::string> headers;
    std::string req_url;
    std::string req_body;
    std::map<std::string, std::string> query_params;
    bool message_complete = false;

    session(tcp::socket s) : socket(std::move(s)) {
        ip = socket.remote_endpoint().address().to_string();
        init_llhttp();
    }

    void init_llhttp() {
        llhttp_settings_init(&settings);

        settings.on_url = [](llhttp_t* p, const char* at, size_t length) -> int {
            auto* self = static_cast<session*>(p->data);
            self->req_url.assign(at, length);
            return 0;
        };

        settings.on_header_field = [](llhttp_t* p, const char* at, size_t length) -> int {
            auto* self = static_cast<session*>(p->data);
            self->tmp_header_field.assign(at, length);
            return 0;
        };

        settings.on_header_value = [](llhttp_t* p, const char* at, size_t length) -> int {
            auto* self = static_cast<session*>(p->data);
            self->headers[self->tmp_header_field] = std::string(at, length);
            return 0;
        };

        settings.on_headers_complete = [](llhttp_t* p) -> int {
            auto* self = static_cast<session*>(p->data);
            self->req_body.reserve(p->content_length);
            return 0;
        };

        settings.on_body = [](llhttp_t* p, const char* at, size_t length) -> int {
            auto* self = static_cast<session*>(p->data);
            self->req_body.append(at, length);
            return 0;
        };

        settings.on_message_complete = [](llhttp_t* p) -> int {
            auto* self = static_cast<session*>(p->data);
            self->message_complete = true;
            self->parse_params();
            self->parse_post_params();
            asio::post(*g_db_pool, [self_ptr = self->shared_from_this()]() {
                self_ptr->process_request();
            });
            return 0;
        };

        llhttp_init(&parser, HTTP_REQUEST, &settings);
        parser.data = this;
    }
    void parse_params() {
        const char* q = std::strchr(req_url.c_str(), '?');
        if (!q) return;
        std::string query(q + 1);
        size_t pos = 0;
        while (pos < query.size()) {
            size_t eq = query.find('=', pos);
            size_t amp = query.find('&', pos);
            if (eq == std::string::npos) break;
            std::string key = url_decode(query.substr(pos, eq - pos));
            std::string val = url_decode(query.substr(eq + 1,
                (amp == std::string::npos ? query.size() : amp) - (eq + 1)));
            query_params[key] = val;
            if (amp == std::string::npos) break;
            pos = amp + 1;
        }
    }

    void parse_post_params() {
        auto ct = headers.find("content-type");
        if (ct == headers.end()) return;
        std::string content_type = ct->second;

        if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
            std::string body = req_body;
            size_t pos = 0;
            while (pos < body.size()) {
                size_t eq = body.find('=', pos);
                size_t amp = body.find('&', pos);
                if (eq == std::string::npos) break;
                std::string key = url_decode(body.substr(pos, eq - pos));
                std::string val = url_decode(body.substr(eq + 1,
                    (amp == std::string::npos ? body.size() : amp) - (eq + 1)));
                query_params[key] = val;
                if (amp == std::string::npos) break;
                pos = amp + 1;
            }
        }
        else if (content_type.find("application/json") != std::string::npos) {
            try {
                json j = json::parse(req_body);
                auto extract = [&](const std::string& field) {
                    if (j.contains(field) && j[field].is_string())
                        query_params[field] = j[field].get<std::string>();
                };
                extract("username");
                extract("password");
                extract("token");
                extract("device_name");
            }
            catch (const json::parse_error&) {
            }
        }

    }
    std::string build_response() {
        httpplt tmp;
        tmp.isget = (parser.method == HTTP_GET);

        std::string path = req_url.substr(0, req_url.find('?'));
        auto getp = [this](const std::string& key) -> std::string {
            auto it = query_params.find(key);
            return it != query_params.end() ? it->second : "";
        };
        tmp.username = getp("username");
        tmp.password = getp("password");
        tmp.token = getp("token");
        tmp.device_name = getp("device_name");
        tmp.path = path;
        int page = 1;
        std::string page_str = getp("page");
        if (!page_str.empty()) {
            try { page = std::stoi(page_str); }
            catch (...) { page = 1; }
        }
        //路由
        if (path == "/" && parser.method == HTTP_GET)                tmp.code = 12;
        else if (path == "/admin/login" && parser.method == HTTP_POST) tmp.code = 1;
        else if (path == "/admin/user/list" && parser.method == HTTP_POST) tmp.code = 2;
        else if (path == "/login" && parser.method == HTTP_POST)      tmp.code = 3;
        else if (path == "/register" && parser.method == HTTP_POST)   tmp.code = 4;
        else if (path == "/get_mail" && parser.method == HTTP_POST)   tmp.code = 5;
        else if (path == "/get_token" && parser.method == HTTP_POST)  tmp.code = 6;
        else if (path == "/user/my_info" && parser.method == HTTP_POST) tmp.code = 7;
        else if (path == "/user/verify_token" && parser.method == HTTP_POST) tmp.code = 8;
        else if (path == "/web/vk_user" && parser.method == HTTP_POST) tmp.code = 9;
        else if (path == "/web/get_token" && parser.method == HTTP_POST) tmp.code = 10;
        else if (path == "/web/sever/list" && parser.method == HTTP_POST) tmp.code = 11;
        else if (path == "/status" && parser.method == HTTP_GET)       tmp.code = 13;
        else if (path == "/favicon.ico" || path == "/robots.txt")      tmp.code = -1;
        else                                                            tmp.code = 0;
        if (tmp.code == -1) {
            if (isdebug) logger("已屏蔽浏览器请求");
            return "";
        }
        resjson out;
        json data;
        std::string out_token;
        int ret = 5;
        std::string device = tmp.device_name.empty() ? "未知设备" : tmp.device_name;

        // ═════ 计时起点 ═════
        auto t_start = std::chrono::steady_clock::now();
        auto ms = [&]() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t_start).count();
        };

        switch (tmp.code) {
        case 1: // 管理员登录
            ret = adlog(tmp.username, tmp.password, ip, device, out_token);
            switch (ret) {
            case 0:
                out.code = 200; out.msg = "success";
                data["token"] = out_token; out.data = data;
                logger("已处理来自ip:" + ip + "的管理员登录请求, " + std::to_string(ms()) + "ms");
                break;
            case 1:
                out.code = 400; out.msg = "用户名或密码错误"; out.data = json::array(); break;
            case 6:
                out.code = 400; out.msg = "用户名或密码错误"; out.data = json::array(); break;
            case 2:
                out.code = 400; out.msg = "用户名或密码为空"; out.data = json::array(); break;
            case 3:
                out.code = 400; out.msg = "用户名或密码长度不合法"; out.data = json::array(); break;
            case 4:
                out.code = 400; out.msg = "用户名或密码存在非法字符"; out.data = json::array(); break;
            case 5:
                out.code = 400; out.msg = "API出错"; out.data = json::array();
                error("出现登录错误！"); break;
            case 7:
                out.code = 403; out.msg = "权限不足"; out.data = json::array(); break;
            default:
                out.code = 500; out.msg = "未知错误"; out.data = json::array();
                error("log 返回未预期的状态码: " + std::to_string(ret)); break;
            }
            break;

        case 2: { //管理员查用户列表
            json list_json;
            int ret = adlist(tmp.token, page, list_json);
            switch (ret) {
            case 0:
                out.code = 200; out.msg = "success";
                out.data = list_json;
                logger("已处理来自ip:" + ip + "的admin/user/list请求, page="
                    + std::to_string(page) + ", " + std::to_string(ms()) + "ms");
                break;
            case 1:
                out.code = 400; out.msg = "token为空"; out.data = json::array(); break;
            case 2:
                out.code = 403; out.msg = "权限不足"; out.data = json::array(); break;
            case 3:
                out.code = 500; out.msg = "查询失败"; out.data = json::array(); break;
            default:
                out.code = 500; out.msg = "未知错误"; out.data = json::array();
                error("adlist 返回未预期的状态码: " + std::to_string(ret)); break;
            }
            break;
        }

        case 3: // 登录
            ret = log(tmp.username, tmp.password, ip, device, out_token);
            switch (ret) {
            case 0:
                out.code = 200; out.msg = "success";
                data["token"] = out_token; out.data = data;
                logger("已处理来自ip:" + ip + "的登录请求, " + std::to_string(ms()) + "ms");
                break;
            case 1:
                out.code = 400; out.msg = "用户名或密码错误"; out.data = json::array(); break;
            case 6:
                out.code = 400; out.msg = "用户名或密码错误"; out.data = json::array(); break;
            case 2:
                out.code = 400; out.msg = "用户名或密码为空"; out.data = json::array(); break;
            case 3:
                out.code = 400; out.msg = "用户名或密码长度不合法"; out.data = json::array(); break;
            case 4:
                out.code = 400; out.msg = "用户名或密码存在非法字符"; out.data = json::array(); break;
            case 5:
                out.code = 400; out.msg = "API出错"; out.data = json::array();
                error("出现登录错误！"); break;
            default:
                out.code = 500; out.msg = "未知错误"; out.data = json::array();
                error("log 返回未预期的状态码: " + std::to_string(ret)); break;
            }
            break;
        case 4: // 注册
            ret = reg(tmp.username, tmp.password, ip, device, out_token);
            switch (ret) {
            case 0:
                out.code = 200; out.msg = "success";
                data["token"] = out_token; out.data = data;
                logger("已成功处理来自ip:" + ip + "的注册请求, " + std::to_string(ms()) + "ms");
                break;
            case 1:
                out.code = 400; out.msg = "用户名已存在"; out.data = json::array(); break;
            case 2:
                out.code = 400; out.msg = "用户名或密码错误"; out.data = json::array(); break;
            case 3:
                out.code = 400; out.msg = "用户名或密码长度不合法"; out.data = json::array(); break;
            case 4:
                out.code = 400; out.msg = "用户名或密码存在非法字符"; out.data = json::array(); break;
            case 5:
                out.code = 400; out.msg = "API出错"; out.data = json::array();
                error("出现注册错误！"); break;
            default:
                out.code = 500; out.msg = "未知错误"; out.data = json::array();
                error("reg 返回未预期的状态码: " + std::to_string(ret)); break;
            }
            break;

        case 5: { //获取邮件
            std::string mail_data;
            int ret = maillist(tmp.token, mail_data);
            switch (ret) {
            case 0:
                out.code = 200; out.msg = "success";
                data["data"] = mail_data; out.data = data;
                logger("已成功处理来自ip:" + ip + "的邮件请求, " + std::to_string(ms()) + "ms");
                break;
            case 1:
                out.code = 400; out.msg = "token为空"; out.data = json::array(); break;
            case 2:
                out.code = 403; out.msg = "无权限"; out.data = json::array(); break;
            case 3:
                out.code = 400; out.msg = "token无效"; out.data = json::array(); break;
            case 4:
                out.code = 500; out.msg = "查询失败"; out.data = json::array(); break;
            default:
                out.code = 500; out.msg = "未知错误"; out.data = json::array(); break;
            }
            break;
        }

        case 6:  //换token
            ret = gettoken(tmp.token, out_token);
            switch (ret)
            {
            case 0:
                out.code = 200; out.msg = "success";
                data["token"] = out_token; out.data = data;
                logger("已成功处理来自ip:" + ip + "的get_token请求, " + std::to_string(ms()) + "ms");
                break;
            case 1:
                out.code = 400; out.msg = "token为空"; out.data = json::array(); break;
            case 2:
                out.code = 400; out.msg = "token无效"; out.data = json::array(); break;
            case 3:
                out.code = 400; out.msg = "token过期"; out.data = json::array(); break;
            case 4:
                out.code = 400; out.msg = "服务器内部错误"; out.data = json::array(); break;
            default:
                out.code = 500; out.msg = "未知错误"; out.data = json::array();
                error("gettoken 返回未预期的状态码: " + std::to_string(ret)); break;
            }
            break;
        case 7: //获取个人信息
            ret = getinfo(tmp.token, data);
            switch (ret)
            {
            case 0:
                out.code = 200; out.msg = "success"; out.data = data;
                logger("已成功处理来自ip:" + ip + "的getinfo请求, " + std::to_string(ms()) + "ms");
                break;
            case 1:
                out.code = 400; out.msg = "token为空"; out.data = json::array(); break;
            case 2:
                out.code = 400; out.msg = "token无效"; out.data = json::array(); break;
            case 3:
                out.code = 400; out.msg = "服务器内部错误"; out.data = json::array(); break;
            default:
                out.code = 500; out.msg = "未知错误"; out.data = json::array();
                error("getinfo 返回未预期的状态码: " + std::to_string(ret)); break;
            }
            break;

        case 8: { //生成v_token
            std::string vtoken;
            int ret = v_token(tmp.token, vtoken);
            switch (ret) {
            case 0:
                out.code = 200; out.msg = "success";
                data["token"] = vtoken; out.data = data;
                logger("已处理来自ip:" + ip + "的verify_token请求, " + std::to_string(ms()) + "ms");
                break;
            case 1:
                out.code = 400; out.msg = "token为空"; out.data = json::array(); break;
            case 2:
                out.code = 400; out.msg = "token无效"; out.data = json::array(); break;
            case 3:
                out.code = 500; out.msg = "生成失败"; out.data = json::array(); break;
            case 4:
                out.code = 403; out.msg = "无权限"; out.data = json::array(); break;
            default:
                out.code = 500; out.msg = "未知错误"; out.data = json::array();
                error("v_token 返回未预期的状态码: " + std::to_string(ret)); break;
            }
            break;
        }

        case 9://web换v_token
            ret = web_v_token(tmp.token, data);
            switch (ret)
            {
            case 0:
                out.code = 200; out.msg = "success"; out.data = data;
                logger("已成功处理来自ip:" + ip + "的web_v_token请求, " + std::to_string(ms()) + "ms");
                break;
            case 1:
                out.code = 400; out.msg = "token为空"; out.data = json::array(); break;
            case 2:
                out.code = 400; out.msg = "token无效"; out.data = json::array(); break;
            case 3:
                out.code = 403; out.msg = "token已使用"; out.data = json::array(); break;
            case 4:
                out.code = 400; out.msg = "服务器内部错误"; out.data = json::array(); break;
            case 5:
                out.code = 400; out.msg = "用户不存在"; out.data = json::array(); break;
            default:
                out.code = 500; out.msg = "未知错误"; out.data = json::array();
                error("web_v_token 返回未预期的状态码: " + std::to_string(ret)); break;
            }
            break;

        case 10://web换token
            ret = webgettoken(tmp.token, out_token);
            switch (ret)
            {
            case 0:
                out.code = 200; out.msg = "success";
                data["token"] = out_token; out.data = data;
                logger("已成功处理来自ip:" + ip + "的web/get_token请求, " + std::to_string(ms()) + "ms");
                break;
            case 1:
                out.code = 400; out.msg = "token为空"; out.data = json::array(); break;
            case 2:
                out.code = 400; out.msg = "token无效"; out.data = json::array(); break;
            case 3:
                out.code = 400; out.msg = "token过期"; out.data = json::array(); break;
            case 4:
                out.code = 400; out.msg = "服务器内部错误"; out.data = json::array(); break;
            default:
                out.code = 500; out.msg = "未知错误"; out.data = json::array();
                error("logger 返回未预期的状态码: " + std::to_string(ret)); break;
            }
            break;

        case 11: { // 服务器列表
            std::string sever_data;
            int ret = severlist(sever_data);
            if (ret == 0) {
                out.code = 200; out.msg = "success";
                data["data"] = sever_data; out.data = data;
                logger("已处理来自ip:" + ip + "的sever/list请求, " + std::to_string(ms()) + "ms");
            }
            else {
                out.code = 500; out.msg = "查询失败"; out.data = json::array();
            }
            break;
        }

        case 12: // API 状态
            out.code = 200; out.msg = "success"; out.data = json::array();
            break;

        case 13: // 服务器状态
            out = status();
            logger("已处理来自ip:" + ip + "的status请求, " + std::to_string(ms()) + "ms");
            break;

        default:
            out.code = 404; out.msg = "未知接口"; out.data = json::array();
            error("客户端发送错误的请求: code=" + std::to_string(tmp.code));
            break;
        }

        json j;
        j["code"] = out.code;
        j["msg"] = out.msg;
        j["data"] = out.data;
        std::string body = j.dump();
        return make_response(200, "application/json", body);
    }

    void process_request() {
        std::string response_str;
        try {
            response_str = build_response();
        }
        catch (const std::exception& e) {
            error("业务处理异常: " + std::string(e.what()));
            response_str = make_response(500, "application/json",
                R"({"code":500,"msg":"服务器错误","data":[]})");
        }
        catch (...) {
            response_str = make_response(500, "application/json",
                R"({"code":500,"msg":"未知错误","data":[]})");
        }
        if (response_str.empty()) return;
        auto self = shared_from_this();
        asio::post(socket.get_executor(), [self, resp = std::move(response_str)]() {
            self->do_write(std::move(resp));
        });
    }

    //启动
    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self = shared_from_this();
        socket.async_read_some(asio::buffer(buffer),
            [this, self](std::error_code ec, std::size_t length) {
            if (ec) {
                if (ec != asio::error::eof)
                    error("读取错误: " + ec.message() + " from " + ip);
                return;
            }
            auto err = llhttp_execute(&parser, buffer.data(), length);
            if (err != HPE_OK) {
                error("HTTP parse error: " + std::string(llhttp_errno_name(err)));
                return;
            }
            if (!message_complete && !parser.upgrade) {
                do_read();
            }
        });
    }

    void do_write(std::string response) {
        auto self = shared_from_this();
        asio::async_write(socket, asio::buffer(response),
            [this, self](std::error_code ec, std::size_t) {
            if (ec) error("写入错误: " + ec.message());
        });
    }
};

BOOL WINAPI console_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
        running = false;
        return TRUE;
    }
    return FALSE;
}

void start() {
    char buf[64];
    auto start_time = std::chrono::high_resolution_clock::now();
    {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &t);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        time_buf[0] = buf;
    }
    std::thread time_thread(cctime);
    time_thread.detach();
    StartCpuMonitor();
    infost();
    info(project_name + " - " + project_type + " v." + project_version);
    info("请输入help获取指令帮助.");
    info("\xC2\xA9 版权占位符,按需分配 2026-2026");
    infost();
    logger("日志系统已初始化");
    string config_path = "config.yaml";

    unsigned int cpu_threads = std::max(2u, std::thread::hardware_concurrency());
    g_business_pool = std::make_unique<asio::thread_pool>(cpu_threads);
    unsigned int db_threads = std::max(8u, cpu_threads * 4);
    g_db_pool = std::make_unique<asio::thread_pool>(db_threads);

    SetConsoleCtrlHandler(console_handler, TRUE);

    if (!file_ex(config_path)) {
        if (!creat_config(config_path)) {
            error("无法创建默认配置文件:" + config_path);
        }
        logger("已生成配置文件");
    }
    if (!load_config(config_path)) {
        error("加载配置失败!");
    }
    st();
    logger("系统配置：");
    logger("端口: " + to_string(port(GlobalConfig->server.port)));
    logger("数据库状态:" + string(GlobalConfig->database.user.empty() ? "配置失败" : "已配置"));
    logger("连接池大小: " + to_string(GlobalConfig->database.max_open_conns));
    logger("登录Token有效期: " + to_string(GlobalConfig->server.login_expire) + "秒");
    logger("邮件最大获取数量: " + to_string(GlobalConfig->server.mail_max_count));
    logger("数据库字符集: " + GlobalConfig->database.charset);
    logger("数据库连接超时: " + to_string(GlobalConfig->database.connect_timeout) + "s");
    logger("数据库读取超时: " + to_string(GlobalConfig->database.read_timeout) + "s");
    logger("数据库写入超时: " + to_string(GlobalConfig->database.write_timeout) + "s");
    logger("调试模式: " + string(isdebug ? "开启" : "关闭"));
    st();
    SetEnvironmentVariableA("MYSQL_PLUGIN_DIR",std::string(getpath().begin(), getpath().end()).c_str());
    logger("尝试连接数据库...");
    g_db = std::make_unique<sqlsave>(
        GlobalConfig->database.host,
        GlobalConfig->database.port,
        GlobalConfig->database.user,
        GlobalConfig->database.password,
        GlobalConfig->database.dbname
    );
    if (!g_db->start()) {
        error("连接数据库失败，退出.");
        std::cout << "按任意键退出..." << std::endl;
        std::cin.get();
        exit(1);
    }
    run("连接数据库成功!");
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    logger("启动完成，耗时 " + std::to_string(duration) + " 毫秒");
}
int main() {
    try {
        //控制台编码 & DLL 路径 
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        SetDllDirectoryW(getpath().c_str());
        start();

        asio::io_context io_context;
        tcp::acceptor acceptor(io_context,
            tcp::endpoint(asio::ip::make_address(ls_IP), port(GlobalConfig->server.port)));
        logger("监听IP: " + ls_IP);
        logger("正在监听" + std::to_string(port(GlobalConfig->server.port)) + "端口");
        std::function<void()> do_a;
        do_a = [&]() {
            acceptor.async_accept(
                [&](std::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<session>(std::move(socket))->start();
                }
                else if (ec != asio::error::operation_aborted) {
                    error("接受连接错误: " + ec.message());
                }
                if (running) do_a();
            }
            );
        };
        do_a();
        unsigned int thread_count = std::thread::hardware_concurrency() * 2;
        if (thread_count == 0) thread_count = 4;
        std::vector<std::thread> io_threads;
        for (unsigned int i = 0; i < thread_count; ++i) {
            io_threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }

        //等待退出信号
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        logger("服务器正在关闭...");
        acceptor.close();
        io_context.stop();
        for (auto& t : io_threads) {
            if (t.joinable()) t.join();
        }
        logger("服务器已关闭");
    }
    catch (const std::exception& e) {
        error("主函数异常: " + std::string(e.what()));
        return 1;
    }
    return 0;
}