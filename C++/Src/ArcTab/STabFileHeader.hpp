#pragma once

#include <cstdint>

struct STabFileHeader {
    int8_t m_Tag[4];
    uint16_t m_Version;
    uint16_t m_EndianCheck;
    int32_t m_AlignState;
};
