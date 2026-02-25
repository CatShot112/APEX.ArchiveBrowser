#include "Utils.hpp"

#include <chrono>

namespace Utils {
    uint64_t GetCurrentTimeMs() {
        auto now = std::chrono::system_clock::now();
        auto epoch = now.time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

        return ms.count();
    }

    /*std::string UnixToDate(uint64_t timestamp) {
        std::chrono::system_clock::time_point tp{ std::chrono::milliseconds(timestamp) };
        std::string result = std::format("{:%d.%m.%Y", std::chrono::floor<std::chrono::milliseconds>(tp));

        return result;
    }
    std::string UnixToTime(uint64_t timestamp) {
        std::chrono::system_clock::time_point tp{ std::chrono::milliseconds(timestamp) };
        std::string result = std::format("{:%H:%M:%S}", std::chrono::floor<std::chrono::milliseconds>(tp));

        return result;
    }*/
    std::string UnixToDateTime(uint64_t timestamp) {
        std::chrono::system_clock::time_point tp{ std::chrono::milliseconds(timestamp) };
        std::string result = std::format("{:%d.%m.%Y %H:%M:%S}", std::chrono::floor<std::chrono::milliseconds>(tp));

        return result;
    }

    std::string BytesToHuman(uint64_t bytes) {
        static const char* suffix[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB" };

        float fBytes = static_cast<float>(bytes);

        uint32_t index = 0;
        auto size = sizeof(suffix) / (sizeof(suffix[0]));

        while (fBytes >= 1024.00f && index < (size - 1)) {
            index++;

            fBytes /= 1024.00f;
        }

        return std::format("{:7.2f} {}", fBytes, suffix[index]);
    }

    std::string BytesToHuman2(uint64_t bytes1, uint64_t bytes2) {
        static const char* suffix[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB" };

        float fBytes1 = static_cast<float>(bytes1);
        float fBytes2 = static_cast<float>(bytes2);

        uint32_t index = 0;
        auto size = sizeof(suffix) / sizeof(suffix[0]);

        if (bytes1 > bytes2)
            std::swap(fBytes1, fBytes2);

        while (fBytes2 >= 1024.00f && index < (size - 1)) {
            index++;

            fBytes1 /= 1024.00f;
            fBytes2 /= 1024.00f;
        }

        //uint64_t cnt = std::to_string(fBytes2).length();

        //return std::vformat("{:{}.2f}/{:{}.2f} {}", std::make_format_args(fBytes1, cnt, fBytes2, cnt, suffix[index]));
        return std::format("{:.2f}/{:.2f} {}", fBytes1, fBytes2, suffix[index]);
    }
}
