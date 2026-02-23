#include "CLog.hpp"

#include <cstdarg>

std::vector<SLogEntry> CLog::m_Entries;

CLog::CLog() {}
CLog::~CLog() {}

static std::string Helper(const char* fmt, va_list args) {
    std::string result;

    va_list copy{};
    va_copy(copy, args);
    size_t len = vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);

    if (len < 0)
        return "";

    if (len > 0) {
        result.resize(len);
        vsnprintf(result.data(), len + 1, fmt, args);
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
