#pragma once

#include <string>
#include <vector>

enum class EFileType {
    AAF,
    ADF,
    ADF0,
    AVTX,
    BINK_BIK,
    BINK_KB2,
    BM6,
    BM8,
    CFX,
    DDS,
    FSB5C,
    GFX,
    GT0C,
    H2014,
    HKX,
    MDI,
    OGG,
    PFX,
    RBMDL,
    RIFF,
    RTPC,
    SARC,
    TAG0,
    Unknown = -1
};

namespace FileType {
    std::string ToString(EFileType fileType) {
        static const char* fileTypeArr[] = {
            "AAF",
            "ADF",
            "ADF0",
            "AVTX",
            "BINK_BIK",
            "BINK_KB2",
            "BM6",
            "BM8",
            "CFX",
            "DDS",
            "FSB5C",
            "GFX",
            "GT0C",
            "H2014",
            "HKX",
            "MDI",
            "OGG",
            "PFX",
            "RBMDL",
            "RIFF",
            "RTPC",
            "SARC",
            "TAG0"
        };

        if (fileType == EFileType::Unknown)
            return "Unknown";

        return fileTypeArr[int(fileType)];
    }

    EFileType BufToFileType(std::string& buf) {
        if (!memcmp(buf.data(), "\x41\x41\x46", 3))
            return EFileType::AAF;
        else if (!memcmp(buf.data(), "\x20\x46\x44\x41", 4))
            return EFileType::ADF;
        else if (!memcmp(buf.data(), "\x00\x46\x44\x41", 4))
            return EFileType::ADF0;
        else if (!memcmp(buf.data(), "\x41\x56\x54\x58", 4))
            return EFileType::AVTX;
        else if (!memcmp(buf.data(), "\x42\x49\x4B", 3))
            return EFileType::BINK_BIK;
        else if (!memcmp(buf.data(), "\x4B\x42\x32", 3))
            return EFileType::BINK_KB2;
        else if (!memcmp(buf.data(), "\x42\x4D\x36", 3))
            return EFileType::BM6;
        else if (!memcmp(buf.data(), "\x42\x4D\x38", 3))
            return EFileType::BM8;
        else if (!memcmp(buf.data(), "\x43\x46\x58", 3))
            return EFileType::CFX;
        else if (!memcmp(buf.data(), "\x44\x44\x53\x20", 4))
            return EFileType::DDS;
        else if (!memcmp(buf.data(), "\x46\x53\x42\x35", 4))
            return EFileType::FSB5C;
        else if (!memcmp(buf.data(), "\x47\x46\x58", 3))
            return EFileType::GFX;
        else if (!memcmp(buf.data(), "\x47\x54\x30\x43", 4))
            return EFileType::GT0C;
        else if (!memcmp(buf.data(), "\x57\xE0\xE0\x57\x10\xC0\xC0\x10", 8))
            return EFileType::H2014;
        else if (!memcmp(buf.data(), "\x57\xE0\xE0\x57", 4))
            return EFileType::HKX;
        else if (!memcmp(buf.data(), "\x4D\x44\x49\x00", 4))
            return EFileType::MDI;
        else if (!memcmp(buf.data(), "\x4F\x67\x67\x53", 4))
            return EFileType::OGG;
        else if (!memcmp(buf.data(), "\x50\x46\x58\x00", 4))
            return EFileType::PFX;
        else if (!memcmp(buf.data(), "\x05\x00\x00\x00\x52\x42\x4D\x44\x4C", 9))
            return EFileType::RBMDL;
        else if (!memcmp(buf.data(), "\x52\x49\x46\x46", 4))
            return EFileType::RIFF;
        else if (!memcmp(buf.data(), "\x52\x54\x50\x43", 4))
            return EFileType::RTPC;
        else if (!memcmp(buf.data(), "\x53\x41\x52\x43", 4))
            return EFileType::SARC;
        else if (!memcmp(buf.data(), "\x54\x41\x47\x30", 4))
            return EFileType::TAG0;

        return EFileType::Unknown;
    }
}
