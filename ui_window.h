#pragma once

// ═══════════════════════════════════════════════════════════
//  ui_window.h  —  主窗口：控件创建、消息路由、ListView 刷新
// ═══════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include "data.h"
#include <vector>

// ── 全局窗口句柄 ────────────────────────────────────────────
extern HWND g_hMainWnd;
extern HWND g_hList;
extern HWND g_hDetailEdit;
extern HWND g_hStatusBar;

// ── 当前显示模式 ────────────────────────────────────────────
extern bool g_showAll;    // true = 全部端口，false = 仅高危
extern bool g_scanning;   // 正在扫描中

// ── 当前扫描结果 ────────────────────────────────────────────
extern std::vector<ActivePort> g_results;

// 注册并创建主窗口
HWND Window_Create(HINSTANCE hInst, int nShow);

// 窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg,
                          WPARAM wParam, LPARAM lParam);

// 根据 g_results 和 g_showAll 刷新 ListView 内容
void Window_RefreshList();

// 获取当前 ListView 选中项对应的 ActivePort（nullptr = 无选中）
ActivePort* Window_GetSelectedPort();

// 在详情框显示选中端口的信息
void Window_ShowDetail();
