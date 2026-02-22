#pragma once

#include <cstdint>
#include <string>

namespace Utils {
    uint64_t GetCurrentTimeMs();
    
    /*std::string UnixToDate(uint64_t timestamp);
    std::string UnixToTime(uint64_t timestamp);*/
    std::string UnixToDateTime(uint64_t timestamp);

    std::string BytesToHuman(uint64_t bytes);
}
