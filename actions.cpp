// ═══════════════════════════════════════════════════════════
//  actions.cpp  —  端口操作业务逻辑
//
//  安全要点：
//    · proto 白名单校验，防止命令注入
//    · 关键系统进程黑名单，防止蓝屏
//    · 所有格式化字符串均用 swprintf_s
// ═══════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include "actions.h"
#include <shellapi.h>
#include <shlobj.h>
#include <vector>
#include <string>

#pragma comment(lib, "shell32.lib")

static std::wstring MakeFirewallRuleName(const ActivePort& ap) {
    wchar_t ruleName[64];
    swprintf_s(ruleName, L"PortScanner_Block_%s_%d", ap.proto.c_str(), ap.port);
    return ruleName;
}

static bool RunCommandCapture(const std::wstring& command, std::wstring& output) {
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr, writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) return false;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = writePipe;
    si.hStdError = writePipe;

    PROCESS_INFORMATION pi = {};
    std::wstring cmdline = L"cmd.exe /c " + command;
    BOOL ok = CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(writePipe);
    if (!ok) {
        CloseHandle(readPipe);
        return false;
    }

    std::string bytes;
    char buffer[4096];
    DWORD read = 0;
    while (ReadFile(readPipe, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        bytes.append(buffer, buffer + read);
    }

    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(readPipe);

    if (bytes.size() >= 2 && (unsigned char)bytes[0] == 0xFF && (unsigned char)bytes[1] == 0xFE) {
        output.assign(reinterpret_cast<const wchar_t*>(bytes.data() + 2),
                      (bytes.size() - 2) / sizeof(wchar_t));
    } else {
        int len = MultiByteToWideChar(CP_OEMCP, 0, bytes.c_str(), (int)bytes.size(), nullptr, 0);
        if (len <= 0) return false;
        output.resize(len);
        MultiByteToWideChar(CP_OEMCP, 0, bytes.c_str(), (int)bytes.size(), output.data(), len);
    }
    return true;
}

static bool ContainsInsensitive(const std::wstring& text, const std::wstring& needle) {
    if (needle.empty()) return true;
    std::wstring hay = text;
    std::wstring ndl = needle;
    CharUpperBuffW(hay.data(), (DWORD)hay.size());
    CharUpperBuffW(ndl.data(), (DWORD)ndl.size());
    return hay.find(ndl) != std::wstring::npos;
}

// ───────────────────────────────────────────────────────────
//  受保护进程黑名单（终止这些进程会导致蓝屏或系统崩溃）
// ───────────────────────────────────────────────────────────
static const wchar_t* const PROTECTED_NAMES[] = {
    L"lsass.exe",    // 本地安全认证
    L"csrss.exe",    // 客户端/服务器运行时
    L"smss.exe",     // 会话管理器
    L"wininit.exe",  // Windows 初始化
    L"winlogon.exe", // 登录进程
    L"services.exe", // 服务控制管理器
    L"System",
    L"System Idle",
    nullptr
};

bool IsProtectedProcess(DWORD pid, const std::wstring& processName) {
    if (pid == 0 || pid == 4) return true;
    for (int i = 0; PROTECTED_NAMES[i]; ++i)
        if (_wcsicmp(processName.c_str(), PROTECTED_NAMES[i]) == 0)
            return true;
    return false;
}

FirewallGlobalState QueryFirewallGlobalState() {
    std::wstring output;
    if (!RunCommandCapture(L"netsh advfirewall show allprofiles state", output))
        return FW_GLOBAL_UNKNOWN;
    if (ContainsInsensitive(output, L"OFF") || ContainsInsensitive(output, L"关闭"))
        return FW_GLOBAL_OFF;
    if (ContainsInsensitive(output, L"ON") || ContainsInsensitive(output, L"开启"))
        return FW_GLOBAL_ON;
    return FW_GLOBAL_UNKNOWN;
}

std::wstring FirewallGlobalStateStr(FirewallGlobalState s) {
    switch (s) {
    case FW_GLOBAL_ON:  return L"已开启";
    case FW_GLOBAL_OFF: return L"已关闭";
    default:            return L"未知";
    }
}

FirewallBlockState QueryFirewallBlockState(const ActivePort& ap) {
    if (ap.proto != L"TCP" && ap.proto != L"UDP") return FW_BLOCK_UNKNOWN;

    std::wstring output;
    std::wstring ruleName = MakeFirewallRuleName(ap);
    wchar_t cmd[256];
    swprintf_s(cmd,
        L"netsh advfirewall firewall show rule name=\"%s\" verbose",
        ruleName.c_str());

    if (!RunCommandCapture(cmd, output)) return FW_BLOCK_UNKNOWN;
    if (!ContainsInsensitive(output, ruleName)) return FW_BLOCK_NO;

    std::wstring enabledYes = L"Yes";
    std::wstring enabledCn = L"是";
    if ((ContainsInsensitive(output, L"Enabled") || ContainsInsensitive(output, L"已启用")) &&
        !(ContainsInsensitive(output, enabledYes) || ContainsInsensitive(output, enabledCn))) {
        return FW_BLOCK_NO;
    }

    return FW_BLOCK_YES;
}

std::wstring FirewallBlockStateStr(FirewallBlockState s) {
    switch (s) {
    case FW_BLOCK_YES: return L"🛡 已封堵";
    case FW_BLOCK_NO:  return L"⚠ 未封堵";
    default:           return L"? 未知";
    }
}

// ───────────────────────────────────────────────────────────
//  终止进程
// ───────────────────────────────────────────────────────────
bool KillProcessById(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h) return false;
    bool ok = (TerminateProcess(h, 0) != FALSE);
    CloseHandle(h);
    return ok;
}

// ───────────────────────────────────────────────────────────
//  关闭端口（终止占用进程）
// ───────────────────────────────────────────────────────────
void ActionClosePort(HWND hParent, const ActivePort& ap) {
    // 1. 保护系统关键进程
    if (IsProtectedProcess(ap.pid, ap.processName)) {
        MessageBoxW(hParent,
            L"此端口由受保护的系统核心进程占用，\n"
            L"强制终止该进程会导致系统崩溃（蓝屏）。\n\n"
            L"请改用「添加防火墙拦截」在网络层屏蔽该端口。",
            L"操作被阻止", MB_ICONWARNING | MB_OK);
        return;
    }

    // 2. 确认对话框
    wchar_t msg[512];
    swprintf_s(msg,
        L"确认要关闭端口 %d（%s）吗？\n\n"
        L"将强制终止进程：%s（PID %lu）\n\n"
        L"⚠  强制结束进程可能导致未保存的数据丢失！",
        ap.port, ap.proto.c_str(), ap.processName.c_str(), ap.pid);

    if (MessageBoxW(hParent, msg, L"确认关闭端口",
                    MB_YESNO | MB_ICONWARNING) != IDYES)
        return;

    // 3. 权限检查
    if (!IsUserAnAdmin()) {
        MessageBoxW(hParent,
            L"终止进程需要管理员权限。\n"
            L"请右键本程序 → 以管理员身份运行。",
            L"权限不足", MB_ICONERROR | MB_OK);
        return;
    }

    // 4. 执行
    if (KillProcessById(ap.pid)) {
        MessageBoxW(hParent,
            L"进程已终止，端口已关闭。\n\n"
            L"点击「重新扫描」刷新端口列表。",
            L"操作成功", MB_ICONINFORMATION | MB_OK);
    } else {
        wchar_t err[256];
        swprintf_s(err,
            L"终止进程失败（系统错误码：%lu）。\n\n"
            L"可能原因：\n"
            L"  · 程序未以管理员权限运行\n"
            L"  · 进程由 SYSTEM 账户拥有，需更高权限",
            GetLastError());
        MessageBoxW(hParent, err, L"操作失败", MB_ICONERROR | MB_OK);
    }
}

// ───────────────────────────────────────────────────────────
//  添加 Windows 防火墙入站阻断规则
// ───────────────────────────────────────────────────────────
bool ActionAddFirewallBlock(HWND hParent, const ActivePort& ap) {
    // proto 白名单校验，防止命令注入
    if (ap.proto != L"TCP" && ap.proto != L"UDP") {
        MessageBoxW(hParent, L"不支持的协议类型，操作取消。",
                    L"错误", MB_ICONERROR | MB_OK);
        return false;
    }

    if (QueryFirewallBlockState(ap) == FW_BLOCK_YES) {
        MessageBoxW(hParent,
            L"已检测到本工具创建的防火墙拦截规则，无需重复添加。",
            L"规则已存在", MB_ICONINFORMATION | MB_OK);
        return false;
    }

    std::wstring ruleName = MakeFirewallRuleName(ap);

    wchar_t cmd[320];
    swprintf_s(cmd,
        L"netsh advfirewall firewall add rule "
        L"name=\"%s\" dir=in action=block protocol=%s localport=%d",
        ruleName.c_str(), ap.proto.c_str(), ap.port);

    wchar_t params[384];
    swprintf_s(params, L"/c %s", cmd);

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb       = L"runas";          // 请求 UAC 提权
    sei.lpFile       = L"cmd.exe";
    sei.lpParameters = params;
    sei.nShow        = SW_HIDE;

    if (ShellExecuteExW(&sei) && sei.hProcess) {
        WaitForSingleObject(sei.hProcess, 8000);
        CloseHandle(sei.hProcess);
        MessageBoxW(hParent,
            L"防火墙入站阻断规则已添加成功。\n\n"
            L"注意：规则仅拦截新入站连接，\n"
            L"不会中断已建立的连接，也不会终止监听进程。",
            L"防火墙规则已添加", MB_ICONINFORMATION | MB_OK);
        return true;
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            // 用户取消了 UAC 提权，静默忽略
            return false;
        }
        wchar_t errMsg[200];
        swprintf_s(errMsg,
            L"添加防火墙规则失败（错误码：%lu）。\n"
            L"请以管理员身份运行本程序后重试。", err);
        MessageBoxW(hParent, errMsg, L"操作失败", MB_ICONERROR | MB_OK);
        return false;
    }
}

bool ActionRemoveFirewallBlock(HWND hParent, const ActivePort& ap) {
    if (ap.proto != L"TCP" && ap.proto != L"UDP") {
        MessageBoxW(hParent, L"不支持的协议类型，操作取消。",
                    L"错误", MB_ICONERROR | MB_OK);
        return false;
    }

    if (QueryFirewallBlockState(ap) != FW_BLOCK_YES) {
        MessageBoxW(hParent,
            L"当前没有检测到本工具创建的防火墙拦截规则。",
            L"无需去除", MB_ICONINFORMATION | MB_OK);
        return false;
    }

    wchar_t msg[512];
    swprintf_s(msg,
        L"确认要去除端口 %d（%s）的防火墙拦截规则吗？\n\n"
        L"这将删除本工具创建的对应入站阻断规则。",
        ap.port, ap.proto.c_str());

    if (MessageBoxW(hParent, msg, L"确认去除拦截",
                    MB_YESNO | MB_ICONWARNING) != IDYES)
        return false;

    std::wstring ruleName = MakeFirewallRuleName(ap);
    wchar_t cmd[320];
    swprintf_s(cmd,
        L"netsh advfirewall firewall delete rule name=\"%s\"",
        ruleName.c_str());

    wchar_t params[384];
    swprintf_s(params, L"/c %s", cmd);

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = params;
    sei.nShow = SW_HIDE;

    if (ShellExecuteExW(&sei) && sei.hProcess) {
        WaitForSingleObject(sei.hProcess, 8000);
        CloseHandle(sei.hProcess);
        MessageBoxW(hParent,
            L"防火墙拦截规则已删除。\n\n"
            L"点击「重新扫描」可刷新端口状态。",
            L"操作成功", MB_ICONINFORMATION | MB_OK);
        return true;
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) return false;
        wchar_t errMsg[200];
        swprintf_s(errMsg,
            L"删除防火墙规则失败（错误码：%lu）。\n"
            L"请以管理员身份运行本程序后重试。", err);
        MessageBoxW(hParent, errMsg, L"操作失败", MB_ICONERROR | MB_OK);
        return false;
    }
}

bool ActionEnableFirewall(HWND hParent) {
    FirewallGlobalState state = QueryFirewallGlobalState();
    if (state == FW_GLOBAL_ON) {
        MessageBoxW(hParent,
            L"检测到 Windows 防火墙已开启，无需重复打开。",
            L"提示", MB_ICONINFORMATION | MB_OK);
        return false;
    }

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = L"/c netsh advfirewall set allprofiles state on";
    sei.nShow = SW_HIDE;

    if (ShellExecuteExW(&sei) && sei.hProcess) {
        WaitForSingleObject(sei.hProcess, 8000);
        CloseHandle(sei.hProcess);
        MessageBoxW(hParent,
            L"Windows 防火墙已尝试开启成功。\n\n"
            L"建议你重新扫描端口，并根据需要继续添加拦截规则。",
            L"操作成功", MB_ICONINFORMATION | MB_OK);
        return true;
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) return false;
        wchar_t errMsg[200];
        swprintf_s(errMsg,
            L"开启 Windows 防火墙失败（错误码：%lu）。\n"
            L"请尝试以管理员身份运行本程序。", err);
        MessageBoxW(hParent, errMsg, L"操作失败", MB_ICONERROR | MB_OK);
        return false;
    }
}

bool ActionDisableFirewall(HWND hParent) {
    FirewallGlobalState state = QueryFirewallGlobalState();
    if (state == FW_GLOBAL_OFF) {
        MessageBoxW(hParent,
            L"检测到 Windows 防火墙已关闭，无需重复关闭。",
            L"提示", MB_ICONINFORMATION | MB_OK);
        return false;
    }

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = L"/c netsh advfirewall set allprofiles state off";
    sei.nShow = SW_HIDE;

    if (ShellExecuteExW(&sei) && sei.hProcess) {
        WaitForSingleObject(sei.hProcess, 8000);
        CloseHandle(sei.hProcess);
        MessageBoxW(hParent,
            L"Windows 防火墙已尝试关闭成功。\n\n"
            L"建议你重新扫描端口，确认整体状态变化。",
            L"操作成功", MB_ICONINFORMATION | MB_OK);
        return true;
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) return false;
        wchar_t errMsg[200];
        swprintf_s(errMsg,
            L"关闭 Windows 防火墙失败（错误码：%lu）。\n"
            L"请尝试以管理员身份运行本程序。", err);
        MessageBoxW(hParent, errMsg, L"操作失败", MB_ICONERROR | MB_OK);
        return false;
    }
}
