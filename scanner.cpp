// ═══════════════════════════════════════════════════════════
//  scanner.cpp  —  端口扫描逻辑
//
//  设计要点：
//    · BuildPidMap() 一次快照取全部进程，避免 N 次重复创建
//    · 用 set<(port,proto)> 统一去重，TCP4/TCP6/UDP 均适用
//    · ScanThread 在工作线程执行，完成后 PostMessage 通知 UI
// ═══════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include "scanner.h"
#include "resource.h"
#include <winsock2.h>   // ← 必须第一个
#include <ws2tcpip.h>
#include <windows.h>    // ← 在 winsock2 之后
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <map>
#include <set>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

// ───────────────────────────────────────────────────────────
//  PID → 进程名 映射
// ───────────────────────────────────────────────────────────
std::map<DWORD, std::wstring> BuildPidMap() {
    std::map<DWORD, std::wstring> m;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return m;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            m[pe.th32ProcessID] = pe.szExeFile;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return m;
}

std::wstring PidToName(const std::map<DWORD, std::wstring>& pidMap, DWORD pid) {
    if (pid == 0) return L"System Idle";
    if (pid == 4) return L"System";
    auto it = pidMap.find(pid);
    return (it != pidMap.end()) ? it->second : L"Unknown";
}

// ───────────────────────────────────────────────────────────
//  核心扫描
// ───────────────────────────────────────────────────────────
std::vector<ActivePort> ScanPorts() {
    std::vector<ActivePort> results;
    std::set<std::pair<int, std::wstring>> seen;   // (port, proto) 去重键

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    auto pidMap = BuildPidMap();

    // 添加一个端口记录；若已存在则跳过
    auto addPort = [&](int port, const std::wstring& proto,
                       const std::wstring& state, DWORD pid)
    {
        auto key = std::make_pair(port, proto);
        if (seen.count(key)) return;
        seen.insert(key);

        ActivePort ap;
        ap.port        = port;
        ap.proto       = proto;
        ap.state       = state;
        ap.pid         = pid;
        ap.processName = PidToName(pidMap, pid);
        ap.info        = FindPortInfo(port, proto);
        results.push_back(ap);
    };

    // ── TCP IPv4 LISTEN ──────────────────────────────────
    {
        DWORD sz = 0;
        GetExtendedTcpTable(nullptr, &sz, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
        std::vector<BYTE> buf(sz);
        if (GetExtendedTcpTable(buf.data(), &sz, TRUE, AF_INET,
                                TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
            auto* tbl = reinterpret_cast<MIB_TCPTABLE_OWNER_PID*>(buf.data());
            for (DWORD i = 0; i < tbl->dwNumEntries; ++i) {
                const auto& row = tbl->table[i];
                if (row.dwState != MIB_TCP_STATE_LISTEN) continue;
                addPort(ntohs((u_short)row.dwLocalPort),
                        L"TCP", L"LISTEN", row.dwOwningPid);
            }
        }
    }

    // ── TCP IPv6 LISTEN（去重后补充 v6-only 端口）────────
    {
        DWORD sz = 0;
        GetExtendedTcpTable(nullptr, &sz, TRUE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0);
        std::vector<BYTE> buf(sz);
        if (GetExtendedTcpTable(buf.data(), &sz, TRUE, AF_INET6,
                                TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
            auto* tbl = reinterpret_cast<MIB_TCP6TABLE_OWNER_PID*>(buf.data());
            for (DWORD i = 0; i < tbl->dwNumEntries; ++i) {
                const auto& row = tbl->table[i];
                if (row.dwState != MIB_TCP_STATE_LISTEN) continue;
                addPort(ntohs((u_short)row.dwLocalPort),
                        L"TCP", L"LISTEN(v6)", row.dwOwningPid);
            }
        }
    }

    // ── UDP IPv4 ─────────────────────────────────────────
    {
        DWORD sz = 0;
        GetExtendedUdpTable(nullptr, &sz, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0);
        std::vector<BYTE> buf(sz);
        if (GetExtendedUdpTable(buf.data(), &sz, TRUE, AF_INET,
                                UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
            auto* tbl = reinterpret_cast<MIB_UDPTABLE_OWNER_PID*>(buf.data());
            for (DWORD i = 0; i < tbl->dwNumEntries; ++i) {
                const auto& row = tbl->table[i];
                addPort(ntohs((u_short)row.dwLocalPort),
                        L"UDP", L"OPEN", row.dwOwningPid);
            }
        }
    }

    WSACleanup();

    std::sort(results.begin(), results.end(),
              [](const ActivePort& a, const ActivePort& b) {
                  return a.port < b.port;
              });
    return results;
}

// ───────────────────────────────────────────────────────────
//  工作线程入口
// ───────────────────────────────────────────────────────────
struct ScanThreadParam { HWND hWnd; };

static DWORD WINAPI ScanThread(LPVOID pv) {
    auto* param = reinterpret_cast<ScanThreadParam*>(pv);
    HWND  hWnd  = param->hWnd;
    delete param;

    auto* res = new std::vector<ActivePort>(ScanPorts());
    PostMessageW(hWnd, WM_SCAN_DONE, 0, reinterpret_cast<LPARAM>(res));
    return 0;
}

void StartScanAsync(HWND hNotifyWnd) {
    auto* p = new ScanThreadParam{ hNotifyWnd };
    HANDLE h = CreateThread(nullptr, 0, ScanThread, p, 0, nullptr);
    if (h) CloseHandle(h);
    else   delete p;
}
