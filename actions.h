#pragma once

// ═══════════════════════════════════════════════════════════
//  actions.h  —  业务操作：终止进程 / 添加防火墙规则
// ═══════════════════════════════════════════════════════════

#include "data.h"

enum FirewallGlobalState {
    FW_GLOBAL_UNKNOWN = 0,
    FW_GLOBAL_OFF,
    FW_GLOBAL_ON,
};

// 检查进程是否受保护（系统关键进程，禁止终止）
bool IsProtectedProcess(DWORD pid, const std::wstring& processName);

// 终止进程（需要管理员权限）
// 返回 true = 成功，false = 失败（GetLastError 获取错误码）
bool KillProcessById(DWORD pid);

// 对选中端口执行"关闭端口"操作（含确认对话框、权限检查）
void ActionClosePort(HWND hParent, const ActivePort& ap);

// 对选中端口添加 Windows 防火墙入站阻断规则
bool ActionAddFirewallBlock(HWND hParent, const ActivePort& ap);

// 对选中端口移除本工具创建的 Windows 防火墙阻断规则
bool ActionRemoveFirewallBlock(HWND hParent, const ActivePort& ap);

// 自动开启 Windows 防火墙（如果当前已关闭）
bool ActionEnableFirewall(HWND hParent);

// 关闭 Windows 防火墙（如果当前已开启）
bool ActionDisableFirewall(HWND hParent);

// 查询 Windows 防火墙整体状态
FirewallGlobalState QueryFirewallGlobalState();
std::wstring FirewallGlobalStateStr(FirewallGlobalState s);

// 查询本工具是否已为该端口创建防火墙阻断规则
FirewallBlockState QueryFirewallBlockState(const ActivePort& ap);

// 辅助文字
std::wstring FirewallBlockStateStr(FirewallBlockState s);
