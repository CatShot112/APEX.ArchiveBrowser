#pragma once

#include <string>
#include <cstdint>

struct SVFSDir;

struct SVFSFile {
    SVFSDir* m_Root;

    std::string m_Name;
    std::string m_ArchivePath;

    int32_t m_FileType;
    int32_t m_FileSize;

    int32_t m_ArchiveId; // Not really needed because of m_ArchivePath.
    int32_t m_ArchiveOffset;

    bool m_IsCompressed;

    SVFSFile(std::string name, std::string archivePath, SVFSDir* root, int32_t fileType, int32_t fileSize, int32_t archiveId, int32_t archiveOffset, bool isCompressed) {
        this->m_Root = root;

        this->m_Name = name;
        this->m_ArchivePath = archivePath;

        this->m_FileType = fileType;
        this->m_FileSize = fileSize;

        this->m_ArchiveId = archiveId;
        this->m_ArchiveOffset = archiveOffset;

        this->m_IsCompressed = isCompressed;
    }

    ~SVFSFile() {

    }
};
