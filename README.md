# 🛡 Windows 高危端口检测与管理工具

<div align="center">

![Version](https://img.shields.io/badge/版本-v3.0-00D4AF?style=for-the-badge)
![Platform](https://img.shields.io/badge/平台-Windows-0078D4?style=for-the-badge&logo=windows)
![Language](https://img.shields.io/badge/语言-C++17-00599C?style=for-the-badge&logo=cplusplus)
![License](https://img.shields.io/badge/许可-MIT-green?style=for-the-badge)

**实时扫描本机监听端口，自动识别高危服务，支持一键关闭端口或添加防火墙规则。**

</div>

---

## 📋 目录

- [功能介绍](#-功能介绍)
- [界面预览](#-界面预览)
- [项目结构](#-项目结构)
- [编译方法](#-编译方法)
- [使用说明](#-使用说明)
- [高危端口数据库](#-高危端口数据库)
- [风险等级说明](#-风险等级说明)
- [常见问题](#-常见问题)
- [扩展开发](#-扩展开发)

---

## ✨ 功能介绍

| 功能 | 说明 |
|------|------|
| 🔍 **端口扫描** | 自动扫描本机所有 TCP / UDP 监听端口，异步执行不阻塞界面 |
| ⚠️ **风险识别** | 对照内置高危端口数据库（60+ 条目），自动标注严重 / 高危 / 中危 |
| 📋 **详情查看** | 点击任意端口，底部面板显示完整危险描述与修复建议 |
| ⛔ **关闭端口** | 终止占用端口的进程（需管理员权限），含关键系统进程保护 |
| 🛡 **防火墙拦截** | 一键添加 Windows 防火墙入站阻断规则（自动 UAC 提权） |
| 🎨 **深色主题** | 深色终端风格 UI，青绿强调色，风险行彩色高亮 |
| 🔄 **筛选显示** | 支持「仅高危端口」和「显示全部」两种视图切换 |

---

## 🖥 界面预览

```
┌─────────────────────────────────────────────────────────────┐
│ 🛡  高危端口检测与管理工具          建议以管理员权限运行      │  ← Banner
│     Windows Port Security Scanner  v3.0                      │
├─────────────────────────────────────────────────────────────┤
│  [🔍 重新扫描]  [⛔ 关闭端口]  [🛡 添加防火墙]  ● 仅高危  ○ 全部  │  ← 工具栏
├──────┬──────┬──────────┬──────────┬──────┬──────┬──────────┤
│ 端口 │ 协议 │  状态    │  服务名  │ 风险 │ PID  │  进程名  │
├──────┼──────┼──────────┼──────────┼──────┼──────┼──────────┤
│  445 │ TCP  │ LISTEN   │   SMB    │ 严重 │  4   │  System  │  ← 红色行
│ 3389 │ TCP  │ LISTEN   │   RDP    │ 严重 │ 1234 │ svchost  │  ← 红色行
│  135 │ TCP  │ LISTEN   │   RPC    │ 严重 │  892 │ svchost  │  ← 红色行
│  139 │ TCP  │ LISTEN   │ NetBIOS  │ 严重 │  4   │  System  │  ← 红色行
│  137 │ UDP  │  OPEN    │NetBIOS NS│ 高危 │  4   │  System  │  ← 橙色行
├─────────────────────────────────────────────────────────────┤
│  端口 : 3389    协议 : TCP    状态 : LISTEN                  │
│  PID  : 1234    进程 : svchost.exe                           │  ← 详情面板
│  ─────────────────────────────────────────                   │
│  服务    : RDP                                               │
│  风险等级: 严重                                              │
│  【危险描述】                                                │
│  远程桌面协议（RDP）是勒索软件最常用的入侵入口...            │
├─────────────────────────────────────────────────────────────┤
│  扫描完成 | 共 42 个监听端口 | 高危: 8 个 | 严重: 5 个      │  ← 状态栏
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 项目结构

```
PortScanner/
│
├── main.cpp            # 程序入口、UAC 提权、消息循环
│
├── resource.h          # 全局常量（控件ID、配色、字体、布局尺寸）
│                       # ★ 修改 UI 样式只需编辑此文件
│
├── data.h              # 数据结构定义（PortInfo、ActivePort、RiskLevel）
├── data.cpp            # 高危端口数据库（60+ 条目）+ 查询函数
│                       # ★ 新增端口条目只需编辑此文件
│
├── scanner.h           # 端口扫描模块接口
├── scanner.cpp         # 扫描实现：工作线程、PID映射、TCP/UDP枚举、去重
│
├── actions.h           # 业务操作接口
├── actions.cpp         # 终止进程、防火墙规则、系统进程保护
│
├── ui_theme.h          # 主题绘制接口
├── ui_theme.cpp        # 深色主题：Banner、按钮、背景、ListView着色
│
├── ui_window.h         # 主窗口接口
├── ui_window.cpp       # 窗口过程、控件创建、布局、详情面板
│
└── Makefile            # MinGW 构建脚本
```

---

## 🔨 编译方法

### 环境要求

| 工具 | 版本要求 | 下载 |
|------|---------|------|
| MinGW-w64 | GCC 10.0+ | [MSYS2](https://www.msys2.org/) 或 [WinLibs](https://winlibs.com/) |
| Windows SDK | 任意版本 | 随 MinGW 附带 |

> **推荐使用 MSYS2 安装 MinGW-w64：**
> ```bash
> pacman -S mingw-w64-x86_64-gcc
> ```

---

### 方式一：命令行直接编译（推荐）

打开 PowerShell 或 CMD，进入项目目录：

```bash
# GUI 版本（无控制台窗口，正式使用）
g++ -g main.cpp data.cpp scanner.cpp actions.cpp ui_theme.cpp ui_window.cpp ^
    -o PortScanner.exe ^
    -std=c++17 -mwindows -municode ^
    -lws2_32 -liphlpapi -lgdi32 -lcomctl32 -lshell32 -lole32

# 调试版本（带控制台输出，开发调试用）
g++ -g main.cpp data.cpp scanner.cpp actions.cpp ui_theme.cpp ui_window.cpp ^
    -o PortScanner_debug.exe ^
    -std=c++17 -municode ^
    -lws2_32 -liphlpapi -lgdi32 -lcomctl32 -lshell32 -lole32
```

---

### 方式二：使用 Makefile

```bash
# 构建 GUI 版（默认）
mingw32-make

# 构建调试版
mingw32-make console

# 清理编译产物
mingw32-make clean
```

---

### 方式三：VS Code 配置

在项目根目录创建 `.vscode/tasks.json`：

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build PortScanner",
            "type": "shell",
            "command": "g++",
            "args": [
                "-g",
                "${workspaceFolder}/main.cpp",
                "${workspaceFolder}/data.cpp",
                "${workspaceFolder}/scanner.cpp",
                "${workspaceFolder}/actions.cpp",
                "${workspaceFolder}/ui_theme.cpp",
                "${workspaceFolder}/ui_window.cpp",
                "-o", "${workspaceFolder}/PortScanner.exe",
                "-std=c++17",
                "-mwindows",
                "-municode",
                "-lws2_32", "-liphlpapi", "-lgdi32",
                "-lcomctl32", "-lshell32", "-lole32"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": "$gcc"
        }
    ]
}
```

按 `Ctrl+Shift+B` 即可构建。

---

### 编译参数说明

| 参数 | 作用 |
|------|------|
| `-std=c++17` | 启用 C++17 标准（lambda、结构化绑定等） |
| `-mwindows` | GUI 模式，隐藏控制台窗口 |
| `-municode` | 启用 Unicode 入口点 `wWinMain` |
| `-lws2_32` | Winsock2 网络库 |
| `-liphlpapi` | IP Helper API（端口枚举） |
| `-lgdi32` | GDI 绘图（自绘 UI） |
| `-lcomctl32` | 公共控件（ListView、StatusBar） |
| `-lshell32` | Shell API（UAC 提权、ShellExecute） |
| `-lole32` | COM 初始化 |

---

## 📖 使用说明

### 启动程序

双击 `PortScanner.exe` 运行。首次启动会弹出权限提示：

```
部分功能（终止进程、添加防火墙规则）需要管理员权限。
是否立即以管理员身份重新启动？
```

- 选 **是** → 以管理员权限重启，所有功能可用
- 选 **否** → 受限模式运行，仅可查看端口信息

> **建议始终以管理员权限运行**，否则关闭端口和添加防火墙规则功能无效。

---

### 扫描端口

程序启动后自动执行一次扫描。如需重新扫描：

1. 点击顶部工具栏的 **「🔍 重新扫描」** 按钮
2. 扫描在后台线程执行，期间界面保持响应
3. 扫描完成后状态栏显示统计信息

---

### 查看端口详情

单击列表中任意一行，底部详情面板将显示：

- 端口号、协议、状态、PID、进程名
- 所属服务名称
- 风险等级
- **危险描述**：该端口的具体安全风险
- **修复建议**：如何消除或降低风险

---

### 关闭端口（终止进程）

1. 在列表中选中目标端口
2. 点击 **「⛔ 关闭端口（终止进程）」**
3. 确认对话框中核实进程名和 PID
4. 点击「是」执行终止

> ⚠️ **注意事项：**
> - 强制终止进程可能导致未保存的数据丢失
> - 系统关键进程（`lsass.exe`、`csrss.exe`、`System` 等）受保护，无法终止
> - 终止后点击「重新扫描」刷新列表

---

### 添加防火墙规则

1. 在列表中选中目标端口
2. 点击 **「🛡 添加防火墙拦截」**
3. 系统弹出 UAC 提权确认框，点击「是」
4. 规则添加成功后提示确认

> **防火墙规则说明：**
> - 规则名格式：`PortScanner_Block_TCP_3389`
> - 仅拦截**新的入站连接**，不影响已建立的连接
> - 不会终止正在监听的进程，进程仍会运行
> - 可在 Windows 防火墙高级设置中查看和删除规则

---

### 视图切换

工具栏右侧的单选按钮控制显示范围：

| 选项 | 说明 |
|------|------|
| **仅高危端口** | 只显示命中高危数据库的端口（默认） |
| **显示全部** | 显示本机所有监听端口 |

---

## 🗄 高危端口数据库

当前数据库覆盖以下类别，共 **60+ 条目**：

### 远程访问 / 传输类
| 端口 | 服务 | 风险 |
|------|------|------|
| 21 | FTP | 🔴 严重 |
| 22 | SSH | 🟠 高危 |
| 23 | Telnet | 🔴 严重 |
| 3389 | RDP | 🔴 严重 |
| 5900 | VNC | 🔴 严重 |
| 512/513/514 | rexec/rlogin/rsh | 🔴 严重 |

### 数据库类
| 端口 | 服务 | 风险 |
|------|------|------|
| 1433 | MS SQL Server | 🔴 严重 |
| 1521 | Oracle DB | 🔴 严重 |
| 3306 | MySQL | 🔴 严重 |
| 5432 | PostgreSQL | 🔴 严重 |
| 6379/6380 | Redis | 🔴 严重 |
| 9200 | Elasticsearch | 🔴 严重 |
| 27017 | MongoDB | 🔴 严重 |
| 5984 | CouchDB | 🔴 严重 |
| 11211 | Memcached | 🔴 严重 |

### 容器 / 云原生类
| 端口 | 服务 | 风险 |
|------|------|------|
| 2375 | Docker API | 🔴 严重 |
| 2376 | Docker API (TLS) | 🟠 高危 |
| 6443 | Kubernetes API | 🔴 严重 |
| 10250 | Kubelet API | 🔴 严重 |

### 工控 / IoT 类
| 端口 | 服务 | 风险 |
|------|------|------|
| 102 | Siemens S7 | 🔴 严重 |
| 502 | Modbus | 🔴 严重 |
| 1883 | MQTT | 🟠 高危 |
| 47808 | BACnet | 🟠 高危 |

---

## 🎨 风险等级说明

| 等级 | 颜色 | 含义 |
|------|------|------|
| 🔴 **严重** | 红色行 | 存在已知 RCE 漏洞或默认无认证，可直接导致系统被控制或数据完全泄露 |
| 🟠 **高危** | 橙色行 | 存在重大安全缺陷，配置不当或使用弱密码可导致严重安全事件 |
| 🔵 **中危** | 蓝色行 | 存在一定安全风险，需要合理配置才能安全使用 |

---

## ❓ 常见问题

**Q：扫描结果为空或端口很少？**
> 以管理员权限运行后重新扫描，部分端口信息需要提升权限才能读取。

**Q：点击「关闭端口」提示"操作被阻止"？**
> 该端口由系统核心进程（如 PID 4 的 System）占用，无法通过终止进程关闭。
> 请改用「添加防火墙拦截」在网络层屏蔽该端口。

**Q：添加防火墙规则后端口还在列表里？**
> 正常现象。防火墙规则只阻断新的入站连接，进程仍在监听。
> 重新扫描后端口依然存在，但外部连接已被拦截。

**Q：如何删除已添加的防火墙规则？**
> 打开「Windows Defender 防火墙」→「高级设置」→「入站规则」，
> 找到名称以 `PortScanner_Block_` 开头的规则，右键删除即可。

**Q：编译时出现 `winsock2.h` 警告？**
> 在所有 `.cpp` 文件的最顶部（第一行）添加：
> ```cpp
> #include <winsock2.h>
> ```
> 确保 winsock2.h 在 windows.h 之前被引入。

**Q：窗口启动后一直闪烁跳动？**
> 检查 `ui_window.cpp` 的 `Window_Create` 函数，确保：
> 1. `CreateWindowExW` 的第一个参数为 `0`，不是 `WS_EX_COMPOSITED`
> 2. `Theme_OnEraseBkgnd` 使用了内存 DC 双缓冲绘制

---

## 🔧 扩展开发

### 新增高危端口条目

编辑 `data.cpp`，在 `DANGEROUS_PORTS` 数组中添加：

```cpp
{端口号, L"TCP/UDP", L"服务名", 风险等级,
 L"危险描述（支持 \\r\\n 换行）",
 L"修复建议"},
```

示例：
```cpp
{8888, L"TCP", L"Jupyter Notebook", RISK_CRITICAL,
 L"Jupyter Notebook 默认无认证，任何人可执行任意 Python 代码。\r\n"
 L"可直接读写文件系统，相当于提供了完整的服务器访问权限。",
 L"设置 token 认证，绑定 127.0.0.1，禁止公网访问。"},
```

### 修改 UI 配色

编辑 `resource.h` 中的颜色常量：

```cpp
#define C_ACCENT        RGB(  0, 212, 175)   // 主强调色，改成你想要的颜色
#define C_BG_APP        RGB( 15,  19,  27)   // 主背景色
```

### 修改字体大小

编辑 `resource.h`：

```cpp
#define FONT_SIZE_UI        19    // 调大或调小
#define FONT_SIZE_BANNER    22
#define FONT_SIZE_DETAIL    16
```

---

## 📄 许可证

MIT License — 自由使用、修改和分发，保留原始版权声明即可。

---

<div align="center">
<sub>如有问题或建议，欢迎提交 Issue 或 PR</sub>
</div>
