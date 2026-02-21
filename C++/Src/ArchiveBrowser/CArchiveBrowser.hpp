#pragma once

#include "SVFSDir.hpp"

#include <mutex>

struct SDL_Window;

class CArchiveBrowser {
    SVFSDir m_RootDir;
    SVFSDir m_HashedDir;

    std::mutex m_Mutex;
    bool m_OpenPopup;
    bool m_ShouldCancel;

    bool m_ShowResolver;

    bool m_OpenedFile;
    bool m_OpenedFolder;
    std::string m_OpenedPath;

    std::vector<uint32_t> m_DuplicateHashes;

    void Thread_ExtractFile(SVFSFile* pFile);
    void Thread_ExtractFolder(SVFSDir* pDir);

protected:
    CArchiveBrowser();
    ~CArchiveBrowser();

    CArchiveBrowser(CArchiveBrowser&) = delete;
    CArchiveBrowser(const CArchiveBrowser&) = delete;

    CArchiveBrowser operator =(CArchiveBrowser&) = delete;
    CArchiveBrowser operator =(const CArchiveBrowser&) = delete;

public:
    static CArchiveBrowser& Instance();

    bool Initialize();
    void Cleanup();

    void Draw();
    void DrawResolver();

private:
    void DrawFile(SVFSFile* file);
    void DrawDirectory(SVFSDir* directory);

    void AddVPathToNode(SVFSDir& node, std::string vPath, std::string archivePath, int32_t fileType = -1, int32_t fileSize = 0, int32_t archiveId = -1, int32_t archiveOffset = -1);

public:
    void OpenFile(std::string path, bool fromFolder = false, bool reload = false);
    void OpenFolder(std::string path, bool reload = false);
    void Close();

    void ExtractFile(SVFSFile* pFile);
    void ExtractFolder(SVFSDir* pDir);

    void ResolveDuplicate(uint32_t hash, std::string vPath);

    const bool IsOpenedAny();
    const bool IsOpenedFile();
    const bool IsOpenedFolder();

    const bool HasDuplicatedHashes();

    const std::string GetLastOpenedPath();

    std::string ConstructVPath(SVFSFile* pFile);
    std::string ConstructVPath(SVFSDir* pDir);

    void PrintDirs(SVFSDir& dir);
};
