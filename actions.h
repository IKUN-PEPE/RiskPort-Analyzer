#pragma once

// ═══════════════════════════════════════════════════════════
//  actions.h  —  业务操作：终止进程 / 添加防火墙规则
// ═══════════════════════════════════════════════════════════

#include "data.h"

// 检查进程是否受保护（系统关键进程，禁止终止）
bool IsProtectedProcess(DWORD pid, const std::wstring& processName);

// 终止进程（需要管理员权限）
// 返回 true = 成功，false = 失败（GetLastError 获取错误码）
bool KillProcessById(DWORD pid);

// 对选中端口执行"关闭端口"操作（含确认对话框、权限检查）
void ActionClosePort(HWND hParent, const ActivePort& ap);

// 对选中端口添加 Windows 防火墙入站阻断规则
void ActionAddFirewallBlock(HWND hParent, const ActivePort& ap);
