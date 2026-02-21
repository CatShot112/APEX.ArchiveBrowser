#define SDL_MAIN_USE_CALLBACKS 1

#include "App.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_messagebox.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl3.h>
#include <imgui/imgui_impl_sdlrenderer3.h>

#pragma comment(lib, "SDL3.lib")

constexpr auto PROGRAM_NAME = "APEX.ArchiveBrowser";
constexpr auto PROGRAM_VER = "1.0.0";

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

ImVec2 windowResolution = { 1280.00f, 720.00f };
ImVec4 clearColor = { 0.45f, 0.55f, 0.60f, 1.00f };

App app;

static SDL_AppResult SDL_AppInit(void** appState, int argc, char** argv) {
    SDL_SetAppMetadata(PROGRAM_NAME, PROGRAM_VER, nullptr);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        auto error = SDL_GetError();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to initialize SDL3!", error, window);

        return SDL_APP_FAILURE;
    }

    float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    window = SDL_CreateWindow(PROGRAM_NAME, int(windowResolution.x * mainScale), int(windowResolution.y * mainScale), windowFlags);
    if (window == nullptr) {
        auto error = SDL_GetError();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to create SDL3 window!", error, window);

        return SDL_APP_FAILURE;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr) {
        auto error = SDL_GetError();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to create SDL3 renderer!", error, window);

        return SDL_APP_FAILURE;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Setup Dear ImGui.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(mainScale);
    style.FontScaleDpi = mainScale;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    app.Initialize(window);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appState) {
    // Don't update app if minimized.
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
        SDL_Delay(10);
        return SDL_APP_CONTINUE;
    }

    auto result = app.Update();

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport();

    app.Draw();

    ImGui::Render();
    auto& io = ImGui::GetIO();

    SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(renderer, clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    SDL_RenderClear(renderer);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);

    return result;
}

SDL_AppResult SDL_AppEvent(void* appState, SDL_Event* event) {
    ImGui_ImplSDL3_ProcessEvent(event);

    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event->window.windowID == SDL_GetWindowID(window))
        return SDL_APP_SUCCESS;

    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        // TODO: Window has been resized. Update positions and scales of GUI elements.
        windowResolution = ImVec2(float(event->window.data1), float(event->window.data2));
    }

    auto result = app.ProcessEvent(event);

    return result;
}

void SDL_AppQuit(void* appState, SDL_AppResult result) {
    app.Cleanup();

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
