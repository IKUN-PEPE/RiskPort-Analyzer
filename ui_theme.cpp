// ═══════════════════════════════════════════════════════════
//  ui_theme.cpp  —  深色安全主题绘制实现
//
//  风格：深色终端 + 青绿强调色
//    · 顶部 Banner：渐变底色 + 盾牌图标 + 标题/副标题
//    · 工具栏：深灰底 + owner-draw 圆角按钮
//    · ListView：深色背景 + 彩色风险行
//    · 详情面板：深色背景 + 等宽字体
// ═══════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include "ui_theme.h"
#include "data.h"
#include <vector>

// ───────────────────────────────────────────────────────────
//  全局 GDI 资源
// ───────────────────────────────────────────────────────────
HFONT  g_hFontUI     = nullptr;
HFONT  g_hFontBanner = nullptr;
HFONT  g_hFontMono   = nullptr;
int    g_btnHover    = -1;

static HBRUSH s_brAppBg     = nullptr;
static HBRUSH s_brPanelBg   = nullptr;
static HBRUSH s_brToolbar   = nullptr;
static HBRUSH s_brDetailBg  = nullptr;
static HBRUSH s_brHeaderBg  = nullptr;
static HBRUSH s_brTransp    = nullptr;  // 透明（单选按钮用）

// ───────────────────────────────────────────────────────────
//  初始化 / 销毁
// ───────────────────────────────────────────────────────────
void Theme_Init(HWND /*hWnd*/) {
    // 字体
    g_hFontUI = CreateFontW(
        -FONT_SIZE_UI, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        FONT_NAME_UI);

    g_hFontBanner = CreateFontW(
        -FONT_SIZE_BANNER, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        FONT_NAME_UI);

    g_hFontMono = CreateFontW(
        -FONT_SIZE_DETAIL, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_DONTCARE,
        FONT_NAME_MONO);

    // 画刷
    s_brAppBg    = CreateSolidBrush(C_BG_APP);
    s_brPanelBg  = CreateSolidBrush(C_BG_LIST);
    s_brToolbar  = CreateSolidBrush(C_BG_TOOLBAR);
    s_brDetailBg = CreateSolidBrush(C_BG_DETAIL);
    s_brHeaderBg = CreateSolidBrush(C_BG_HEADER);
    s_brTransp   = CreateSolidBrush(C_BG_TOOLBAR);  // 与工具栏同色
}

void Theme_Destroy() {
    auto safeFont  = [](HFONT&  f) { if (f) { DeleteObject(f); f = nullptr; } };
    auto safeBrush = [](HBRUSH& b) { if (b) { DeleteObject(b); b = nullptr; } };

    safeFont(g_hFontUI);
    safeFont(g_hFontBanner);
    safeFont(g_hFontMono);

    safeBrush(s_brAppBg);
    safeBrush(s_brPanelBg);
    safeBrush(s_brToolbar);
    safeBrush(s_brDetailBg);
    safeBrush(s_brHeaderBg);
    safeBrush(s_brTransp);
}

// ───────────────────────────────────────────────────────────
//  辅助：填充纯色矩形
// ───────────────────────────────────────────────────────────
static void FillRect(HDC hdc, int x, int y, int w, int h, COLORREF c) {
    RECT r = { x, y, x + w, y + h };
    HBRUSH br = CreateSolidBrush(c);
    ::FillRect(hdc, &r, br);
    DeleteObject(br);
}

// ───────────────────────────────────────────────────────────
//  Banner（顶部标题区）
// ───────────────────────────────────────────────────────────
void Theme_DrawBanner(HDC hdc, const RECT& cr) {
    int W = cr.right - cr.left;

    // 背景
    FillRect(hdc, 0, 0, W, BANNER_H, C_BG_HEADER);

    // 左侧竖色条（强调色）
    FillRect(hdc, 0, 0, 4, BANNER_H, C_ACCENT);

    // 底部分割线
    FillRect(hdc, 0, BANNER_H - 1, W, 1, C_BORDER_BRIGHT);

    // 盾牌图标（Unicode 字符代替）
    SetBkMode(hdc, TRANSPARENT);
    HFONT hOld = (HFONT)SelectObject(hdc, g_hFontBanner);

    // 图标
    SetTextColor(hdc, C_ACCENT);
    TextOutW(hdc, 16, 10, L"🛡", 2);

    // 主标题
    SetTextColor(hdc, C_TEXT_PRIMARY);
    TextOutW(hdc, 56, 8, L"高危端口检测与管理工具", 11);

    // 副标题（小字）
    HFONT hSub = CreateFontW(-FONT_SIZE_SUBTITLE, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, FONT_NAME_UI);
    SelectObject(hdc, hSub);
    SetTextColor(hdc, C_TEXT_SECONDARY);
    TextOutW(hdc, 58, 36, L"Windows Port Security Scanner  v3.0", 35);
    SelectObject(hdc, hOld);
    DeleteObject(hSub);

    // 右侧角标说明
    SelectObject(hdc, hSub = CreateFontW(-FONT_SIZE_SUBTITLE, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, FONT_NAME_UI));
    SetTextColor(hdc, C_TEXT_DIM);
    wchar_t ver[] = L"建议以管理员权限运行";
    SIZE sz; GetTextExtentPoint32W(hdc, ver, (int)wcslen(ver), &sz);
    TextOutW(hdc, W - sz.cx - 14, (BANNER_H - sz.cy) / 2, ver, (int)wcslen(ver));
    SelectObject(hdc, hOld);
    DeleteObject(hSub);
}

// ───────────────────────────────────────────────────────────
//  工具栏背景
// ───────────────────────────────────────────────────────────
void Theme_DrawToolbar(HDC hdc, const RECT& cr) {
    int W = cr.right - cr.left;
    FillRect(hdc, 0, TOOLBAR_Y, W, TOOLBAR_H, C_BG_TOOLBAR);
    // 底部细线
    FillRect(hdc, 0, TOOLBAR_Y + TOOLBAR_H - 1, W, 1, C_BORDER);
}

// ───────────────────────────────────────────────────────────
//  主背景
// ───────────────────────────────────────────────────────────
void Theme_DrawBackground(HDC hdc, const RECT& cr) {
    int W = cr.right - cr.left;
    int H = cr.bottom - cr.top;
    FillRect(hdc, 0, BANNER_H + TOOLBAR_H, W, H - BANNER_H - TOOLBAR_H, C_BG_APP);
}

// ───────────────────────────────────────────────────────────
//  WM_ERASEBKGND
// ───────────────────────────────────────────────────────────
BOOL Theme_OnEraseBkgnd(HWND hWnd, HDC hdc) {
    RECT cr;
    GetClientRect(hWnd, &cr);
    int w = cr.right, h = cr.bottom;

    // 创建内存 DC，画完再一次性贴到屏幕
    HDC     memDC  = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    Theme_DrawBanner(memDC, cr);
    Theme_DrawToolbar(memDC, cr);
    Theme_DrawBackground(memDC, cr);

    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
    return TRUE;
}

// ───────────────────────────────────────────────────────────
//  控件颜色消息
// ───────────────────────────────────────────────────────────
HBRUSH Theme_OnCtlColorEdit(HDC hdc, HWND /*hCtrl*/) {
    SetBkColor(hdc, C_BG_DETAIL);
    SetTextColor(hdc, C_TEXT_PRIMARY);
    return s_brDetailBg;
}

HBRUSH Theme_OnCtlColorStatic(HDC hdc, HWND /*hCtrl*/) {
    SetBkColor(hdc, C_BG_TOOLBAR);
    SetTextColor(hdc, C_TEXT_PRIMARY);
    return s_brTransp;
}

HBRUSH Theme_OnCtlColorBtn(HDC hdc, HWND /*hCtrl*/) {
    SetBkColor(hdc, C_BG_TOOLBAR);
    SetTextColor(hdc, C_TEXT_PRIMARY);
    return s_brTransp;
}

// ───────────────────────────────────────────────────────────
//  owner-draw 按钮绘制
// ───────────────────────────────────────────────────────────
void Theme_DrawButton(LPDRAWITEMSTRUCT dis) {
    HDC  hdc  = dis->hDC;
    RECT rc   = dis->rcItem;
    bool hover = (g_btnHover == (int)dis->CtlID);
    bool press = (dis->itemState & ODS_SELECTED) != 0;

    // 背景色
    COLORREF bgColor = press  ? C_ACCENT_PRESS
                     : hover  ? C_ACCENT_HOVER
                              : C_ACCENT;

    // 圆角矩形背景
    HBRUSH br = CreateSolidBrush(bgColor);
    HPEN   pn = CreatePen(PS_SOLID, 0, bgColor);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
    HPEN   op = (HPEN)SelectObject(hdc, pn);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom,
              BTN_RADIUS * 2, BTN_RADIUS * 2);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(br);
    DeleteObject(pn);

    // 按钮文字
    wchar_t text[128] = {};
    GetWindowTextW(dis->hwndItem, text, 127);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(10, 15, 22));  // 深色文字，在青绿底上清晰

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontUI);
    DrawTextW(hdc, text, -1, &rc,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);

    // 焦点虚线框
    if (dis->itemState & ODS_FOCUS) {
        RECT fr = { rc.left+3, rc.top+3, rc.right-3, rc.bottom-3 };
        DrawFocusRect(hdc, &fr);
    }
}

// ───────────────────────────────────────────────────────────
//  ListView 自定义绘制
// ───────────────────────────────────────────────────────────
LRESULT Theme_ListCustomDraw(LPARAM lParam,
                              const std::vector<ActivePort>& results)
{
    auto* cd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

    switch (cd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT: {
        int idx = (int)cd->nmcd.lItemlParam;
        if (idx < 0 || idx >= (int)results.size())
            return CDRF_DODEFAULT;

        const ActivePort& ap = results[idx];
        if (ap.info) {
            switch (ap.info->risk) {
            case RISK_CRITICAL:
                cd->clrTextBk = C_RISK_CRIT_BG;
                cd->clrText   = C_RISK_CRIT_FG;
                break;
            case RISK_HIGH:
                cd->clrTextBk = C_RISK_HIGH_BG;
                cd->clrText   = C_RISK_HIGH_FG;
                break;
            case RISK_MEDIUM:
                cd->clrTextBk = C_RISK_MED_BG;
                cd->clrText   = C_RISK_MED_FG;
                break;
            }
        } else {
            cd->clrTextBk = C_RISK_NORM_BG;
            cd->clrText   = C_RISK_NORM_FG;
        }
        return CDRF_NEWFONT;
    }
    }
    return CDRF_DODEFAULT;
}
