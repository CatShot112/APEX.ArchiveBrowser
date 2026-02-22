#include "CArchiveBrowser.hpp"
#include "../Database/CDatabaseManager.hpp"

#include "../ArcTab/STabFileEntry.hpp"
#include "../ArcTab/STabFileHeader.hpp"

#include <SDL3/SDL_filesystem.h>

#include <imgui/imgui.h>
#include <filesystem>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

CArchiveBrowser::CArchiveBrowser() {
    m_RootDir = SVFSDir("/");
    m_HashedDir = SVFSDir("Not dehashed");

    m_OpenPopup = false;
    m_ShouldCancel = false;

    m_ShowResolver = false;

    m_OpenedFile = false;
    m_OpenedFolder = false;
    m_OpenedPath = "";
}
CArchiveBrowser::~CArchiveBrowser() {

}

static void Callback_ExtractTo(void* userData, const char* const* fileList, int filter) {
    if (!fileList) {
        CLog::ERR(SDL_GetError());
        return;
    }
    else if (!*fileList) {
        CLog::WAR("User did not select any file to save (cancelled).");
        return;
    }

    if (*fileList) {

    }
}

uint64_t CalcDirSize(SVFSDir* pDir) {
    uint64_t size = 0;

    for (auto& file : pDir->m_Files)
        size += file->m_FileSize;

    for (auto dir : pDir->m_Directories) {
        if (!dir->m_Files.empty())
            size += CalcDirSize(dir);
    }

    return size;
}

CArchiveBrowser& CArchiveBrowser::Instance() {
    static CArchiveBrowser instance;
    return instance;
}

void CArchiveBrowser::Thread_ExtractFile(SVFSFile* pFile) {
    std::string pathBase = SDL_GetBasePath();
    std::string pathDest = pathBase + "Extracted/";

    auto vPath = ConstructVPath(pFile);

    pathDest += vPath;
    fs::path pathFull(pathDest);

    if (!fs::exists(pathFull))
        fs::create_directories(pathFull.parent_path());

    std::ifstream iFile(pFile->m_ArchivePath, std::ios::binary);
    if (!iFile.is_open()) {
        CLog::ERR("Failed to open archive! Path: %s", pFile->m_ArchivePath.c_str());
        return;
    }

    std::ofstream oFile(pathDest, std::ios::binary);
    if (!oFile.is_open()) {
        CLog::ERR("Failed to open file for writing! Path: %s", pathDest.c_str());

        iFile.close();
        return;
    }

    std::string data;
    data.resize(pFile->m_FileSize);

    iFile.seekg(pFile->m_ArchiveOffset);
    iFile.read(data.data(), pFile->m_FileSize);
    oFile.write(data.data(), pFile->m_FileSize);

    iFile.close();
    oFile.close();

    {
        std::lock_guard lock(m_Mutex);

        CLog::INF("Extracted: %s, To: %s", pFile->m_Name.c_str(), pathDest.c_str());
    }
}
void CArchiveBrowser::Thread_ExtractFolder(SVFSDir* pDir) {
    for (auto file : pDir->m_Files)
        Thread_ExtractFile(file);

    for (auto dir : pDir->m_Directories) {
        if (!dir->m_Directories.empty())
            Thread_ExtractFolder(dir);
    }

    // If log then mutex?
    //CLog::INF("Extracted: %s, To: %s", pDir->m_Name.c_str(), "Extracted/");
}

bool CArchiveBrowser::Initialize() {
    m_RootDir.m_Files.reserve(1024);
    m_RootDir.m_Directories.reserve(512);

    m_HashedDir.m_Files.reserve(102400);

    return true;
}

void CArchiveBrowser::Cleanup() {

}

void CArchiveBrowser::Draw() {
    ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    //ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver | ImGuiCond_Appearing, ImVec2(0.50f, 0.50f));

    if (!ImGui::Begin("Archive Browser", nullptr, /*ImGuiWindowFlags_NoBringToFrontOnFocus | */ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    if (!ImGui::BeginTable("Archive Browser Table", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::EndTable();
        return;
    }

    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Size");
    ImGui::TableSetupColumn("Type");
    ImGui::TableHeadersRow();

    DrawDirectory(&m_RootDir);
    DrawDirectory(&m_HashedDir);

    ImGui::EndTable();
    ImGui::End();
}
void CArchiveBrowser::DrawResolver() {
    ImGui::SetNextWindowSize(ImVec2(600, 350), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Conflict Resolver", &m_ShowResolver, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("Here shall be detailed instructions on how to use this.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto& vPaths = CDatabaseManager::Instance().GetVPaths();

    for (auto hash : m_DuplicateHashes) {
        auto its = vPaths.equal_range(hash);
        auto sHash = std::format("{:x}", hash);

        ImGui::Text("Hash: %x", hash);
        ImGui::SameLine();
        if (ImGui::Button(std::string("Extract##" + sHash).c_str())) {
            for (auto file : m_HashedDir.m_Files) {
                if (!file->m_Name.compare(sHash)) {
                    ExtractFile(file);
                }
            }
        }

        for (auto it = its.first; it != its.second; ++it) {
            auto& vPath = (*it).second;

            ImGui::Text(vPath.c_str());
            ImGui::SameLine();

            if (ImGui::Button(std::string("Resolve##" + vPath).c_str())) {
                ResolveDuplicate(hash, vPath);
                break;
            }
        }

        ImGui::Separator();
    }

    ImGui::End();
}

void CArchiveBrowser::DrawFile(SVFSFile* file) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImGui::TreeNodeEx(file->m_Name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_None | ImGuiTreeNodeFlags_NoTreePushOnOpen);

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Button("Extract")) {
            // TODO: Popup with cancel
            //std::thread thread(&CArchiveBrowser::ExtractFile, this, file);
            //thread.detach();

            ExtractFile(file);

            /*std::string pathBase = SDL_GetBasePath();
            std::string pathDestination = pathBase + "Extracted/";

            auto vPath = ConstructVPath(file);

            pathDestination += vPath;
            fs::path fullPath(pathDestination);

            if (!fs::exists(pathDestination))
                fs::create_directories(fullPath.parent_path());

            std::ifstream iFile(file->m_ArchivePath, std::ios::binary);
            if (iFile.is_open()) {
                std::ofstream oFile(pathDestination, std::ios::binary);
                if (oFile.is_open()) {
                    std::string data;
                    data.resize(file->m_FileSize);

                    iFile.seekg(file->m_ArchiveOffset);
                    iFile.read(data.data(), file->m_FileSize);
                    oFile.write(data.data(), file->m_FileSize);

                    iFile.close();
                    oFile.close();

                    CLog::INF("Extracted: %s, To: %s", file->m_Name.c_str(), pathDestination.c_str());
                }
                else {
                    CLog::ERR("(Extract): Failed to open destination file! Path: %s", pathDestination.c_str());
                }
            }
            else {
                CLog::ERR("(Extract): Failed to open archive file! Path: %s", file->m_ArchivePath.c_str());
            }*/

            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Extract to..")) {
            // TODO: Open file dialog

            //CLog::INF("Extracting: %s, Destination: %s", file->m_Name.c_str(), "Extracted/");
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::TableNextColumn();
    //ImGui::Text("%d", file->m_FileSize);
    ImGui::Text("%s", Utils::BytesToHuman(file->m_FileSize).c_str());

    ImGui::TableNextColumn();
    //ImGui::Text("%s", FileTypeToString(file.m_FileType).c_str());
    ImGui::Text("%s", "Unknown");
}
void CArchiveBrowser::DrawDirectory(SVFSDir* directory) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    bool isOpen = ImGui::TreeNodeEx(directory->m_Name.c_str());

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Button("Extract")) {
            // TODO: Popup with cancel
            //std::thread thread(&CArchiveBrowser::ExtractFolder, this, directory);
            //thread.detach();

            ExtractFolder(directory);

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::TableNextColumn();
    //ImGui::TextDisabled("---"); // Size
    ImGui::TextDisabled(Utils::BytesToHuman(CalcDirSize(directory)).c_str());
    ImGui::TableNextColumn();
    ImGui::TextDisabled("---"); // Type

    if (!isOpen)
        return;

    for (auto dir : directory->m_Directories)
        DrawDirectory(dir);

    for (auto file : directory->m_Files)
        DrawFile(file);

    ImGui::TreePop();
}

void CArchiveBrowser::AddVPathToNode(SVFSDir& node, std::string vPath, std::string archivePath, int32_t fileType, int32_t fileSize, int32_t archiveId, int32_t archiveOffset) {
    std::regex del("/");
    std::sregex_token_iterator iter(vPath.begin(), vPath.end(), del, -1);
    std::sregex_token_iterator itEnd;

    SVFSDir* dir = &node;

    if (std::next(iter) == itEnd) {
        dir->AddFile((*iter).str(), archivePath, fileType, fileSize, archiveId, archiveOffset);
        return;
    }

    for (; iter != itEnd; ++iter) {
        if (std::next(iter) == itEnd) {
            dir->AddFile((*iter).str(), archivePath, fileType, fileSize, archiveId, archiveOffset);
            return;
        }

        dir = dir->AddDirectory((*iter).str(), dir);
    }
}

void CArchiveBrowser::OpenFile(std::string filePath, bool fromFolder, bool reload) {
    auto& db = CDatabaseManager::Instance();
    if (!fromFolder && db.GetVPaths().size() <= 0)
        CLog::WAR("Database is not initialized in memory!");

    if (!fromFolder && (m_OpenedFile || m_OpenedFolder))
        Close();

    if (!fromFolder && reload)
        filePath = m_OpenedPath;

    if (fs::is_directory(filePath)) {
        CLog::ERR("Passed folder to OpenFile! Path: %s", filePath.c_str());
        return;
    }

    fs::path path(filePath);

    std::ifstream arcFile(path.replace_extension(".arc"), std::ios::binary);
    auto arcSize = fs::file_size(path);

    if (!arcFile.is_open()) {
        CLog::ERR("Failed to open .arc file! Path: %s", path.c_str());
        return;
    }

    std::ifstream tabFile(path.replace_extension(".tab"), std::ios::binary);
    auto tabSize = fs::file_size(path);

    if (!tabFile.is_open()) {
        CLog::ERR("Failed to open .tab file! Path: %s", path.c_str());

        arcFile.close();
        return;
    }

    std::regex rgx("\\d+");
    std::smatch match;

    std::string str = path.string();
    std::reverse(str.begin(), str.end());

    uint32_t archiveId{};

    if (std::regex_search(str, match, rgx))
        archiveId = std::atoi(std::string(match[0]).c_str());

    STabFileHeader header{};
    tabFile.read((char*)&header, sizeof(header));

    while (!tabFile.eof()) {
        STabFileEntry entry{};
        tabFile.read((char*)&entry, sizeof(entry));

        if (entry.m_Size <= 8)
            continue;

        bool dehashed = false;
        std::stringstream dehashedName{};
        auto& vPaths = CDatabaseManager::Instance().GetVPaths();
        auto count = vPaths.count(entry.m_Hash);

        // TODO: Database dehashed vpaths
        if (count <= 0) {
            dehashed = false;
            dehashedName << std::hex << entry.m_Hash;
        }
        else if (count > 1) {
            auto it = std::find(m_DuplicateHashes.begin(), m_DuplicateHashes.end(), entry.m_Hash);

            if (it == m_DuplicateHashes.end())
                m_DuplicateHashes.push_back(entry.m_Hash);

            CLog::WAR("Duplicate hash found, need to resolve! Hash: %x", entry.m_Hash);

            dehashed = false;
            dehashedName << std::hex << entry.m_Hash;
        }
        else {
            dehashed = true;
            dehashedName << vPaths.find(entry.m_Hash)->second;
        }

        // TODO: Store fileType by buffer in database once and only do fetching here.

        /*std::string buffer;
        buffer.resize(32);

        arcFile.seekg(entry.m_Offset);
        arcFile.read(buffer.data(), 32);

        int32_t fileType = BufferToFileType(buffer);*/

        if (dehashed)
            AddVPathToNode(m_RootDir, dehashedName.str(), path.replace_extension(".arc").string(), -1, entry.m_Size, archiveId, entry.m_Offset);
        else
            AddVPathToNode(m_HashedDir, dehashedName.str(), path.replace_extension(".arc").string() , -1, entry.m_Size, archiveId, entry.m_Offset);
    }

    tabFile.close();
    arcFile.close();

    if (!fromFolder) {
        m_HashedDir.m_Name = m_HashedDir.m_Name + " (" + std::to_string(m_HashedDir.m_Files.size()) + ")";

        m_OpenedFile = true;
        m_OpenedFolder = false;
        m_OpenedPath = filePath;

        //PrintDirs(m_RootDir);

        CLog::INF("Done opening file!");
    }
}
void CArchiveBrowser::OpenFolder(std::string folderPath, bool reload) {
    auto& db = CDatabaseManager::Instance();
    if (db.GetVPaths().size() <= 0)
        CLog::WAR("Database is not initialized in memory!");

    if (m_OpenedFile || m_OpenedFolder)
        Close();

    if (reload)
        folderPath = m_OpenedPath;

    if (!fs::is_directory(folderPath)) {
        CLog::ERR("Passed file to OpenFolder! Path: %s", folderPath.c_str());
        return;
    }

    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (entry.is_directory() || entry.path().extension().compare(".arc"))
            continue;

        OpenFile(entry.path().string(), true);
    }

    m_HashedDir.m_Name = m_HashedDir.m_Name + " (" + std::to_string(m_HashedDir.m_Files.size()) + ")";

    m_OpenedFile = false;
    m_OpenedFolder = true;
    m_OpenedPath = folderPath;

    CLog::INF("Done opening folder!");
}
void CArchiveBrowser::Close() {
    m_OpenedFile = false;
    m_OpenedFolder = false;

    m_DuplicateHashes.clear();

    m_RootDir.m_Files.clear();
    m_RootDir.m_Directories.clear();

    m_HashedDir.m_Files.clear();
    m_HashedDir.m_Directories.clear();
    m_HashedDir.m_Name = "Not dehashed";
}

void CArchiveBrowser::ExtractFile(SVFSFile* pFile) {
    m_OpenPopup = true;

    std::thread thread(&CArchiveBrowser::Thread_ExtractFile, this, pFile);
    thread.detach();

}
void CArchiveBrowser::ExtractFolder(SVFSDir* pDir) {
    // TODO: Calculate total file count and total size.
    auto size = CalcDirSize(pDir);

    CLog::INF("Directory size: %s (%lld)", Utils::BytesToHuman(size).c_str(), size);
    
    m_OpenPopup = true;

    std::thread thread(&CArchiveBrowser::Thread_ExtractFolder, this, pDir);
    thread.detach();
}

void CArchiveBrowser::ResolveDuplicate(uint32_t hash, std::string vPath) {
    auto fileName = std::format("{:x}", hash);
    auto fileIt = m_HashedDir.GetFileIt(fileName);

    if (fileIt == m_HashedDir.m_Files.end()) {
        CLog::ERR("No such file found in m_HashedDir! Name: %s", fileName.c_str());
        return;
    }

    auto pFile = (*fileIt);

    AddVPathToNode(m_RootDir, vPath, pFile->m_ArchivePath, pFile->m_FileType, pFile->m_FileSize, pFile->m_ArchiveId, pFile->m_ArchiveOffset);
    
    auto it1 = std::find(m_DuplicateHashes.begin(), m_DuplicateHashes.end(), hash);
    if (it1 != m_DuplicateHashes.end())
        m_DuplicateHashes.erase(it1);

    // TODO: Move to separate functions
    m_HashedDir.RemoveFile(fileName);

    m_HashedDir.m_Name = m_HashedDir.m_Name + " (" + std::to_string(m_HashedDir.m_Files.size()) + ")";
}

const bool CArchiveBrowser::IsOpenedAny() { return m_OpenedFile || m_OpenedFolder; }
const bool CArchiveBrowser::IsOpenedFile() { return m_OpenedFile; }
const bool CArchiveBrowser::IsOpenedFolder() { return m_OpenedFolder; }

const bool CArchiveBrowser::HasDuplicatedHashes() { return m_DuplicateHashes.size() > 0; }

const std::string CArchiveBrowser::GetLastOpenedPath() { return m_OpenedPath; }

std::string CArchiveBrowser::ConstructVPath(SVFSFile* pFile) {
    auto ptr = pFile->m_Root;
    std::string vPath = pFile->m_Name;

    while (ptr != nullptr) {
        if (ptr != &m_RootDir && ptr != &m_HashedDir)
            vPath.insert(0, ptr->m_Name + '/');

        ptr = ptr->m_Root;
    }

    return vPath;
}
std::string CArchiveBrowser::ConstructVPath(SVFSDir* pDir) {
    auto ptr = pDir->m_Root;
    std::string vPath = pDir->m_Name;

    while (ptr != nullptr) {
        if (ptr != &m_RootDir && ptr != &m_HashedDir)
            vPath.insert(0, ptr->m_Name + '/');

        ptr = ptr->m_Root;
    }

    return vPath;
}


void CArchiveBrowser::PrintDirs(SVFSDir& next = CArchiveBrowser::Instance().m_RootDir) {
    for (auto& dir : next.m_Directories) {
        //printf("Dir: %s | Prev Dir: %s\n", next.m_Name.c_str(), dir.m_Name.c_str());
        CLog::INF("Dir: %s | Prev Dir: %s", dir->m_Name.c_str(), next.m_Name.c_str());

        PrintDirs(*dir);
    }
}
