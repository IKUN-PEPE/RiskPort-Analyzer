#pragma once

// ═══════════════════════════════════════════════════════════
//  data.h  —  高危端口数据库 + 核心数据结构
// ═══════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <winsock2.h>   // winsock2 必须在 windows.h 之前，此处统一用它
#include <windows.h>
#include <string>
#include <vector>

// ── 风险等级 ────────────────────────────────────────────────
enum RiskLevel {
    RISK_CRITICAL = 0,   // 严重
    RISK_HIGH,           // 高危
    RISK_MEDIUM,         // 中危
};

enum FirewallBlockState {
    FW_BLOCK_UNKNOWN = 0,
    FW_BLOCK_NO,
    FW_BLOCK_YES,
};

// ── 高危端口描述记录 ────────────────────────────────────────
struct PortInfo {
    int          port;
    std::wstring proto;        // L"TCP" / L"UDP"
    std::wstring service;      // 服务名称
    RiskLevel    risk;
    std::wstring description;  // 危险描述
    std::wstring mitigation;   // 修复建议
};

// ── 扫描结果：一个活动监听端口 ─────────────────────────────
struct ActivePort {
    int             port;
    std::wstring    proto;
    std::wstring    state;
    DWORD           pid;
    std::wstring    processName;
    const PortInfo* info;      // nullptr = 不在高危库中
    FirewallBlockState firewallBlockState;
};

// ── 全局高危端口数据库（定义在 data.cpp）─────────────────
extern const std::vector<PortInfo> DANGEROUS_PORTS;

// ── 查询函数 ────────────────────────────────────────────────
const PortInfo* FindPortInfo(int port, const std::wstring& proto);

// ── 辅助文字 ────────────────────────────────────────────────
std::wstring RiskLevelStr(RiskLevel r);
