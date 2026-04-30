#pragma once
#include <ctime>
#include <cstdint>
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "events.h"

// RAII wrapper for CRITICAL_SECTION — drop-in replacement for std::lock_guard
struct CritLock {
    CRITICAL_SECTION* _cs;
    explicit CritLock(CRITICAL_SECTION* cs) : _cs(cs) { EnterCriticalSection(_cs); }
    ~CritLock() { LeaveCriticalSection(_cs); }
    CritLock(const CritLock&) = delete;
    CritLock& operator=(const CritLock&) = delete;
};

// ── persistent settings (saved to MetaTrain.ini) ──────────────────────────
struct Settings {
    bool   autoAnnounce;      // send /d message automatically at prepMinutes
    bool   autoAdvanceOnMap;  // advance loop when player enters event map
    int    startIdx;          // last manually selected start position
};

// ── named saved run ───────────────────────────────────────────────────────
static const int MAX_SAVED_RUNS = 8;

struct SavedRun {
    char name[32];
    int  plan[MAX_LOOP_PLAN];
    int  count;                // 0 = slot unused
};

// ── runtime addon state ───────────────────────────────────────────────────
struct AddonState {
    // Loop tracking
    int     currentIdx;       // index into g_Metas[] for the current event
    int     loopStartIdx;     // g_Metas[] index where the user started this loop
    time_t  loopStartTime;    // wall-clock when user hit "Start"
    bool    loopActive;       // false = idle / plan being built

    // Custom run plan (indices into g_Metas[])
    int     loopPlan[MAX_LOOP_PLAN];
    int     loopPlanCount;    // 0 = no plan built
    int     loopPlanPos;      // current position in the plan

    // Named saved runs
    SavedRun savedRuns[MAX_SAVED_RUNS];
    int      savedRunsCount;  // number of occupied slots
    int      savedRunsSel;    // dropdown selection index (0-based)
    char     saveNameBuf[32]; // text input buffer for naming a new save

    // Map presence
    uint32_t currentMapId;

    // Waypoint button
    bool waypointUnlocked;

    // Auto-announce gate — resets when currentIdx advances
    bool     announceFired;
    time_t   lastAnnounceTime;

    // Toast notification banner
    std::string toastText;
    float       toastAlpha;

    // ImGui one-time init flag
    bool imguiReady;

    // Window open state
    bool windowOpen;

    // ── Schedule-a-Run planner ─────────────────────────────────────────
    bool   showSchedule;
    int    schedStartIdx;
    int    schedDaysOffset;
    int    schedHour;
    int    schedMinute;
    bool   schedDirty;

    // ── "Ready to move?" popup ─────────────────────────────────────────
    bool   moveNotifyEnabled;
    time_t mapEntryTime;

    Settings cfg;
};

extern AddonState        g_State;
extern CRITICAL_SECTION  g_StateMutex;

void StateInit();
void StateSave(const char* addonDir);
void StateLoad(const char* addonDir);

void StateAdvance();
void StateSetCurrent(int idx);
void StateToast(const char* msg);
