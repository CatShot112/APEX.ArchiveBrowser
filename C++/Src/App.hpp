#pragma once

#include <mutex>

enum SDL_AppResult;
struct SDL_Window;
union SDL_Event;

// TODO: Validate if database is not corrupted/incomplete.
//       Remove incomplete/corrupted database from disk before creating it again.

// TODO: Ability to import new strings to database without recreating it.

// TODO: Cancellation of extraction.
//       Popup just like when creating/loading database.

// TODO: Create global popup mgr?

// TODO: Fix mutex where needed.

// TODO: Total folder size (if enabled in settings).
//       Requires openeing file/folder again to populate data?

// TODO: Determine file type.

// TODO: File size in human readable format with tooltip showing size in bytes.

// TODO: Complete creating Resolver for conflicting hashes.
//       Ability to extract raw file for inspection in external programs. (or in local hex viewer if implemented)
//       Add simple usage/tutorial.
//       Ability to remember which prop/vPath selected for current game version. (in game_ver database?)

class App {
    bool m_ShouldExit;

    bool m_FileOpened;

    bool m_ShowLog;
    bool m_ShowAbout;
    bool m_ShowSettings;
    bool m_ShowResolver;

    bool m_AutoScroll;
    bool m_ScrollToBottom;

    SDL_Window* m_Window;

    std::mutex m_Mutex;

    void Draw_MainMenuBar();
    void Draw_Log();
    void Draw_About();
    void Draw_Settings();

public:
    App();
    ~App();

    void Initialize(SDL_Window* window);
    void Cleanup();

    SDL_AppResult Update();
    SDL_AppResult ProcessEvent(SDL_Event* event);

    void Draw();
};
