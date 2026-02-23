#include "CDatabaseManager.hpp"

#include "../Logging/CLog.hpp"

#include <thread>
#include <fstream>
#include <filesystem>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

#include <sqlite3/sqlite3.h>
#include <hashes/hashlittle.h>

namespace fs = std::filesystem;

CDatabaseManager::CDatabaseManager() {
    m_DBBase = nullptr;
    m_DBGame = nullptr;

    m_ProgressProps = 0.00f;
    m_ProgressVPaths = 0.00f;

    m_OpenPopup = false;
    m_Initialized = false;
    m_ShouldCancel = false;
}
CDatabaseManager::~CDatabaseManager() {

}

CDatabaseManager& CDatabaseManager::Instance() {
    static CDatabaseManager instance;
    return instance;
}

void CDatabaseManager::ClearProps() {
    m_Props.clear();
    m_VecProps.clear();
}
void CDatabaseManager::ClearVPaths() {
    m_VPaths.clear();
    m_VecVPaths.clear();
}

void CDatabaseManager::Thread_CreateDatabase() {
    if (sqlite3_open("Database_base.db", &m_DBBase)) {
        CLog::ERR("Error while opening database! Error: %s", sqlite3_errmsg(m_DBBase));
        return;
    }

    // Populated once (shared between games/game versions) (Database_base.db)
    const char* tableProps = { "CREATE TABLE IF NOT EXISTS props("
        "id INTEGER NOT NULL UNIQUE,"
        "hash32 INTEGER NOT NULL,"
        //"hash64 INTEGER NOT NULL," // MurmurHash3
        "prop TEXT NOT NULL,"
        "PRIMARY KEY(id AUTOINCREMENT));" };

    // 1. Shared for games/game versions. Can have duplicate hashes tho.
    // 2. Separate for each game type (theHunter, genz, etc.)
    //    There is still possibility for a collision.
    // Some vPaths can have the same hash32 (collisions).
    const char* tableVPaths = { "CREATE TABLE IF NOT EXISTS vPaths("
        "id INTEGER NOT NULL UNIQUE,"
        "hash32 INTEGER NOT NULL,"
        //"hash64 INTEGER NOT NULL," // MurmurHash3
        "vPath TEXT NOT NULL,"
        "PRIMARY KEY(id AUTOINCREMENT));" };

    const char* tableFiles = { "CREATE TABLE IF NOT EXISTS files("
        "id INTEGER NOT NULL UNIQUE,"
        "vPathId INTEGER,"
        "fileType INTEGER NOT NULL,"
        "sizeCompressed INTEGER NOT NULL,"
        "sizeUncompressed INTEGER NOT NULL,"
        "archivePath TEXT NOT NULL,"
        "archiveOffset INTEGER NOT NULL,"
        "PRIMARY KEY(id AUTOINCREMENT));" };

    const char* idx1 = "CREATE INDEX IF NOT EXISTS props_hash32 ON props (hash32);";
    const char* idx2 = "CREATE INDEX IF NOT EXISTS vPaths_hash32 ON props (hash32);";

    sqlite3_exec(m_DBBase, tableProps, nullptr, nullptr, nullptr);

    // Props
    {
        const char* stmtStr = "INSERT INTO props(hash32, prop) VALUES(?, ?);";
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_DBBase, stmtStr, -1, &stmt, nullptr);

        std::ifstream iFile("Props.txt");
        if (!iFile.is_open()) {
            CLog::ERR("Failed to open Props.txt!");

            sqlite3_finalize(stmt);
            sqlite3_close(m_DBBase);
            return;
        }

        size_t lineCurrent = 0;
        auto lineCnt = std::count(std::istreambuf_iterator<char>(iFile), std::istreambuf_iterator<char>(), '\n');

        iFile.clear();
        iFile.seekg(std::ifstream::beg);

        sqlite3_exec(m_DBBase, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

        std::string line;
        while (std::getline(iFile, line)) {
            lineCurrent++;

            if (line.empty() || line.size() <= 0 || line[0] == ' ')
                continue;

            uint32_t hash32 = hashlittle(line.c_str(), line.length(), 0);

            sqlite3_bind_int64(stmt, 1, hash32);
            sqlite3_bind_text(stmt, 2, line.c_str(), int(line.length()), SQLITE_STATIC);

            auto retVal = sqlite3_step(stmt);
            if (retVal != SQLITE_DONE) {
                CLog::ERR("sqlite3_step failed! Val: %d, Err: %s", retVal, sqlite3_errmsg(m_DBBase));
                CLog::ERR("hash32: %d, string: %s", hash32, line.c_str());
            }

            {
                std::lock_guard lock(m_Mutex);
                m_ProgressProps = (static_cast<float>(lineCurrent) / lineCnt);

                if (m_ShouldCancel) {
                    m_ShouldCancel = false;
                    iFile.close();

                    sqlite3_reset(stmt);
                    sqlite3_exec(m_DBBase, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
                    sqlite3_finalize(stmt);
                    sqlite3_close(m_DBBase);

                    CLog::WAR("User cancelled creating database!");
                    return;
                }
            }

            sqlite3_reset(stmt);
        }

        iFile.close();

        sqlite3_exec(m_DBBase, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
        sqlite3_finalize(stmt);
    }

    sqlite3_exec(m_DBBase, idx1, nullptr, nullptr, nullptr);
    sqlite3_exec(m_DBBase, tableVPaths, nullptr, nullptr, nullptr);

    // VPaths
    {
        const char* stmtStr = "INSERT INTO vPaths(hash32, vPath) VALUES(?, ?);";
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_DBBase, stmtStr, -1, &stmt, nullptr);

        std::ifstream iFile("VPaths.txt");
        if (!iFile.is_open()) {
            CLog::ERR("Failed to open VPaths.txt!");

            sqlite3_finalize(stmt);
            sqlite3_close(m_DBBase);
            return;
        }

        size_t lineCurrent = 0;
        auto lineCnt = std::count(std::istreambuf_iterator<char>(iFile), std::istreambuf_iterator<char>(), '\n');

        iFile.clear();
        iFile.seekg(std::ifstream::beg);

        sqlite3_exec(m_DBBase, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

        std::string line;
        while (std::getline(iFile, line)) {
            lineCurrent++;

            if (line.empty() || line.size() <= 0 || line[0] == ' ')
                continue;

            uint32_t hash32 = hashlittle(line.c_str(), line.length(), 0);

            sqlite3_bind_int64(stmt, 1, hash32);
            sqlite3_bind_text(stmt, 2, line.c_str(), int(line.length()), SQLITE_STATIC);

            auto retVal = sqlite3_step(stmt);
            if (retVal != SQLITE_DONE) {
                CLog::ERR("sqlite3_step failed! Val: %d, Err: %s", retVal, sqlite3_errmsg(m_DBBase));
                CLog::WAR("hash32: %d, string: %s", hash32, line.c_str());
            }

            {
                std::lock_guard lock(m_Mutex);
                m_ProgressVPaths = (static_cast<float>(lineCurrent) / lineCnt);

                if (m_ShouldCancel) {
                    m_ShouldCancel = false;
                    iFile.close();

                    sqlite3_reset(stmt);
                    sqlite3_exec(m_DBBase, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
                    sqlite3_finalize(stmt);
                    sqlite3_close(m_DBBase);

                    CLog::WAR("User cancelled creating database!");
                    return;
                }
            }

            sqlite3_reset(stmt);
        }

        iFile.close();

        sqlite3_exec(m_DBBase, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
        sqlite3_finalize(stmt);
    }

    sqlite3_exec(m_DBBase, idx2, nullptr, nullptr, nullptr);
    sqlite3_close(m_DBBase);
}
void CDatabaseManager::Thread_LoadDataFromTxt() {
    m_Initialized = false;

    ClearProps();
    ClearVPaths();

    // Props
    {
        std::ifstream iFile("Props.txt");
        if (!iFile.is_open()) {
            CLog::ERR("Failed to open Props.txt!");
            return;
        }

        size_t lineCurrent = 0;
        auto lineCnt = std::count(std::istreambuf_iterator<char>(iFile), std::istreambuf_iterator<char>(), '\n');

        iFile.clear();
        iFile.seekg(std::ifstream::beg);

        std::string line;
        while (std::getline(iFile, line)) {
            lineCurrent++;

            if (line.empty() || line.size() <= 0 || line[0] == ' ')
                continue;

            uint32_t hash32 = hashlittle(line.c_str(), line.size(), 0);

            {
                std::lock_guard lock(m_Mutex);

                m_Props.insert({ hash32, line });
                m_VecProps.push_back(hash32);

                m_ProgressProps = (static_cast<float>(lineCurrent) / lineCnt);
            }
        }

        iFile.close();
    }

    // VPaths
    {
        std::ifstream iFile("VPaths.txt");
        if (!iFile.is_open()) {
            CLog::ERR("Failed to open VPaths.txt!");
            return;
        }

        size_t lineCurrent = 0;
        auto lineCnt = std::count(std::istreambuf_iterator<char>(iFile), std::istreambuf_iterator<char>(), '\n');

        iFile.clear();
        iFile.seekg(std::ifstream::beg);

        std::string line;
        while (std::getline(iFile, line)) {
            lineCurrent++;

            if (line.empty() || line.size() <= 0 || line[0] == ' ')
                continue;

            uint32_t hash32 = hashlittle(line.c_str(), line.size(), 0);

            {
                std::lock_guard lock(m_Mutex);

                m_VPaths.insert({ hash32, line });
                m_VecVPaths.push_back(hash32);

                m_ProgressVPaths = (static_cast<float>(lineCurrent) / lineCnt);
            }
        }

        iFile.close();
    }

    m_Initialized = true;
}
void CDatabaseManager::Thread_LoadDataFromDb() {
    m_Initialized = false;

    ClearProps();
    ClearVPaths();

    if (sqlite3_open("Database_base.db", &m_DBBase)) {
        CLog::ERR("Error while opening database! Error: %s", sqlite3_errmsg(m_DBBase));
        return;
    }

    size_t sizeProps = 0;
    size_t sizeVPaths = 0;

    // Get sizes
    {
        sqlite3_stmt* stmt = nullptr;
        const char* stmtStr = "SELECT COUNT(*) FROM props UNION ALL SELECT COUNT(*) FROM vPaths;";

        sqlite3_prepare_v2(m_DBBase, stmtStr, -1, &stmt, nullptr);

        sqlite3_step(stmt);
        sizeProps = sqlite3_column_int64(stmt, 0);
        sqlite3_step(stmt);
        sizeVPaths = sqlite3_column_int64(stmt, 0);

        sqlite3_finalize(stmt);
    }

    // Props
    {
        sqlite3_stmt* stmt = nullptr;
        const char* stmtStr = "SELECT hash32, prop FROM props;";

        sqlite3_prepare_v2(m_DBBase, stmtStr, -1, &stmt, nullptr);

        uint64_t lineCurrent = 0;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            lineCurrent++;

            uint32_t hash32 = static_cast<uint32_t>(sqlite3_column_int64(stmt, 0));
            std::string text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

            {
                std::lock_guard lock(m_Mutex);

                m_Props.insert({ hash32, text });
                m_VecProps.push_back(hash32);

                m_ProgressProps = (static_cast<float>(lineCurrent) / sizeProps);
            }
        }

        sqlite3_finalize(stmt);
    }

    // VPaths
    {
        const char* stmtStr = "SELECT hash32, vPath FROM vPaths;";
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_DBBase, stmtStr, -1, &stmt, nullptr);

        uint64_t lineCurrent = 0;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            lineCurrent++;

            uint32_t hash32 = static_cast<uint32_t>(sqlite3_column_int64(stmt, 0));
            std::string text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

            {
                std::lock_guard lock(m_Mutex);

                m_VPaths.insert({ hash32, text });
                m_VecVPaths.push_back(hash32);

                m_ProgressVPaths = (static_cast<float>(lineCurrent) / sizeVPaths);
            }
        }

        sqlite3_finalize(stmt);
    }

    sqlite3_close(m_DBBase);

    m_Initialized = true;
}

void CDatabaseManager::CreateDatabase(bool recreate) {
    if (!recreate && fs::exists("Database_base.db")) {
        CLog::INF("Database already exists: %s", "Database_base.db");
        return;
    }

    if (recreate) {
        CLog::WAR("Recreating existing database! Path: %s", "Database_base.db");
        fs::remove("Database_base.db");
    }

    m_OpenPopup = true;

    std::thread thread(&CDatabaseManager::Thread_CreateDatabase, this);
    thread.detach();
}

void CDatabaseManager::CancelCurrentOperation() {
    m_ShouldCancel = true;

    m_ProgressProps = 0.00f;
    m_ProgressVPaths = 0.00f;
}

void CDatabaseManager::LoadDataFromFileTxt() {
    m_OpenPopup = true;

    std::thread thread(&CDatabaseManager::Thread_LoadDataFromTxt, this);
    thread.detach();
}
void CDatabaseManager::LoadDataFromFileDb() {
    m_OpenPopup = true;

    std::thread thread(&CDatabaseManager::Thread_LoadDataFromDb, this);
    thread.detach();
}

void CDatabaseManager::FindDupsLocal() {
    std::vector<uint32_t> toSkip;
    //std::unordered_set<uint32_t> toSkip;

    for (auto& [key, val] : m_VPaths) {
        // Skip keys with only one occurrence
        if (m_VPaths.count(key) <= 1)
            continue;

        // Skip already processed key
        auto find = std::find(toSkip.begin(), toSkip.end(), key);
        if (find != toSkip.end())
            continue;

        // Remove toSkip.push_back(key) if using this.
        /*if (!toSkip.insert(key).second)
            continue;*/

        CLog::WAR("Duplicate hash found! Hash: %x", key);
        toSkip.push_back(key);

        auto its = m_VPaths.equal_range(key);
        for (auto& it = its.first; it != its.second; ++it)
            CLog::WAR("\t%s", it->second.c_str());
        CLog::WAR("");
    }
}

std::string str;
bool shouldFilter = false;
std::vector<std::string*> filtered;

void CDatabaseManager::Draw() {
    if (!ImGui::Begin("Database Browser", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    shouldFilter |= ImGui::InputText("Search", &str);

    if (ImGui::BeginTabBar("##Tabs")) {
        if (ImGui::BeginTabItem("Props")) {
            DrawProps();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("VPaths")) {
            DrawVPaths();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
void CDatabaseManager::DrawProps() {
    if (!ImGui::BeginTable("Database Browser Table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::EndTable();
        return;
    }

    ImGui::TableSetupColumn("Hash32");
    ImGui::TableSetupColumn("Prop");
    ImGui::TableHeadersRow();

    if (!m_Initialized) {
        ImGui::EndTable();
        return;
    }

    // Filtering
    if (shouldFilter) {
        filtered.clear();

        if (!str.empty()) {
            for (auto& [key, val] : m_Props) {
                if (val.find(str) != std::string::npos)
                    filtered.push_back(&val);
            }
        }

        shouldFilter = false;
    }

    ImGuiListClipper clipper{};

    if (filtered.size() > 0)
        clipper.Begin(int(filtered.size()));
    else
        clipper.Begin(int(m_Props.size()));

    while (clipper.Step()) {
        for (int rowN = clipper.DisplayStart; rowN < clipper.DisplayEnd; ++rowN) {
            if (filtered.size() > 0) {
                auto item = filtered[rowN];

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%X", m_VecProps[rowN]);

                ImGui::TableNextColumn();
                ImGui::Text("%s", item->c_str());
            }
            else {
                auto it = m_Props.find(m_VecProps[rowN]);

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%X", (*it).first);

                ImGui::TableNextColumn();
                ImGui::Text("%s", (*it).second.c_str());
            }
        }
    }

    ImGui::EndTable();
}
void CDatabaseManager::DrawVPaths() {
    if (!ImGui::BeginTable("Database Browser Table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::EndTable();
        return;
    }

    ImGui::TableSetupColumn("Hash32");
    ImGui::TableSetupColumn("VPath");
    ImGui::TableHeadersRow();

    if (!m_Initialized) {
        ImGui::EndTable();
        return;
    }

    // Filter items on change
    if (shouldFilter) {
        filtered.clear();

        if (!str.empty()) {
            for (auto& [key, val] : m_VPaths) {
                if (val.find(str) != std::string::npos)
                    filtered.push_back(&val);
            }
        }

        shouldFilter = false;
    }

    ImGuiListClipper clipper{};

    if (filtered.size() > 0)
        clipper.Begin((int)filtered.size());
    else
        clipper.Begin((int)m_VPaths.size());

    while (clipper.Step()) {
        for (int rowN = clipper.DisplayStart; rowN < clipper.DisplayEnd; ++rowN) {
            if (filtered.size() > 0) {
                auto item = filtered[rowN];

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%X", m_VecVPaths[rowN]);

                ImGui::TableNextColumn();
                ImGui::Text("%s", item->c_str());
            }
            else {
                auto it = m_VPaths.find(m_VecVPaths[rowN]);

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%X", (*it).first);

                ImGui::TableNextColumn();
                ImGui::Text("%s", (*it).second.c_str());
            }
        }
    }

    ImGui::EndTable();
}
void CDatabaseManager::DrawProgressbar() {
    // TODO: Replace with some variable. Ex: shouldShowProgress.
    if (m_ProgressProps <= 0.00f && m_ProgressVPaths <= 0.00f)
        return;

    if (m_OpenPopup) {
        ImGui::OpenPopup("DB_Progress");
        m_OpenPopup = false;
    }

    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.50f, 0.50f));

    if (ImGui::BeginPopupModal("DB_Progress")) {
        std::lock_guard lock(m_Mutex);

        ImGui::ProgressBar(m_ProgressProps, ImVec2(0.00f, 0.00f));
        ImGui::ProgressBar(m_ProgressVPaths, ImVec2(0.00f, 0.00f));

        if (ImGui::Button("Cancel")) {
            CancelCurrentOperation();

            ImGui::CloseCurrentPopup();
        }

        if (m_ProgressProps >= 1.00f && m_ProgressVPaths >= 1.00f) {
            m_ProgressProps = 0.00f;
            m_ProgressVPaths = 0.00f;

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

const bool CDatabaseManager::IsInitialized() { return m_Initialized; }

const std::unordered_multimap<uint32_t, std::string>& CDatabaseManager::GetProps() { return m_Props; }
const std::unordered_multimap<uint32_t, std::string>& CDatabaseManager::GetVPaths() { return m_VPaths; }
