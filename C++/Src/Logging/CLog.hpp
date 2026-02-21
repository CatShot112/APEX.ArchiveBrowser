#pragma once

#include "SLogEntry.hpp"
#include "../Utils/Utils.hpp"

#include <vector>
#include <chrono>

class CLog {
    static std::vector<SLogEntry> m_Entries;

protected:
    CLog();
    ~CLog();

    CLog(CLog&) = delete;
    CLog(const CLog&) = delete;

    CLog operator =(CLog&) = delete;
    CLog operator =(const CLog&) = delete;

public:
    static void INF(const char* fmt, ...);
    static void WAR(const char* fmt, ...);
    static void ERR(const char* fmt, ...);

    static void Clear();

    static std::string TypeToStr(ELogType type);

    static const std::vector<SLogEntry>& GetEntries();
};
