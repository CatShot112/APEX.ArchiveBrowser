#include "CLog.hpp"

#include <cstdarg>

std::vector<SLogEntry> CLog::m_Entries;

CLog::CLog() {}
CLog::~CLog() {}

static std::string Helper(const char* fmt, va_list args) {
    std::string result;

    size_t len = _vscprintf(fmt, args);
    if (len < 0)
        return "";

    if (len > 0) {
        result.resize(len);
        vsnprintf_s(result.data(), len + 1, _TRUNCATE, fmt, args);
    }

    return result;
}

void CLog::INF(const char* fmt, ...) {
    va_list args{};
    va_start(args, fmt);
    auto result = Helper(fmt, args);
    va_end(args);

    m_Entries.push_back({ ELogType::INFO, Utils::GetCurrentTimeMs(), result });
}
void CLog::WAR(const char* fmt, ...) {
    va_list args{};
    va_start(args, fmt);
    auto result = Helper(fmt, args);
    va_end(args);

    m_Entries.push_back({ ELogType::WARNING, Utils::GetCurrentTimeMs(), result });
}
void CLog::ERR(const char* fmt, ...) {
    va_list args{};
    va_start(args, fmt);
    auto result = Helper(fmt, args);
    va_end(args);

    m_Entries.push_back({ ELogType::ERROR, Utils::GetCurrentTimeMs(), result });
}

void CLog::Clear() {
    m_Entries.clear();
}

std::string CLog::TypeToStr(ELogType type) {
    if (type == ELogType::INFO)
        return "INF";
    else if (type == ELogType::WARNING)
        return "WAR";
    else
        return "ERR";

    return "UNK";
}

const std::vector<SLogEntry>& CLog::GetEntries() {
    return m_Entries;
}
