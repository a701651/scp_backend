# SCP:COC_MS_backend

**语言**：C++20  
**数据库**：MySQL  
**依赖**：zlib、libmysqlclient / mysql-connector-c++、LLHTTP  

---

## 1. 项目结构

```
项目根目录/
├── 头文件/
│   ├── API/
│   │   ├── admin/
│   │   │   ├── ad_login.h         # POST /admin/login
│   │   │   └── ad_userlist.h      # POST /admin/user/list
│   │   ├── user/
│   │   │   ├── getinfo.h          # POST /user/my_info
│   │   │   ├── getmail.h          # POST /get_mail
│   │   │   └── v_token.h          # POST /user/verify_token
│   │   └── web/
│   │       ├── get_token.h        # POST /get_token
│   │       ├── login.h            # POST /login
│   │       ├── register.h         # POST /register
│   │       ├── serverlist.h       # POST /web/sever/list
│   │       ├── web_get_token.h    # POST /web/get_token
│   │       └── web_v_token.h      # POST /web/vk_user
│   ├── API_tool.h                 # 分发路由器 & 通用响应结构
│   ├── common.h                   # 预编译头（共用 include）
│   ├── gzpi.h                     # Base64 + Gzip 压缩/解压
│   ├── LOG.h                      # 日志系统
│   ├── othertool.h                # 工具函数（token生成、MD5等）
│   └── sqlsave.h                  # MySQL 连接池 & 查询封装
└── 源文件/
    ├── API/
    │   ├── admin/
    │   │   ├── ad_login.cpp
    │   │   └── ad_userlist.cpp
    │   ├── user/
    │   │   ├── getinfo.cpp
    │   │   ├── getmail.cpp
    │   │   └── v_token.cpp
    │   └── web/
    │       ├── get_token.cpp
    │       ├── login.cpp
    │       ├── register.cpp
    │       ├── severlist.cpp
    │       ├── web_get_token.cpp
    │       └── web_v_token.cpp
    ├── config.cpp                  # JSON配置文件解析
    ├── main.cpp                    # 入口 & HTTP监听循环
    ├── othertool.cpp
    └── sqlsave.cpp                 # MySQL连接池实现
```

---

## 2. 充分说明

```
鉴权：① login_token → ② user.token → ③ verify_token → 游戏服拿到 user_id
权限表是缓存在内存，用hash维护
mysql大部分使用直接查询
采用多线程异步处理，其中业务、网路、数据层都是分不同线程的，避免堵塞
```

---

## 3. 数据库

| 表名 | 引擎 | 说明 |
|------|------|------|
| `user` | MyISAM | 用户账号 & 角色（token/api_key在此） |
| `login_token` | MyISAM | 登录会话，每次login/register生成一条 |
| `user_verify_token` | MyISAM | 一次性验证令牌，`is_use` 防重放 |
| `permission` | MyISAM | RBAC权限组 |
| `mail` | MyISAM | 游戏邮件，`item`为JSON物品 |
| `sever_list` | MyISAM | 服务器列表 |
| `ban_history` | MyISAM | 封禁历史记录 |

---

## 4. 构建 & 运行

- 构建：使用 **MSVC** 编译即可  
- 运行：终端在同目录下执行  
  ```bash
  ./coc-ms.exe