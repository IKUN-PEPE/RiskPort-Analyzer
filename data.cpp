// ═══════════════════════════════════════════════════════════
//  data.cpp  —  高危端口数据库 + 查询函数实现
// ═══════════════════════════════════════════════════════════

#include "data.h"
#include <algorithm>

const std::vector<PortInfo> DANGEROUS_PORTS = {

    // ── 远程访问 / 传输类 ─────────────────────────────────
    {21,    L"TCP", L"FTP",              RISK_CRITICAL,
     L"FTP 使用明文传输，账号密码可被中间人直接嗅探。\r\n"
     L"匿名 FTP 登录可能导致敏感文件泄露，常被用于后门植入。",
     L"建议改用 SFTP/FTPS，或直接禁用 FTP 服务。"},

    {22,    L"TCP", L"SSH",              RISK_HIGH,
     L"SSH 暴露公网后常遭暴力破解和字典攻击。\r\n"
     L"若使用弱密码或默认配置，可被远程完全控制。",
     L"限制来源 IP，禁用密码登录改用密钥，修改默认端口。"},

    {23,    L"TCP", L"Telnet",           RISK_CRITICAL,
     L"Telnet 完全明文传输，包括密码，已被 SSH 彻底淘汰。\r\n"
     L"开启此端口极易造成服务器被完全控制。",
     L"立即禁用 Telnet 服务，改用 SSH 进行远程管理。"},

    {25,    L"TCP", L"SMTP",             RISK_HIGH,
     L"SMTP 开放中继允许攻击者以你的服务器发送垃圾邮件。\r\n"
     L"未经认证的 SMTP 还可用于钓鱼和恶意邮件投递。",
     L"禁用开放中继，要求 SMTP 认证，限制访问来源。"},

    {53,    L"UDP", L"DNS",              RISK_MEDIUM,
     L"DNS 服务可被用于 DNS 放大 DDoS 攻击（流量放大 70x）。\r\n"
     L"DNS 缓存污染可将用户重定向至恶意站点。",
     L"限制递归查询，仅允许授权客户端访问 DNS 服务。"},

    {80,    L"TCP", L"HTTP",             RISK_MEDIUM,
     L"HTTP 明文传输，所有数据可被中间人截获和篡改。\r\n"
     L"常见 Web 漏洞（SQL 注入、XSS）均通过此端口利用。",
     L"升级到 HTTPS（443），部署 WAF，定期更新 Web 应用。"},

    {102,   L"TCP", L"Siemens S7",       RISK_CRITICAL,
     L"西门子 S7 工控协议端口，用于与 PLC 直接通信。\r\n"
     L"震网（Stuxnet）病毒曾利用此协议攻击工业设施，无认证保护。",
     L"物理隔离工控网络，部署工控 IDS，禁止任何公网访问。"},

    {135,   L"TCP", L"RPC Endpoint",     RISK_CRITICAL,
     L"Windows RPC 端点映射器，是 WannaCry、Blaster 等蠕虫的\r\n"
     L"主要攻击入口。大量已知 RCE 漏洞通过此端口利用。",
     L"在防火墙屏蔽此端口，仅限内网访问，及时打补丁。"},

    {137,   L"UDP", L"NetBIOS NS",       RISK_HIGH,
     L"NetBIOS 名称服务可泄露主机名、工作组、用户名等信息。\r\n"
     L"可被利用进行 NBNS 欺骗攻击，截获 NTLM 哈希。",
     L"禁用 NetBIOS over TCP/IP，改用 DNS 进行名称解析。"},

    {138,   L"UDP", L"NetBIOS DGM",      RISK_HIGH,
     L"NetBIOS 数据报服务，可被利用进行网络广播攻击。\r\n"
     L"配合 137 端口可完整枚举局域网中的 Windows 主机。",
     L"在防火墙关闭此端口，禁用 NetBIOS 相关服务。"},

    {139,   L"TCP", L"NetBIOS SSN",      RISK_CRITICAL,
     L"SMB over NetBIOS，EternalBlue（MS17-010）漏洞利用路径之一。\r\n"
     L"WannaCry 勒索软件正是通过 139/445 端口蔓延全球。",
     L"关闭此端口，安装 MS17-010 补丁，禁用 SMBv1。"},

    {161,   L"UDP", L"SNMP",             RISK_HIGH,
     L"SNMP v1/v2 使用明文 community 字符串（默认 public/private）。\r\n"
     L"攻击者可读取路由器/交换机配置，甚至修改网络设备参数。",
     L"升级至 SNMPv3（带认证和加密），修改默认 community 字符串。"},

    {389,   L"TCP", L"LDAP",             RISK_HIGH,
     L"未加密的 LDAP 传输 Active Directory 认证信息。\r\n"
     L"可被用于凭据中继攻击（LDAP Relay）获取域管权限。",
     L"改用 LDAPS（636），启用 LDAP 签名和通道绑定。"},

    {443,   L"TCP", L"HTTPS",            RISK_MEDIUM,
     L"若 SSL/TLS 配置不当（弱算法、自签证书、过期证书）\r\n"
     L"则存在降级攻击（POODLE、BEAST）和中间人风险。",
     L"使用 TLS 1.2+，禁用 SSLv3/TLS 1.0，定期更新证书。"},

    {445,   L"TCP", L"SMB",              RISK_CRITICAL,
     L"SMBv1 存在 EternalBlue (MS17-010) 等严重 RCE 漏洞。\r\n"
     L"WannaCry、NotPetya 等勒索软件通过此端口大规模传播，\r\n"
     L"可在几秒内感染未打补丁的主机，造成全盘加密。",
     L"禁用 SMBv1，安装所有安全补丁，防火墙屏蔽公网访问。"},

    {502,   L"TCP", L"Modbus",           RISK_CRITICAL,
     L"工业控制系统协议，完全无认证无加密。\r\n"
     L"攻击者可直接读写 PLC 寄存器，操控工厂设备或基础设施。",
     L"仅限工控内网使用，严禁暴露公网，部署工控专用防火墙。"},

    {512,   L"TCP", L"rexec",            RISK_CRITICAL,
     L"远古 Unix 远程执行服务，明文传输用户名和密码。\r\n"
     L"现代网络中几乎不应存在此服务，出现即视为严重安全隐患。",
     L"立即禁用，改用 SSH 进行所有远程管理操作。"},

    {513,   L"TCP", L"rlogin",           RISK_CRITICAL,
     L"Unix 远程登录服务，依赖 .rhosts 文件做信任认证，极易被伪造。\r\n"
     L"明文传输所有数据，已被完全淘汰。",
     L"立即禁用，改用 SSH 密钥认证。"},

    {514,   L"TCP", L"rsh",              RISK_CRITICAL,
     L"Unix 远程 Shell 服务，无需密码仅凭主机名信任即可执行命令。\r\n"
     L"信任关系极易被 IP 欺骗攻击利用，实现无认证远程控制。",
     L"立即禁用，防火墙屏蔽，改用 SSH。"},

    // ── 数据库类 ──────────────────────────────────────────
    {1433,  L"TCP", L"MS SQL Server",    RISK_CRITICAL,
     L"SQL Server 默认端口，常遭暴力破解和 SQL 注入攻击。\r\n"
     L"sa 账号弱密码可导致数据库完全泄露，甚至 xp_cmdshell 提权。",
     L"修改默认端口，禁用 sa 账号，限制访问来源 IP。"},

    {1521,  L"TCP", L"Oracle DB",        RISK_CRITICAL,
     L"Oracle 监听器默认无认证，攻击者可枚举 SID、暴力破解账号。\r\n"
     L"TNS Poison 漏洞可导致中间人攻击劫持数据库连接。",
     L"升级至最新版本，设置监听器密码，限制访问来源 IP。"},

    {1723,  L"TCP", L"PPTP VPN",         RISK_HIGH,
     L"PPTP 使用已破解的 MS-CHAPv2 认证，可被实时破解。\r\n"
     L"密码强度无论多复杂都无法抵御现代 PPTP 攻击工具。",
     L"迁移至 IKEv2/OpenVPN/WireGuard 等现代 VPN 协议。"},

    {1883,  L"TCP", L"MQTT",             RISK_HIGH,
     L"IoT 设备常用消息协议，默认无认证。\r\n"
     L"攻击者可订阅所有主题获取传感器数据，或发布恶意指令控制设备。",
     L"启用用户名密码认证，改用 MQTTS（8883），限制订阅权限。"},

    {2375,  L"TCP", L"Docker API",       RISK_CRITICAL,
     L"Docker 未加密远程 API，无需认证即可完全控制宿主机。\r\n"
     L"攻击者可创建特权容器挂载宿主机根目录，实现完全逃逸。",
     L"绝对禁止公网暴露，改用 TLS 加密的 2376 端口，严格限制访问。"},

    {2376,  L"TCP", L"Docker API (TLS)", RISK_HIGH,
     L"Docker TLS 远程 API，若证书配置不当仍存在未授权访问风险。\r\n"
     L"客户端证书泄露可导致攻击者完全控制 Docker 宿主机。",
     L"严格管理客户端证书，限制访问来源，定期轮换证书。"},

    {3306,  L"TCP", L"MySQL",            RISK_CRITICAL,
     L"MySQL 暴露公网后常遭暴力破解和数据库转储攻击。\r\n"
     L"root 账号弱密码可导致所有数据库完全泄露。",
     L"绑定到 127.0.0.1，不向公网开放，使用强密码。"},

    {3389,  L"TCP", L"RDP",              RISK_CRITICAL,
     L"远程桌面协议（RDP）是勒索软件最常用的入侵入口。\r\n"
     L"BlueKeep (CVE-2019-0708) 可无需认证实现远程代码执行。\r\n"
     L"暴力破解工具每秒可尝试数百个密码组合。",
     L"限制来源 IP，启用 NLA 认证，修改默认端口，使用 VPN。"},

    {4444,  L"TCP", L"Metasploit Shell", RISK_CRITICAL,
     L"Metasploit 框架默认反弹 Shell 监听端口。\r\n"
     L"此端口开放极可能意味着系统已被入侵并植入后门。",
     L"立即检查系统是否被入侵，终止相关进程，全面安全审计。"},

    {4848,  L"TCP", L"GlassFish Admin",  RISK_CRITICAL,
     L"GlassFish 应用服务器管理控制台，默认凭据广为人知。\r\n"
     L"可通过管理界面部署恶意 WAR 包，实现远程代码执行。",
     L"修改默认密码，绑定内网地址，防火墙屏蔽公网访问。"},

    {5353,  L"UDP", L"mDNS",             RISK_MEDIUM,
     L"mDNS 广播可暴露主机名、服务列表等信息。\r\n"
     L"为局域网内的攻击者提供基础设施侦察便利。",
     L"在不需要设备发现的场景下禁用 mDNS 服务。"},

    {5432,  L"TCP", L"PostgreSQL",       RISK_CRITICAL,
     L"PostgreSQL 暴露公网后常遭暴力破解。\r\n"
     L"默认 postgres 超级用户若使用弱密码，可执行任意 SQL 并读写文件系统。",
     L"绑定 127.0.0.1，限制 pg_hba.conf 访问来源，禁用不必要的超级用户。"},

    {5900,  L"TCP", L"VNC",              RISK_CRITICAL,
     L"VNC 默认无加密，密码可被嗅探。\r\n"
     L"大量 VNC 服务使用弱密码甚至无密码，可直接远程控制桌面。",
     L"禁止公网访问，使用 SSH 隧道，设置强密码并启用加密。"},

    {5984,  L"TCP", L"CouchDB",          RISK_CRITICAL,
     L"CouchDB 默认开启 HTTP API 且无需认证，任何人可读写所有数据库。\r\n"
     L"历史上曾有大量 CouchDB 实例因此被勒索删库。",
     L"绑定 127.0.0.1，启用 CouchDB 认证，关闭 admin party 模式。"},

    {5985,  L"TCP", L"WinRM HTTP",       RISK_HIGH,
     L"Windows 远程管理（WinRM）HTTP 端口，凭据以明文方式传输。\r\n"
     L"攻击者可通过 Evil-WinRM 等工具远程执行 PowerShell 命令。",
     L"改用 HTTPS（5986），限制来源 IP，禁用不必要的 WinRM 服务。"},

    {5986,  L"TCP", L"WinRM HTTPS",      RISK_HIGH,
     L"WinRM 的 HTTPS 端口，配置不当（如自签证书）仍存在中间人风险。\r\n"
     L"弱密码账号可被暴力破解后用于远程控制主机。",
     L"限制来源 IP 白名单，强制使用强密码和多因素认证。"},

    {6000,  L"TCP", L"X11",              RISK_HIGH,
     L"X Window System 网络端口，允许远程访问图形桌面会话。\r\n"
     L"未加密传输，攻击者可捕获键盘输入、截取屏幕、注入鼠标事件。",
     L"禁止公网暴露，通过 SSH X11 转发替代直接 X11 网络连接。"},

    {6379,  L"TCP", L"Redis",            RISK_CRITICAL,
     L"Redis 默认无认证，可被任意读写数据，写入 crontab 反弹 Shell。\r\n"
     L"已有大量实战案例通过未授权 Redis 实现服务器完全控制。",
     L"绑定 127.0.0.1，设置 requirepass，不向公网暴露。"},

    {6380,  L"TCP", L"Redis (Alt)",      RISK_CRITICAL,
     L"Redis 常见备用端口，与 6379 存在相同的未授权访问风险。\r\n"
     L"部分运维人员更换端口后误以为安全，实则防护同样薄弱。",
     L"绑定 127.0.0.1，设置 requirepass，防火墙屏蔽公网访问。"},

    {6443,  L"TCP", L"Kubernetes API",   RISK_CRITICAL,
     L"Kubernetes API Server，集群控制平面核心入口。\r\n"
     L"若 RBAC 配置不当或存在未授权访问，攻击者可接管整个集群。",
     L"启用 RBAC，限制来源 IP，禁用匿名访问，使用强认证机制。"},

    {7474,  L"TCP", L"Neo4j HTTP",       RISK_HIGH,
     L"Neo4j 图数据库 HTTP 接口，默认凭据（neo4j/neo4j）广为人知。\r\n"
     L"未修改默认密码的实例可被直接登录，导致图数据完全泄露。",
     L"立即修改默认密码，绑定内网地址，防火墙限制访问来源。"},

    {8080,  L"TCP", L"HTTP Proxy/Alt",   RISK_MEDIUM,
     L"常用替代 HTTP 端口，易被误配置为开放代理。\r\n"
     L"开放代理可被攻击者用于隐藏身份、发起攻击或访问内网。",
     L"关闭不必要的代理服务，检查 Web 应用安全配置。"},

    {8500,  L"TCP", L"Consul HTTP",      RISK_HIGH,
     L"HashiCorp Consul 服务发现 API，默认无认证。\r\n"
     L"攻击者可获取所有服务注册信息，篡改健康检查，执行任意脚本。",
     L"启用 ACL 认证，绑定内网地址，防火墙限制访问来源。"},

    {8883,  L"TCP", L"MQTTS",            RISK_MEDIUM,
     L"MQTT over TLS 端口，若证书配置不当仍存在中间人风险。\r\n"
     L"弱认证凭据可被暴力破解，导致 IoT 设备被恶意控制。",
     L"使用受信任 CA 签发证书，强制双向 TLS 认证，定期轮换密钥。"},

    {9090,  L"TCP", L"Prometheus",       RISK_HIGH,
     L"Prometheus 监控系统默认无认证，暴露所有监控指标数据。\r\n"
     L"泄露的指标可揭示系统架构、服务版本及潜在攻击面。",
     L"启用 Basic Auth 或反向代理认证，限制访问来源 IP。"},

    {9200,  L"TCP", L"Elasticsearch",    RISK_CRITICAL,
     L"Elasticsearch 默认无认证，历史上数亿条数据因此泄露。\r\n"
     L"攻击者可直接读取、删除所有索引数据，并写入恶意脚本。",
     L"启用 X-Pack 安全认证，绑定内网地址，防火墙屏蔽公网。"},

    {10250, L"TCP", L"Kubelet API",      RISK_CRITICAL,
     L"Kubernetes 节点 Kubelet API，可在节点上执行任意容器命令。\r\n"
     L"未授权访问可直接在所有 Pod 中执行命令，实现横向渗透。",
     L"启用 Kubelet 认证授权，限制 API Server 以外的访问来源。"},

    {11211, L"TCP", L"Memcached",        RISK_CRITICAL,
     L"Memcached 默认无认证，历史上曾被用于 Tbps 级 UDP 放大 DDoS 攻击。\r\n"
     L"攻击者可读取缓存中的所有数据，包括会话令牌和敏感信息。",
     L"绑定 127.0.0.1，禁用 UDP 端口，防火墙屏蔽公网访问。"},

    {11434, L"TCP", L"Ollama API",       RISK_HIGH,
     L"Ollama 本地 AI 模型服务，默认监听所有网络接口且无任何认证。\r\n"
     L"攻击者可通过此端口免费调用你的 GPU 资源运行模型，\r\n"
     L"或读取本地已下载的所有模型文件及对话历史。",
     L"绑定 127.0.0.1（OLLAMA_HOST=127.0.0.1），\r\n"
     L"或通过反向代理添加认证后再对外暴露。"},

    {18789, L"TCP", L"OpenClaw Gateway", RISK_HIGH,
     L"OpenClaw AI 助手的 WebSocket 控制平面，管理所有会话、\r\n"
     L"渠道（WhatsApp/Telegram/Discord 等）、工具调用和事件路由。\r\n"
     L"若绑定到 0.0.0.0 而非 127.0.0.1，任何人均可访问控制界面，\r\n"
     L"进而操控文件系统、执行 Shell 命令、读取所有对话记录和 API 密钥。",
     L"确保 gateway.bind 设为 loopback（127.0.0.1），\r\n"
     L"远程访问请通过 SSH 隧道或 Tailscale VPN，切勿直接暴露公网。"},

    {18791, L"TCP", L"OpenClaw Dashboard", RISK_HIGH,
     L"OpenClaw 浏览器控制台和 Canvas 访问端口。\r\n"
     L"Canvas 内容为不可信 HTML/JS，若对外暴露可被注入恶意脚本，\r\n"
     L"并可通过浏览器控制接口横向渗透至宿主机环境。",
     L"仅限本地访问，不得对 LAN 或公网开放，\r\n"
     L"需远程访问时使用 Tailscale Serve 而非直接端口转发。"},

    {27017, L"TCP", L"MongoDB",          RISK_CRITICAL,
     L"MongoDB 默认无认证，历史上已有数十万数据库被公开删除勒索。\r\n"
     L"攻击者可直接读取、删除、篡改所有数据库内容。",
     L"绑定 127.0.0.1，启用认证，防火墙屏蔽公网访问。"},

    {47808, L"UDP", L"BACnet",           RISK_HIGH,
     L"楼宇自动化控制协议，广泛用于空调、电梯、门禁等系统。\r\n"
     L"无内置认证机制，暴露公网可被攻击者操控楼宇设备。",
     L"隔离于专用工控网络，部署访问控制列表，禁止公网访问。"},
};

// ───────────────────────────────────────────────────────────
//  查询函数
// ───────────────────────────────────────────────────────────
const PortInfo* FindPortInfo(int port, const std::wstring& proto) {
    for (const auto& p : DANGEROUS_PORTS)
        if (p.port == port && p.proto == proto)
            return &p;
    return nullptr;
}

std::wstring RiskLevelStr(RiskLevel r) {
    switch (r) {
        case RISK_CRITICAL: return L"严重";
        case RISK_HIGH:     return L"高危";
        case RISK_MEDIUM:   return L"中危";
    }
    return L"未知";
}