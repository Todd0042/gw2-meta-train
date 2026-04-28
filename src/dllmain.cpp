#include "Nexus.h"
#include "events.h"
#include "state.h"
#include "ui.h"
#include "icon_png.h"
#include <windows.h>
#include <cstdio>

// ── global API handle ─────────────────────────────────────────────────────
AddonAPI_t* APIDefs    = nullptr;
Texture_t*  g_AddonIcon = nullptr;

// ── forward declarations ──────────────────────────────────────────────────
static void AddonLoad     (AddonAPI_t* api);
static void AddonUnload   ();
static void OnRender      ();
static void OnOptions     ();
static void OnMumble      (void* eventArgs);
static void OnToggleWindow(const char* id, bool isRelease);

// ── addon definition ──────────────────────────────────────────────────────
// Signature: self-published negative value cast to uint32_t (Lessons_Learned §33)
static AddonDefinition_t s_AddonDef = {
    /* Signature    */ (uint32_t)(-0x4D54), // 'MT' negated — unique, non-zero, non-Raidcore
    /* APIVersion   */ NEXUS_API_VERSION,
    /* Name         */ "Meta Train Commander",
    /* Version      */ {1, 0, 0, 0},
    /* Author       */ "Todd0042",
    /* Description  */ "GW2 meta-event train timer, waypoint helper, and squad announcer for Commanders.",
    /* Load         */ AddonLoad,
    /* Unload       */ AddonUnload,
    /* Flags        */ AF_None,
    /* Provider     */ UP_GitHub,
    /* UpdateLink   */ "https://github.com/Todd0042/gw2-meta-train",
};

// ── required Nexus export ─────────────────────────────────────────────────
extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef()
{
    return &s_AddonDef;
}

// ── DLL entry ─────────────────────────────────────────────────────────────
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_DETACH && APIDefs)
        AddonUnload();
    return TRUE;
}

// ── AddonLoad ─────────────────────────────────────────────────────────────
static void AddonLoad(AddonAPI_t* api)
{
    APIDefs = api;

    // Initialise event table and state
    EventsInit();
    StateInit();

    // Load saved settings
    const char* addonDir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
    if (addonDir)
        StateLoad(addonDir);

    // Load addon icon from embedded PNG data
    g_AddonIcon = APIDefs->Textures_GetOrCreateFromMemory(
        "METATRAIN_ICON", (void*)icon_png, (uint64_t)icon_png_len);

    // Subscribe to map-change events (Lessons_Learned §9: filter garbage names)
    APIDefs->Events_Subscribe(EV_MUMBLE_IDENTITY_UPDATED, OnMumble);

    // Register render callbacks
    APIDefs->GUI_Register(RT_Render,        OnRender);
    APIDefs->GUI_Register(RT_OptionsRender, OnOptions);

    // Register keybind and Quick Access toolbar icon (window starts hidden)
    APIDefs->InputBinds_RegisterWithString("KB_METATRAIN_TOGGLE", OnToggleWindow, "");
    APIDefs->QuickAccess_Add("QA_METATRAIN", "METATRAIN_ICON", "METATRAIN_ICON",
                             "KB_METATRAIN_TOGGLE", "Meta Train Commander");

    APIDefs->Log(LOGL_INFO, "MetaTrain", "Meta Train Commander loaded.");
}

// ── AddonUnload ───────────────────────────────────────────────────────────
static void AddonUnload()
{
    if (!APIDefs) return;

    APIDefs->Events_Unsubscribe(EV_MUMBLE_IDENTITY_UPDATED, OnMumble);
    APIDefs->GUI_Deregister(OnRender);
    APIDefs->GUI_Deregister(OnOptions);
    APIDefs->QuickAccess_Remove("QA_METATRAIN");
    APIDefs->InputBinds_Deregister("KB_METATRAIN_TOGGLE");

    const char* addonDir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
    if (addonDir)
        StateSave(addonDir);

    APIDefs->Log(LOGL_INFO, "MetaTrain", "Meta Train Commander unloaded.");
    APIDefs = nullptr;
}

// ── render callbacks ──────────────────────────────────────────────────────
static void OnRender()  { UIRender(); }
static void OnOptions() { UIRenderOptions(); }

// ── window toggle (keybind + Quick Access click) ──────────────────────────
static void OnToggleWindow(const char* /*id*/, bool isRelease)
{
    if (isRelease) return;
    std::lock_guard<std::mutex> lk(g_StateMutex);
    g_State.windowOpen = !g_State.windowOpen;
}

// ── Mumble identity callback ──────────────────────────────────────────────
// Fired when the player changes map or character.
// Filter garbage names that appear at startup (Lessons_Learned §9).
static void OnMumble(void* eventArgs)
{
    MumbleIdentity* id = static_cast<MumbleIdentity*>(eventArgs);
    if (!id) return;

    // Garbage-name filter: reject paths (Lessons_Learned §9)
    if (strchr(id->Name, '\\') || strchr(id->Name, '/') || strchr(id->Name, ':'))
        return;

    uint32_t newMap = id->MapID;
    if (newMap == 0) return;

    {
        std::lock_guard<std::mutex> lk(g_StateMutex);
        if (g_State.currentMapId == newMap) return; // no change
        g_State.currentMapId = newMap;
    }

    // Let the UI module handle auto-advance logic
    UIOnMapChanged(newMap);
}
