// ═══════════════════════════════════════════════════════════
//  main.cpp  —  程序入口
//
//  职责：
//    · UAC 管理员权限检查 + 提权重启
//    · 初始化公共控件
//    · 创建主窗口
//    · 标准 Win32 消息循环
// PortScanner v3.0
// ├── main.cpp          入口 + UAC提权
// ├── resource.h        所有常量（改UI只动这里）
// ├── data.h / .cpp     端口数据库（加端口只动这里）
// ├── scanner.h / .cpp  扫描逻辑（工作线程）
// ├── actions.h / .cpp  终止进程 + 防火墙规则
// ├── ui_theme.h / .cpp 深色主题绘制
// └── ui_window.h / .cpp 窗口 + 控件 + 布局
// ═══════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include "ui_window.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

// ───────────────────────────────────────────────────────────
//  以管理员权限重新启动本程序
// ───────────────────────────────────────────────────────────
static void RelaunchAsAdmin() {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    ShellExecuteW(nullptr, L"runas", path, nullptr, nullptr, SW_SHOW);
}

// ───────────────────────────────────────────────────────────
//  wWinMain
// ───────────────────────────────────────────────────────────
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nShow) {
    // 1. 管理员权限检查
    if (!IsUserAnAdmin()) {
        int choice = MessageBoxW(nullptr,
            L"部分功能（终止进程、添加防火墙规则）需要管理员权限。\n\n"
            L"是否立即以管理员身份重新启动？\n"
            L"（选「否」将以受限模式继续运行）",
            L"需要管理员权限",
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1);
        if (choice == IDYES) {
            RelaunchAsAdmin();
            return 0;
        }
        // 用户选 No → 受限模式继续，各操作内部会单独提示
    }

    // 2. 初始化公共控件（ListView、StatusBar 等）
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    // 3. 创建主窗口
    HWND hWnd = Window_Create(hInst, nShow);
    if (!hWnd) return 1;

    // 4. 消息循环
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
