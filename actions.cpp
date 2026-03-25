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

#pragma comment(lib, "shell32.lib")

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
void ActionAddFirewallBlock(HWND hParent, const ActivePort& ap) {
    // proto 白名单校验，防止命令注入
    if (ap.proto != L"TCP" && ap.proto != L"UDP") {
        MessageBoxW(hParent, L"不支持的协议类型，操作取消。",
                    L"错误", MB_ICONERROR | MB_OK);
        return;
    }

    // 规则名只含整数端口和白名单协议，无注入风险
    wchar_t ruleName[64];
    swprintf_s(ruleName, L"PortScanner_Block_%s_%d", ap.proto.c_str(), ap.port);

    wchar_t cmd[320];
    swprintf_s(cmd,
        L"netsh advfirewall firewall add rule "
        L"name=\"%s\" dir=in action=block protocol=%s localport=%d",
        ruleName, ap.proto.c_str(), ap.port);

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
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            // 用户取消了 UAC 提权，静默忽略
            return;
        }
        wchar_t errMsg[200];
        swprintf_s(errMsg,
            L"添加防火墙规则失败（错误码：%lu）。\n"
            L"请以管理员身份运行本程序后重试。", err);
        MessageBoxW(hParent, errMsg, L"操作失败", MB_ICONERROR | MB_OK);
    }
}
