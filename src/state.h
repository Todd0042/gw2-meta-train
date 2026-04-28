#pragma once
#include <ctime>
#include <cstdint>
#include <string>
#include <mutex>

// ── persistent settings (saved to MetaTrain.ini) ──────────────────────────
struct Settings {
    bool   autoAnnounce;      // send /d message automatically at prepMinutes
    bool   autoAdvanceOnMap;  // advance loop when player enters event map
    int    startIdx;          // last manually selected start position [0..26]
};

// ── runtime addon state ───────────────────────────────────────────────────
struct AddonState {
    // Loop tracking
    int     currentIdx;       // which event we are currently at or heading to [0..26]
    int     loopStartIdx;     // event where the user started this loop
    time_t  loopStartTime;    // wall-clock when user hit "Start Loop Here"
    bool    loopActive;       // false = idle / just opened

    // Map presence
    uint32_t currentMapId;

    // Waypoint button
    // Becomes unlocked once the PREVIOUS event's UTC start time has passed,
    // so the commander can share the WP while the current event is still running.
    bool waypointUnlocked;

    // Auto-announce gate — resets when currentIdx advances
    bool     announceFired;   // true once the auto-announce fired for this event
    time_t   lastAnnounceTime;

    // Toast notification banner (ImGui overlay inside our window)
    std::string toastText;
    float       toastAlpha;   // counts down from 1.0 to 0.0 over ~3 s

    // ImGui one-time init flag (§1 Lessons_Learned)
    bool imguiReady;

    // Window open state
    bool windowOpen;

    // ── Schedule-a-Run planner ─────────────────────────────────────────
    // Commander picks days-from-today (0-21), hour, minute to preview
    // a full loop schedule.  Year/month are derived from today's UTC date.
    bool   showSchedule;         // whether the schedule panel is open
    int    schedStartIdx;        // which event the planned run begins at
    int    schedDaysOffset;      // 0 = today, 1 = tomorrow … max 21
    int    schedHour;            // 0..23  UTC
    int    schedMinute;          // 0..59  UTC
    bool   schedDirty;           // recalculate when true

    // ── "Ready to move?" popup ─────────────────────────────────────────
    bool   moveNotifyEnabled;    // toggle in settings
    time_t mapEntryTime;         // UTC epoch when player last entered a map

    Settings cfg;
};

extern AddonState  g_State;
extern std::mutex  g_StateMutex; // guards g_State

void StateInit();
void StateSave(const char* addonDir);
void StateLoad(const char* addonDir);

// Thread-safe helper: advance the loop by one position
void StateAdvance();
// Thread-safe helper: set current position without changing loopStartIdx
void StateSetCurrent(int idx);

// Show a toast notification that fades out automatically
void StateToast(const char* msg);
