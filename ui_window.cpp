// ═══════════════════════════════════════════════════════════
//  ui_window.cpp  —  主窗口实现
//
//  职责：
//    · 创建并布局所有子控件
//    · 消息循环路由（WM_COMMAND / WM_NOTIFY / WM_SIZE ...）
//    · ListView 数据填充与刷新
//    · 详情面板文本构建
//    · 鼠标悬停跟踪（TME）→ 驱动按钮高亮重绘
// ═══════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include "ui_window.h"
#include "ui_theme.h"
#include "resource.h"
#include "scanner.h"
#include "actions.h"

#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <windowsx.h>  // ← 添加这行（GET_X_LPARAM / GET_Y_LPARAM）

#pragma comment(lib, "comctl32.lib")

// ───────────────────────────────────────────────────────────
//  全局状态
// ───────────────────────────────────────────────────────────
HWND g_hMainWnd    = nullptr;
HWND g_hList       = nullptr;
HWND g_hDetailEdit = nullptr;
HWND g_hStatusBar  = nullptr;

bool g_showAll  = false;
bool g_scanning = false;

std::vector<ActivePort> g_results;

// ───────────────────────────────────────────────────────────
//  ListView 填充
// ───────────────────────────────────────────────────────────
void Window_RefreshList() {
    ListView_DeleteAllItems(g_hList);
    int row = 0;

    for (int idx = 0; idx < (int)g_results.size(); ++idx) {
        const ActivePort& ap = g_results[idx];
        if (!g_showAll && !ap.info) continue;   // 过滤非高危

        LVITEMW lvi  = {};
        lvi.mask     = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem    = row;
        lvi.lParam   = (LPARAM)idx;             // 存索引，不存指针

        wchar_t portBuf[16];
        swprintf_s(portBuf, L"%d", ap.port);
        lvi.pszText = portBuf;
        ListView_InsertItem(g_hList, &lvi);

        // 协议
        ListView_SetItemText(g_hList, row, 1, (LPWSTR)ap.proto.c_str());
        // 状态
        ListView_SetItemText(g_hList, row, 2, (LPWSTR)ap.state.c_str());
        // 服务名 / 风险
        if (ap.info) {
            ListView_SetItemText(g_hList, row, 3, (LPWSTR)ap.info->service.c_str());
            ListView_SetItemText(g_hList, row, 4, (LPWSTR)RiskLevelStr(ap.info->risk).c_str());
        } else {
            wchar_t dash[] = L"—", norm[] = L"正常";
            ListView_SetItemText(g_hList, row, 3, dash);
            ListView_SetItemText(g_hList, row, 4, norm);
        }
        // PID / 进程名
        wchar_t pidBuf[16];
        swprintf_s(pidBuf, L"%lu", ap.pid);
        ListView_SetItemText(g_hList, row, 5, pidBuf);
        ListView_SetItemText(g_hList, row, 6, (LPWSTR)ap.processName.c_str());

        ++row;
    }
}

// ───────────────────────────────────────────────────────────
//  获取选中行对应的 ActivePort
// ───────────────────────────────────────────────────────────
ActivePort* Window_GetSelectedPort() {
    int sel = ListView_GetNextItem(g_hList, -1, LVNI_SELECTED);
    if (sel < 0) return nullptr;

    LVITEMW lvi = {};
    lvi.mask    = LVIF_PARAM;
    lvi.iItem   = sel;
    ListView_GetItem(g_hList, &lvi);

    int idx = (int)lvi.lParam;
    if (idx < 0 || idx >= (int)g_results.size()) return nullptr;
    return &g_results[idx];
}

// ───────────────────────────────────────────────────────────
//  详情面板
// ───────────────────────────────────────────────────────────
void Window_ShowDetail() {
    const ActivePort* ap = Window_GetSelectedPort();
    if (!ap) return;

    std::wstring t;
    t += L"  端口 : " + std::to_wstring(ap->port)
      +  L"    协议 : " + ap->proto
      +  L"    状态 : " + ap->state + L"\r\n";
    t += L"  PID  : " + std::to_wstring(ap->pid)
      +  L"    进程 : " + ap->processName + L"\r\n";
    t += L"  ─────────────────────────────────────────\r\n";

    if (ap->info) {
        t += L"  服务    : " + ap->info->service + L"\r\n";
        t += L"  风险等级: " + RiskLevelStr(ap->info->risk) + L"\r\n\r\n";
        t += L"  【危险描述】\r\n  " + ap->info->description + L"\r\n\r\n";
        t += L"  【修复建议】\r\n  " + ap->info->mitigation + L"\r\n";
    } else {
        t += L"  此端口未在高危数据库中，属于正常监听端口。\r\n";
    }

    SetWindowTextW(g_hDetailEdit, t.c_str());
}

// ───────────────────────────────────────────────────────────
//  创建控件
// ───────────────────────────────────────────────────────────
static void CreateControls(HWND hWnd, HINSTANCE hInst) {
    int toolY = TOOLBAR_Y + BTN_Y_OFFSET;

    // ── 功能按钮（owner-draw）──────────────────────────────
    auto makeBtn = [&](const wchar_t* text, int x, int w, int id) {
        CreateWindowW(L"BUTTON", text,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            x, toolY, w, BTN_H, hWnd, (HMENU)(UINT_PTR)id, hInst, nullptr);
    };
    makeBtn(L"  🔍  重新扫描",         BTN_SCAN_X,  BTN_SCAN_W,  IDC_BTN_SCAN);
    makeBtn(L"  ⛔  关闭端口（终止进程）", BTN_CLOSE_X, BTN_CLOSE_W, IDC_BTN_CLOSE);
    makeBtn(L"  🛡  添加防火墙拦截",    BTN_FW_X,    BTN_FW_W,    IDC_BTN_FW);

    // ── 单选按钮 ───────────────────────────────────────────
    int radioY = TOOLBAR_Y + RADIO_Y_OFFSET;
    CreateWindowW(L"BUTTON", L"仅高危端口",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP,
        RADIO_DANGER_X, radioY, RADIO_W, 22,
        hWnd, (HMENU)IDC_RADIO_DANGER, hInst, nullptr);
    CreateWindowW(L"BUTTON", L"显示全部",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP,
        RADIO_ALL_X, radioY, 100, 22,
        hWnd, (HMENU)IDC_RADIO_ALL, hInst, nullptr);
    CheckRadioButton(hWnd, IDC_RADIO_DANGER, IDC_RADIO_ALL, IDC_RADIO_DANGER);

    // ── ListView ───────────────────────────────────────────
    g_hList = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        0, LIST_TOP, 100, 100,      // 实际尺寸在 WM_SIZE 设置
        hWnd, (HMENU)IDC_LISTVIEW, hInst, nullptr);

    ListView_SetExtendedListViewStyle(g_hList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // 深色背景 + 文字颜色
    ListView_SetBkColor(g_hList,   C_BG_LIST);
    ListView_SetTextBkColor(g_hList, C_BG_LIST);
    ListView_SetTextColor(g_hList,  C_TEXT_PRIMARY);

    // 列定义
    struct ColDef { const wchar_t* name; int width; };
    static const ColDef cols[] = {
        {L"端口",  70}, {L"协议",  62}, {L"状态",  105},
        {L"服务名",145}, {L"风险",  80}, {L"PID",   68},
        {L"进程名",200}
    };
    for (int i = 0; i < 7; ++i) {
        LVCOLUMNW lvc = {};
        lvc.mask    = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        lvc.fmt     = LVCFMT_LEFT;
        lvc.pszText = (LPWSTR)cols[i].name;
        lvc.cx      = cols[i].width;
        ListView_InsertColumn(g_hList, i, &lvc);
    }

    // ── 详情文本框 ─────────────────────────────────────────
    g_hDetailEdit = CreateWindowExW(0, L"EDIT",
        L"  ← 点击列表中的端口查看详细信息",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        0, 0, 100, 100,
        hWnd, (HMENU)IDC_DETAIL_EDIT, hInst, nullptr);

    // ── 状态栏 ─────────────────────────────────────────────
    g_hStatusBar = CreateWindowW(STATUSCLASSNAME, nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        hWnd, (HMENU)IDC_STATUSBAR, hInst, nullptr);

    // 状态栏分区（两段）
    RECT cr; GetClientRect(hWnd, &cr);
    int parts[] = { cr.right - 200, -1 };
    SendMessageW(g_hStatusBar, SB_SETPARTS, 2, (LPARAM)parts);
    SendMessageW(g_hStatusBar, SB_SETTEXTW, 0,
        (LPARAM)L"  就绪 — 点击「重新扫描」开始检测");

    // ── 为所有控件设置字体 ──────────────────────────────────
    EnumChildWindows(hWnd, [](HWND h, LPARAM lp) -> BOOL {
        SendMessageW(h, WM_SETFONT, lp, TRUE);
        return TRUE;
    }, (LPARAM)g_hFontUI);

    // 详情框单独用等宽字体
    SendMessageW(g_hDetailEdit, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);
}

// ───────────────────────────────────────────────────────────
//  WM_SIZE 布局
// ───────────────────────────────────────────────────────────
static void OnSize(HWND hWnd, int w, int h) {
    if (w < MIN_WIN_W) w = MIN_WIN_W;
    if (h < MIN_WIN_H) h = MIN_WIN_H;

    // 状态栏先 resize（自动计算高度）
    if (g_hStatusBar) SendMessageW(g_hStatusBar, WM_SIZE, 0, 0);

    RECT sbRect = {};
    if (g_hStatusBar) GetWindowRect(g_hStatusBar, &sbRect);
    int sbH = sbRect.bottom - sbRect.top;

    int listTop    = LIST_TOP;
    int detailTop  = h - sbH - DETAIL_H - 2;
    int listH      = detailTop - listTop - 4;
    if (listH < 40) listH = 40;

    // 更新状态栏分区宽度
    int parts[] = { w - 220, -1 };
    SendMessageW(g_hStatusBar, SB_SETPARTS, 2, (LPARAM)parts);

    if (g_hList)
        SetWindowPos(g_hList, nullptr, 0, listTop,
                     w, listH, SWP_NOZORDER | SWP_NOACTIVATE);

    if (g_hDetailEdit)
        SetWindowPos(g_hDetailEdit, nullptr, 0, detailTop,
                     w, DETAIL_H, SWP_NOZORDER | SWP_NOACTIVATE);
}

// ───────────────────────────────────────────────────────────
//  扫描完成处理
// ───────────────────────────────────────────────────────────
static void OnScanDone(LPARAM lParam) {
    auto* res = reinterpret_cast<std::vector<ActivePort>*>(lParam);
    g_results  = std::move(*res);
    delete res;

    int total = (int)g_results.size();
    int danger = 0, critical = 0;
    for (const auto& r : g_results) {
        if (r.info) {
            ++danger;
            if (r.info->risk == RISK_CRITICAL) ++critical;
        }
    }

    Window_RefreshList();

    wchar_t status[256];
    swprintf_s(status,
        L"  扫描完成  |  共 %d 个监听端口  |  高危: %d  |  严重: %d",
        total, danger, critical);
    SendMessageW(g_hStatusBar, SB_SETTEXTW, 0, (LPARAM)status);

    wchar_t right[64];
    swprintf_s(right, L"  严重 %d  高危 %d", critical, danger - critical);
    SendMessageW(g_hStatusBar, SB_SETTEXTW, 1, (LPARAM)right);

    g_scanning = false;
    EnableWindow(GetDlgItem(g_hMainWnd, IDC_BTN_SCAN), TRUE);
}

// ───────────────────────────────────────────────────────────
//  按钮悬停跟踪（驱动 owner-draw 高亮）
// ───────────────────────────────────────────────────────────
static void TrackButtonHover(HWND hWnd, LPARAM lParam) {
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    int newHover = -1;

    for (int id : { IDC_BTN_SCAN, IDC_BTN_CLOSE, IDC_BTN_FW }) {
        HWND hBtn = GetDlgItem(hWnd, id);
        if (!hBtn) continue;
        RECT rc; GetWindowRect(hBtn, &rc);
        MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rc, 2);
        if (PtInRect(&rc, pt)) { newHover = id; break; }
    }

    if (newHover != g_btnHover) {
        g_btnHover = newHover;
        // 重绘三个按钮
        for (int id : { IDC_BTN_SCAN, IDC_BTN_CLOSE, IDC_BTN_FW }) {
            HWND hBtn = GetDlgItem(hWnd, id);
            if (hBtn) InvalidateRect(hBtn, nullptr, FALSE);
        }
        if (newHover != -1) {
            TRACKMOUSEEVENT tme = {};
            tme.cbSize    = sizeof(tme);
            tme.dwFlags   = TME_LEAVE;
            tme.hwndTrack = hWnd;
            TrackMouseEvent(&tme);
        }
    }
}

// ───────────────────────────────────────────────────────────
//  窗口过程
// ───────────────────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    // ── 创建 ──────────────────────────────────────────────
    case WM_CREATE: {
        g_hMainWnd = hWnd;
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        Theme_Init(hWnd);
        CreateControls(hWnd, hInst);
        PostMessageW(hWnd, WM_COMMAND, IDC_BTN_SCAN, 0);   // 启动时自动扫描
        return 0;
    }

    // ── 背景擦除（完全自绘）────────────────────────────────
    case WM_ERASEBKGND:
        return Theme_OnEraseBkgnd(hWnd, (HDC)wParam);

    // ── 控件颜色 ──────────────────────────────────────────
    case WM_CTLCOLOREDIT:
        return (LRESULT)Theme_OnCtlColorEdit((HDC)wParam, (HWND)lParam);
    case WM_CTLCOLORSTATIC:
        return (LRESULT)Theme_OnCtlColorStatic((HDC)wParam, (HWND)lParam);
    case WM_CTLCOLORBTN:
        return (LRESULT)Theme_OnCtlColorBtn((HDC)wParam, (HWND)lParam);

    // ── owner-draw（按钮）─────────────────────────────────
    case WM_DRAWITEM:
        Theme_DrawButton((LPDRAWITEMSTRUCT)lParam);
        return TRUE;

    // ── 鼠标移动（按钮悬停高亮）──────────────────────────
    case WM_MOUSEMOVE:
        TrackButtonHover(hWnd, lParam);
        return 0;
    case WM_MOUSELEAVE:
        g_btnHover = -1;
        for (int id : { IDC_BTN_SCAN, IDC_BTN_CLOSE, IDC_BTN_FW }) {
            HWND hBtn = GetDlgItem(hWnd, id);
            if (hBtn) InvalidateRect(hBtn, nullptr, FALSE);
        }
        return 0;

    // ── 命令 ──────────────────────────────────────────────
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_SCAN:
            if (!g_scanning) {
                g_scanning = true;
                EnableWindow(GetDlgItem(hWnd, IDC_BTN_SCAN), FALSE);
                SendMessageW(g_hStatusBar, SB_SETTEXTW, 0,
                    (LPARAM)L"  正在扫描端口，请稍候...");
                StartScanAsync(hWnd);
            }
            break;

        case IDC_BTN_CLOSE: {
            ActivePort* ap = Window_GetSelectedPort();
            if (!ap)
                MessageBoxW(hWnd, L"请先在列表中选择一个端口。",
                            L"提示", MB_ICONINFORMATION);
            else
                ActionClosePort(hWnd, *ap);
            break;
        }
        case IDC_BTN_FW: {
            ActivePort* ap = Window_GetSelectedPort();
            if (!ap)
                MessageBoxW(hWnd, L"请先在列表中选择一个端口。",
                            L"提示", MB_ICONINFORMATION);
            else
                ActionAddFirewallBlock(hWnd, *ap);
            break;
        }
        case IDC_RADIO_DANGER:
            g_showAll = false;
            Window_RefreshList();
            break;
        case IDC_RADIO_ALL:
            g_showAll = true;
            Window_RefreshList();
            break;
        }
        return 0;

    // ── 通知（ListView）───────────────────────────────────
    case WM_NOTIFY: {
        auto* hdr = reinterpret_cast<NMHDR*>(lParam);
        if (hdr->idFrom == IDC_LISTVIEW) {
            if (hdr->code == NM_CUSTOMDRAW)
                return Theme_ListCustomDraw(lParam, g_results);
            if (hdr->code == NM_CLICK || hdr->code == LVN_ITEMCHANGED)
                Window_ShowDetail();
        }
        break;
    }

    // ── 扫描线程完成 ──────────────────────────────────────
    case WM_SCAN_DONE:
        OnScanDone(lParam);
        return 0;

    // ── 尺寸变化 ──────────────────────────────────────────
    case WM_SIZE:
        OnSize(hWnd, LOWORD(lParam), HIWORD(lParam));
        return 0;

    // ── 最小尺寸限制 ──────────────────────────────────────
    case WM_GETMINMAXINFO: {
        auto* mm = reinterpret_cast<MINMAXINFO*>(lParam);
        mm->ptMinTrackSize = { MIN_WIN_W, MIN_WIN_H };
        return 0;
    }

    // ── 销毁 ──────────────────────────────────────────────
    case WM_DESTROY:
        Theme_Destroy();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ───────────────────────────────────────────────────────────
//  窗口注册 + 创建
// ───────────────────────────────────────────────────────────
HWND Window_Create(HINSTANCE hInst, int nShow) {
    WNDCLASSEXW wc  = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;             // 完全自绘，不需要系统画刷
    wc.lpszClassName = L"PortScannerWnd";
    wc.hIcon         = LoadIcon(nullptr, IDI_SHIELD);
    wc.hIconSm       = LoadIcon(nullptr, IDI_SHIELD);
    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowExW(
        0,                   // 双缓冲，减少闪烁
        L"PortScannerWnd",
        L"高危端口检测与管理  v3.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 980, 720,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(hWnd, nShow);
    UpdateWindow(hWnd);
    return hWnd;
}
