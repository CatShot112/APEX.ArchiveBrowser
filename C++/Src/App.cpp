#include "App.hpp"

#include "Logging/CLog.hpp"
#include "ArchiveBrowser/CArchiveBrowser.hpp"
#include "Database/CDatabaseManager.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_clipboard.h>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>
#include <imgui/imgui_internal.h>

#include <chrono>
#include <fstream>

// Imitating lock_guard
class CTimed {
    std::string m_String;
    std::chrono::high_resolution_clock::time_point m_Start;
    std::chrono::high_resolution_clock::time_point m_End;

public:
    CTimed(std::string str) {
        m_String = str;
        m_Start = std::chrono::high_resolution_clock::now();
        m_End = {};
    }
    ~CTimed() {
        m_End = std::chrono::high_resolution_clock::now();
        
        auto result = std::chrono::duration_cast<std::chrono::milliseconds>(m_End - m_Start);
        CLog::INF("%s | Duration: %lldms", m_String.c_str(), result.count());
    }

    CTimed(const CTimed&) = delete;
    CTimed& operator=(const CTimed&) = delete;
};

App::App() {
    m_ShouldExit = false;

    m_FileOpened = false;

    m_ShowLog = true;
    m_ShowAbout = false;
    m_ShowSettings = false;

    m_AutoScroll = true;
    m_ScrollToBottom = false;

    m_Window = nullptr;
}
App::~App() {

}

static void SaveLog(std::string path) {
    std::stringstream result;

    for (auto& entry : CLog::GetEntries()) {
        auto time = Utils::UnixToDateTime(entry.m_Timestamp);
        auto type = CLog::TypeToStr(entry.m_Type);

        result << std::format("[{}] [{}]: {}\n", time, type, entry.m_Message);
    }

    std::ofstream oFile(path);
    if (oFile.is_open()) {
        oFile.write(result.str().data(), result.str().size());
        oFile.close();

        CLog::INF("Saved log file as: %s", path.c_str());
    }
    else {
        CLog::ERR("Failed to create Log file! Path: %s", path.c_str());
    }
}

// Callbacks run from separate thread (at least on Windows).
static void Callback_OpenFile(void* userData, const char* const* fileList, int filter) {
    auto& ab = CArchiveBrowser::Instance();

    if (!fileList) {
        CLog::ERR(SDL_GetError());
        return;
    }
    else if (!*fileList) {
        CLog::WAR("User did not select any file to open (cancelled).");
        return;
    }

    if (*fileList) {
        CLog::INF("Opening file: %s", *fileList);
        ab.OpenFile(*fileList);
    }
}
static void Callback_OpenFolder(void* userData, const char* const* folderList, int filter) {
    auto& ab = CArchiveBrowser::Instance();

    if (!folderList) {
        CLog::ERR(SDL_GetError());
        return;
    }
    else if (!*folderList) {
        CLog::WAR("User did not select any folder to open (cancelled).");
        return;
    }

    if (*folderList) {
        CLog::INF("Opening folder: %s", *folderList);
        ab.OpenFolder(*folderList);
    }
}
static void Callback_SaveLogFile(void* userData, const char* const* fileList, int filter) {
    if (!fileList) {
        CLog::ERR(SDL_GetError());
        return;
    }
    else if (!*fileList) {
        CLog::WAR("User did not select any file to save (cancelled).");
        return;
    }

    if (*fileList) {
        SaveLog(*fileList);
    }
}

void App::Initialize(SDL_Window* window) {
    m_Window = window;

    CArchiveBrowser::Instance().Initialize();
}
void App::Cleanup() {

}

SDL_AppResult App::Update() {
    if (m_ShouldExit)
        return SDL_APP_SUCCESS;



    return SDL_APP_CONTINUE;
}
SDL_AppResult App::ProcessEvent(SDL_Event* event) {
    auto& ab = CArchiveBrowser::Instance();

    if (event->type != SDL_EVENT_KEY_DOWN || event->key.repeat)
        return SDL_APP_CONTINUE;

    auto& key = event->key;

    bool ctrl = (key.mod & SDL_KMOD_LCTRL);
    bool shift = (key.mod & SDL_KMOD_LSHIFT);

    if (ctrl && !shift) {
        if (key.scancode == SDL_SCANCODE_O)
            SDL_ShowOpenFileDialog(Callback_OpenFile, 0, SDL_GetWindowFromEvent(event), nullptr, 0, nullptr, false);

        if (ab.IsOpenedAny() && key.scancode == SDL_SCANCODE_R) {
            auto path = ab.GetLastOpenedPath();

            if (ab.IsOpenedFile())
                ab.OpenFile(path);
            else
                ab.OpenFolder(path);
        }

        if (ab.IsOpenedAny() && key.scancode == SDL_SCANCODE_Q)
            ab.Close();
    }

    if (ctrl && shift) {
        if (key.scancode == SDL_SCANCODE_O)
            SDL_ShowOpenFolderDialog(Callback_OpenFolder, 0, SDL_GetWindowFromEvent(event), 0, 0);
    }

    return SDL_APP_CONTINUE;
}

void App::Draw() {
    Draw_MainMenuBar();

    CArchiveBrowser::Instance().Draw();
    CDatabaseManager::Instance().Draw();

    if (m_ShowLog)
        Draw_Log();
    if (m_ShowAbout)
        Draw_About();
    if (m_ShowSettings)
        Draw_Settings();
}

void App::Draw_MainMenuBar() {
    auto& ab = CArchiveBrowser::Instance();

    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("File")) {
        // TODO: Thread-safety for opening files/folders.
        //       Callback and OpenFile/Folder is called from another thread.

        if (ImGui::MenuItem("Open File", "CTRL+O"))
            SDL_ShowOpenFileDialog(Callback_OpenFile, nullptr, m_Window, nullptr, 0, nullptr, false);

        if (ImGui::MenuItem("Open Folder", "CTRL+SHIFT+O"))
            SDL_ShowOpenFolderDialog(Callback_OpenFolder, nullptr, m_Window, nullptr, false);

        ImGui::Separator();

        // TODO: Blocking UI (run in separate thread)
        if (ImGui::MenuItem("Reload", "CTRL+R", nullptr, ab.IsOpenedAny())) {
            auto path = ab.GetLastOpenedPath();

            if (ab.IsOpenedFile())
                ab.OpenFile(path);
            else
                ab.OpenFolder(path);
        }

        if (ImGui::MenuItem("Close", "CTLR+Q", nullptr, ab.IsOpenedAny()))
            ab.Close();

        ImGui::Separator();

        if (ImGui::MenuItem("Resolve duplicates", "", nullptr, ab.HasDuplicatedHashes())) {
            ab.OpenResolver();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit"))
            m_ShouldExit = true;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Database")) {
        if (ImGui::MenuItem("Initialize")) {
            auto& db = CDatabaseManager::Instance();

            auto start = std::chrono::high_resolution_clock::now();

            db.CreateDatabase();

            auto end = std::chrono::high_resolution_clock::now();
            auto result = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            CLog::INF("Create Database, Time: %lld | Size: %lld", result.count(), db.GetVPaths().size());
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Load From .txt File")) {
            auto& db = CDatabaseManager::Instance();

            CTimed timed("LoadDataFromFileTxt()");
            db.LoadDataFromFileTxt();
        }

        if (ImGui::MenuItem("Load From .db File")) {
            auto& db = CDatabaseManager::Instance();

            CTimed timed("LoadDataFromFileDb()");
            db.LoadDataFromFileDb();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Find Dups Local")) {
            auto& db = CDatabaseManager::Instance();

            CTimed timed("FindDupsLocal()");
            db.FindDupsLocal();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Options")) {
        if (ImGui::MenuItem("Settings"))
            m_ShowSettings = true;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About"))
            m_ShowAbout = true;

        ImGui::Separator();

        if (ImGui::MenuItem("Log"))
            m_ShowLog = true;

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void App::Draw_Log() {
    static bool autoscroll = true;
    static bool viewInf = true;
    static bool viewWar = true;
    static bool viewErr = true;

    if (ImGui::Begin("Log", &m_ShowLog, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {
        ImGui::BeginMenuBar();
        {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save")) {
                    auto time = Utils::GetCurrentTimeMs();
                    auto oFilePath = std::string("Log_" + std::to_string(time) + ".txt");

                    SaveLog(oFilePath);
                }

                // TODO: Let user choose path
                if (ImGui::MenuItem("Save As..")) {
                    SDL_ShowSaveFileDialog(Callback_SaveLogFile, nullptr, m_Window, nullptr, 0, nullptr);
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Log")) {
                if (ImGui::MenuItem("Copy all")) {
                    std::stringstream result;

                    for (auto& entry : CLog::GetEntries()) {
                        auto time = Utils::UnixToDateTime(entry.m_Timestamp);
                        auto type = CLog::TypeToStr(entry.m_Type);

                        result << std::format("[{}] [{}]: {}\n", time, type, entry.m_Message);
                    }

                    SDL_SetClipboardText(result.str().data());
                    CLog::INF("Copied log to clipboard.");
                }

                if (ImGui::MenuItem("Clear")) {
                    CLog::Clear();
                }

                ImGui::EndMenu();
            }
        }
        ImGui::EndMenuBar();

        ImGui::Checkbox("Autoscroll", &autoscroll);
        ImGui::SameLine();

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        ImGui::Text("Log type:");
        ImGui::SameLine();

        ImGui::Checkbox("Info", &viewInf);
        ImGui::SameLine();

        ImGui::Checkbox("Warning", &viewWar);
        ImGui::SameLine();

        ImGui::Checkbox("Error", &viewErr);
        ImGui::Separator();

        if (ImGui::BeginChild("##Log_Scrolling", ImVec2(0, 0), 0, ImGuiWindowFlags_NoMove)) {
            //std::lock_guard lock(m_Mutex);

            auto& entries = CLog::GetEntries();

            ImGuiListClipper clipper{};
            clipper.Begin(int(entries.size()));

            while (clipper.Step()) {
                for (int rowN = clipper.DisplayStart; rowN < clipper.DisplayEnd; ++rowN) {
                    auto& entry = entries[rowN];
                    auto type = entry.m_Type;

                    if (type == ELogType::INFO && !viewInf)
                        continue;
                    if (type == ELogType::WARNING && !viewWar)
                        continue;
                    if (type == ELogType::ERROR && !viewErr)
                        continue;

                    if (type != ELogType::INFO)
                        ImGui::PushStyleColor(ImGuiCol_Text, type == ELogType::ERROR ? IM_COL32(255, 0, 0, 255) : IM_COL32(255, 255, 0, 255));

                    auto time = Utils::UnixToDateTime(entry.m_Timestamp);
                    ImGui::Text("[%s] [%s]: %s", time.c_str(), CLog::TypeToStr(type).c_str(), entry.m_Message.c_str());

                    if (type != ELogType::INFO)
                        ImGui::PopStyleColor();
                }
            }

            if (m_ScrollToBottom || (autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
                ImGui::SetScrollHereY();
                m_ScrollToBottom = false;
            }
        }

        ImGui::EndChild();
    }

    ImGui::End();
}

void App::Draw_About() {
    if (!ImGui::Begin("About", &m_ShowAbout, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) {
        ImGui::End();
        return;
    }

    ImGui::Text("APEX.ArchiveBrowser");
    ImGui::Spacing();

    if (ImGui::BeginTabBar("About")) {
        if (ImGui::BeginTabItem("About")) {
            ImGui::TextWrapped("Program allows to browse APEX engine game archives and extract files from them.");
            ImGui::TextWrapped("Later you will be able to modify or replace them.");
            ImGui::Spacing();
            ImGui::Text("Special thanks to:");

            ImGui::TextLinkOpenURL("kk49", "https://github.com/kk49");
            ImGui::SameLine();
            ImGui::Text("- For creating");
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("DECA", "https://github.com/kk49/deca");
            ImGui::SameLine(0.00f, 0.00f);
            ImGui::Text(", a tool for modding APEX engine games.");

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Libraries")) {
            ImGui::TextLinkOpenURL("ImGui v1.92.5-docking", "https://github.com/ocornut/imgui");
            ImGui::SameLine();
            ImGui::Text("- Graphical User Interface.");

            ImGui::TextLinkOpenURL("SDL3 v3.4.0", "https://github.com/libsdl-org/SDL");
            ImGui::SameLine();
            ImGui::Text("- Multiplatform backend.");

            ImGui::TextLinkOpenURL("SQLite v3.51.2", "https://sqlite.org/index.html");
            ImGui::SameLine();
            ImGui::Text("- Local database.");

            ImGui::Separator();
            
            ImGui::TextLinkOpenURL("hashlittle", "https://www.burtleburtle.net/bob/c/lookup3.c");
            ImGui::SameLine();
            ImGui::Text("- Used for 32bit hashes. Took only parts of source code.");

            ImGui::TextLinkOpenURL("MurmurHash3", "https://github.com/aappleby/smhasher");
            ImGui::SameLine();
            ImGui::Text("- Used for 64bit hashes.");

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Contributors")) {
            ImGui::TextLinkOpenURL("CatShot112", "https://github.com/CatShot112");
            ImGui::SameLine();
            ImGui::Text("- Author.");

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("License")) {
            ImGui::TextWrapped("Program licensed under GPL-3. You can read GPL-3 license by opening 'LICENSE-GPL.txt' file in text editor.");

            ImGui::Separator();

            ImGui::TextWrapped("SDL3 uses Zlib License. You can read it by opening 'LICENSE-ZLIB.txt' file in text editor.");
            ImGui::TextWrapped("ImGui uses MIT License. You can read it by opening 'LICENSE-MIT.txt' file in text editor.");

            ImGui::Separator();

            ImGui::TextWrapped("SQLite, MurmurHash3 and lookup3.c (hashlittle) are under Public Domain.");

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void App::Draw_Settings() {
    if (!ImGui::Begin("Settings", &m_ShowSettings, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) {
        ImGui::End();
        return;
    }



    ImGui::End();
}
