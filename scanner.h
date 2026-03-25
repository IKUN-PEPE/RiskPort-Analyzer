#pragma once

// ═══════════════════════════════════════════════════════════
//  scanner.h  —  端口扫描模块（工作线程）
// ═══════════════════════════════════════════════════════════

#include "data.h"
#include <map>        // ← 添加这行
#include <vector>

// 一次性构建 PID → 进程名 映射（避免每个端口都创建快照）
std::map<DWORD, std::wstring> BuildPidMap();

// 将 PID 转为进程名（使用预建映射表）
std::wstring PidToName(const std::map<DWORD, std::wstring>& pidMap, DWORD pid);

// 同步扫描所有监听端口（在工作线程中调用）
std::vector<ActivePort> ScanPorts();

// 启动异步扫描线程；完成后向 hNotifyWnd 发送 WM_SCAN_DONE
// lParam 为 new std::vector<ActivePort>*，调用方负责 delete
void StartScanAsync(HWND hNotifyWnd);
