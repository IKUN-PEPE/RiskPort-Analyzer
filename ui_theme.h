#pragma once

// ═══════════════════════════════════════════════════════════
//  ui_theme.h  —  深色主题自绘：背景、Banner、按钮
// ═══════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include <vector>     // ← 添加这行

// ── 全局字体句柄（由 Theme_Init 创建，Theme_Destroy 释放）──
extern HFONT g_hFontUI;       // 控件通用字体（大号）
extern HFONT g_hFontBanner;   // Banner 标题字体
extern HFONT g_hFontMono;     // 详情框等宽字体

// ── 按钮悬停状态跟踪 ────────────────────────────────────────
extern int   g_btnHover;      // 当前悬停的按钮 IDC，-1 表示无

// 初始化：创建字体、画刷
void Theme_Init(HWND hWnd);

// 销毁：释放 GDI 资源
void Theme_Destroy();

// 绘制顶部 Banner（标题 + 副标题 + 底部亮线）
void Theme_DrawBanner(HDC hdc, const RECT& clientRect);

// 绘制工具栏背景
void Theme_DrawToolbar(HDC hdc, const RECT& clientRect);

// 绘制主背景（Banner + Toolbar 以外的区域）
void Theme_DrawBackground(HDC hdc, const RECT& clientRect);

// 处理 WM_ERASEBKGND：整体背景填充
BOOL Theme_OnEraseBkgnd(HWND hWnd, HDC hdc);

// 处理 WM_CTLCOLOREDIT（详情框背景）
HBRUSH Theme_OnCtlColorEdit(HDC hdc, HWND hCtrl);

// 处理 WM_CTLCOLORSTATIC（单选按钮等静态控件）
HBRUSH Theme_OnCtlColorStatic(HDC hdc, HWND hCtrl);

// 处理 WM_CTLCOLORBTN（按钮背景，用于普通 BUTTON 子类）
HBRUSH Theme_OnCtlColorBtn(HDC hdc, HWND hCtrl);

// 绘制自定义按钮（WM_DRAWITEM / owner-draw）
void Theme_DrawButton(LPDRAWITEMSTRUCT dis);

// ListView 自定义绘制回调（NM_CUSTOMDRAW）
LRESULT Theme_ListCustomDraw(LPARAM lParam,
                              const std::vector<struct ActivePort>& results);
