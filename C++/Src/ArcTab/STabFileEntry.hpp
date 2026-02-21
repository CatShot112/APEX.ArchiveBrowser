#pragma once

#include <cstdint>

struct STabFileEntry {
    uint32_t m_Hash;   // Hash of vPath.
    uint32_t m_Offset; // Offset in .arc file.
    uint32_t m_Size;   // File size.
};
