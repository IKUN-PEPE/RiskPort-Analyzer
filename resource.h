#pragma once

// ═══════════════════════════════════════════════════════════
//  resource.h  —  控件 ID、配色、布局尺寸  全部集中在此
// ═══════════════════════════════════════════════════════════

// ── 控件 ID ─────────────────────────────────────────────────
#define IDC_LISTVIEW        100
#define IDC_BTN_SCAN        101
#define IDC_BTN_CLOSE       102
#define IDC_BTN_FW          103
#define IDC_DETAIL_EDIT     104
#define IDC_RADIO_DANGER    105
#define IDC_RADIO_ALL       106
#define IDC_STATUSBAR       108

// ── 自定义消息 ──────────────────────────────────────────────
#define WM_SCAN_DONE        (WM_USER + 1)   // 扫描线程完成

// ═══════════════════════════════════════════════════════════
//  深色「安全终端」配色
// ═══════════════════════════════════════════════════════════
#define C_BG_APP            RGB( 15,  19,  27)   // 应用主背景
#define C_BG_HEADER         RGB( 10,  13,  20)   // 顶部 Banner 背景
#define C_BG_TOOLBAR        RGB( 18,  23,  34)   // 工具栏区背景
#define C_BG_LIST           RGB( 20,  26,  38)   // ListView 背景
#define C_BG_DETAIL         RGB( 16,  21,  32)   // 详情面板背景
#define C_BG_STATUSBAR      RGB( 10,  13,  20)   // 状态栏背景

#define C_ACCENT            RGB(  0, 212, 175)   // 主强调色（青绿）
#define C_ACCENT_HOVER      RGB(  0, 180, 148)   // 按钮悬停
#define C_ACCENT_PRESS      RGB(  0, 130, 108)   // 按钮按下
#define C_ACCENT_DIM        RGB(  0,  80,  66)   // 按钮边框暗色

#define C_BORDER            RGB( 32,  44,  64)   // 面板边框/分割线
#define C_BORDER_BRIGHT     RGB( 48,  66,  96)   // 高亮边框

#define C_TEXT_PRIMARY      RGB(210, 225, 245)   // 主文字
#define C_TEXT_SECONDARY    RGB(110, 135, 170)   // 次要文字
#define C_TEXT_ACCENT       RGB(  0, 212, 175)   // 强调文字
#define C_TEXT_DIM          RGB( 70,  90, 120)   // 暗淡文字

// ── 风险行配色 ─────────────────────────────────────────────
#define C_RISK_CRIT_BG      RGB( 55,  15,  15)
#define C_RISK_CRIT_FG      RGB(255, 100,  85)
#define C_RISK_CRIT_ACCENT  RGB(200,  50,  50)

#define C_RISK_HIGH_BG      RGB( 52,  33,   8)
#define C_RISK_HIGH_FG      RGB(255, 185,  55)
#define C_RISK_HIGH_ACCENT  RGB(200, 130,  20)

#define C_RISK_MED_BG       RGB(  8,  34,  62)
#define C_RISK_MED_FG       RGB( 80, 170, 255)
#define C_RISK_MED_ACCENT   RGB( 30, 100, 200)

#define C_RISK_NORM_BG      C_BG_LIST
#define C_RISK_NORM_FG      C_TEXT_PRIMARY

// ═══════════════════════════════════════════════════════════
//  布局参数（像素）
// ═══════════════════════════════════════════════════════════

// 顶部 Banner 高度
#define BANNER_H            60

// 工具栏（按钮行）参数
#define TOOLBAR_H           56
#define TOOLBAR_Y           BANNER_H          // Banner 正下方

// 按钮尺寸
#define BTN_H               36
#define BTN_RADIUS          6                 // 圆角半径
#define BTN_SCAN_X          14
#define BTN_SCAN_W          136
#define BTN_CLOSE_X         162
#define BTN_CLOSE_W         220
#define BTN_FW_X            394
#define BTN_FW_W            190
#define BTN_Y_OFFSET        10                // 按钮在工具栏内的垂直偏移

// 单选按钮
#define RADIO_DANGER_X      600
#define RADIO_ALL_X         760
#define RADIO_W             148
#define RADIO_Y_OFFSET      18

// ListView 上边距（相对于客户区）
#define LIST_TOP            (BANNER_H + TOOLBAR_H + 4)

// 详情面板高度（底部固定）
#define DETAIL_H            185

// 最小窗口尺寸
#define MIN_WIN_W           680
#define MIN_WIN_H           560

// ═══════════════════════════════════════════════════════════
//  字体参数
// ═══════════════════════════════════════════════════════════
#define FONT_SIZE_UI        19    // 按钮、标签、列表内容
#define FONT_SIZE_BANNER    22    // Banner 标题
#define FONT_SIZE_SUBTITLE  13    // Banner 副标题
#define FONT_SIZE_DETAIL    16    // 详情框 Consolas
#define FONT_NAME_UI        L"Microsoft YaHei UI"
#define FONT_NAME_MONO      L"Consolas"
