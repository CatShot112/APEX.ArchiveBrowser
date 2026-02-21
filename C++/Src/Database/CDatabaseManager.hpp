#pragma once

#include <mutex>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

struct sqlite3;

class CDatabaseManager {
    sqlite3* m_DBBase;
    sqlite3* m_DBGame;

    std::unordered_multimap<uint32_t, std::string> m_Props;
    std::unordered_multimap<uint32_t, std::string> m_VPaths;

    std::vector<uint32_t> m_VecProps;
    std::vector<uint32_t> m_VecVPaths;

    float m_ProgressProps;
    float m_ProgressVPaths;

    std::mutex m_Mutex;

    bool m_OpenPopup;
    bool m_Initialized;
    bool m_ShouldCancel;

    void ClearProps();
    void ClearVPaths();

    void Thread_CreateDatabase();
    void Thread_LoadDataFromTxt();
    void Thread_LoadDataFromDb();

protected:
    CDatabaseManager();
    ~CDatabaseManager();

    CDatabaseManager(CDatabaseManager&) = delete;
    CDatabaseManager(const CDatabaseManager&) = delete;

    CDatabaseManager operator =(CDatabaseManager&) = delete;
    CDatabaseManager operator =(const CDatabaseManager&) = delete;

public:
    static CDatabaseManager& Instance();

    void CreateDatabase(bool recreate = false);

    void CancelCurrentOperation();

    // TODO: Progress bars
    void LoadDataFromFileTxt();
    void LoadDataFromFileDb();

    void FindDupsLocal();
    /*void FindDupsDatabase() {
        if (sqlite3_open("Database_base.db", &m_DBBase)) {
            CLog::ERR("Error while opening database! Error: %s", sqlite3_errmsg(m_DBBase));
            return;
        }

        //const char* stmtStr = "SELECT hash32, vPath FROM vPaths GROUP BY hash32 HAVING COUNT(*) > 1;";
        const char* stmtStr = "SELECT a.hash32, a.vPath FROM vPaths a JOIN (SELECT hash32, vPath FROM vPaths GROUP BY hash32 HAVING COUNT(*) > 1) b ON a.hash32 = b.hash32 ORDER BY a.hash32;";
        sqlite3_stmt* stmt = nullptr;

        sqlite3_prepare_v2(m_DBBase, stmtStr, -1, &stmt, nullptr);

        uint32_t prevHash = 0;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            uint32_t hash32 = static_cast<uint32_t>(sqlite3_column_int64(stmt, 0));
            std::string text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

            if (prevHash != hash32) {
                CLog::WAR("Duplicate hash found! Hash: %x", hash32);
                prevHash = hash32;
            }

            CLog::WAR("\t%s", text.c_str());
            CLog::WAR("\n");
        }

        sqlite3_finalize(stmt);
        sqlite3_close(m_DBBase);
    }*/

    void Draw();

private:
    void DrawProps();
    void DrawVPaths();

public:
    void DrawProgressbar();

    const bool IsInitialized();

    const std::unordered_multimap<uint32_t, std::string>& GetProps();
    const std::unordered_multimap<uint32_t, std::string>& GetVPaths();
};
