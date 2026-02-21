#pragma once

#include "ELogType.hpp"

#include <string>
#include <cstdint>

struct SLogEntry {
    ELogType m_Type;
    uint64_t m_Timestamp;
    std::string m_Message;
};
