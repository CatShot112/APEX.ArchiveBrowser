#pragma once

#include "SVFSFile.hpp"
#include "../Logging/CLog.hpp"

#include <vector>

struct SVFSDir {
    SVFSDir* m_Root;

    std::string m_Name;

    std::vector<SVFSDir*> m_Directories;
    std::vector<SVFSFile*> m_Files;

    SVFSDir() {
        this->m_Root = nullptr;

        this->m_Name = "<UNKNOWN>";
    }
    SVFSDir(std::string name, SVFSDir* root = nullptr) {
        this->m_Root = root;

        this->m_Name = name;
    }

    SVFSDir* AddDirectory(std::string name, SVFSDir* root = nullptr) {
        // Cannot have multiple directories with the same name.
        for (auto dir : m_Directories) {
            if (!dir->m_Name.compare(name)) {
                // TODO: Walk up the root nodes and construct full VPath.
                //CLog::WAR("Directory already exists! VPath: %s, Directory: %s", name.c_str(), dir.m_Name.c_str());
                return dir;
            }
        }

        this->m_Directories.push_back(new SVFSDir(name, root));

        return this->m_Directories.back();
    }

    SVFSFile* AddFile(std::string name, std::string archivePath, int32_t fileType, int32_t fileSize, int32_t archiveId = -1, int32_t archiveOffset = -1, bool isCompressed = false) {
        // Cannot have multiple files with the same name.
        for (auto file : m_Files) {
            if (!file->m_Name.compare(name)) {
                // TODO: Walk up the root nodes and construct full VPath.
                //CLog::WAR("File already exists! VPath: %s, File: %s", name.c_str(), file.m_Name.c_str());
                return file;
            }
        }

        this->m_Files.push_back(new SVFSFile(name, archivePath, this, fileType, fileSize, archiveId, archiveOffset, isCompressed));
        return this->m_Files.back();
    }

    void RemoveFile(std::string name) {
        auto it = GetFileIt(name);

        if (it != m_Files.end()) {
            delete (*it);
            m_Files.erase(it);
        }
    }

    void RemoveDirectory(std::string name) {
        auto it = GetDirectoryIt(name);

        if (it != m_Directories.end()) {
            // Clear files in directory
            for (auto fIt = (*it)->m_Files.begin(); fIt != (*it)->m_Files.end(); ++fIt) {
                delete (*fIt);
                (*it)->m_Files.erase(fIt);
            }

            (*it)->m_Files.clear();
            m_Directories.erase(it);
        }
    }

    uint32_t GetFileCount(bool recursive = true) {
        auto count = m_Files.size();

        if (recursive) {
            for (auto dir : m_Directories)
                count += dir->GetFileCount();
        }

        return static_cast<uint32_t>(count);
    }

    uint32_t GetFolderCount(bool recursive = true) {
        auto count = m_Directories.size();

        if (recursive) {
            for (auto dir : m_Directories)
                count += dir->GetFolderCount();
        }

        return static_cast<uint32_t>(count);
    }

    std::vector<SVFSFile*>::iterator GetFileIt(std::string name) {
        return std::find_if(m_Files.begin(), m_Files.end(), [&n = name](const SVFSFile* file) { return !file->m_Name.compare(n); });
    }

    std::vector<SVFSDir*>::iterator GetDirectoryIt(std::string name) {
        return std::find_if(m_Directories.begin(), m_Directories.end(), [&n = name](const SVFSDir* dir) {return !dir->m_Name.compare(n); });
    }
};
