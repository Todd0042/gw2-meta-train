#include "state.h"
#include "events.h"
#include <cstdio>
#include <cstring>

AddonState g_State   = {};
std::mutex g_StateMutex;

void StateInit()
{
    std::lock_guard<std::mutex> lk(g_StateMutex);
    g_State = {};
    g_State.windowOpen   = false;
    g_State.imguiReady   = false;
    g_State.loopActive   = false;
    g_State.currentIdx   = 0;
    g_State.loopStartIdx = 0;
    g_State.toastAlpha   = 0.0f;
    g_State.cfg.autoAnnounce     = false;
    g_State.cfg.autoAdvanceOnMap = true;
    g_State.cfg.startIdx         = 0;

    // Initialise schedule planner to current UTC time
    time_t now = time(nullptr);
    struct tm u;
#ifdef _WIN32
    gmtime_s(&u, &now);
#else
    gmtime_r(&now, &u);
#endif
    g_State.showSchedule      = false;
    g_State.schedStartIdx     = 0;
    g_State.schedDaysOffset   = 0;
    g_State.schedHour         = u.tm_hour;
    g_State.schedMinute       = (u.tm_min / 5) * 5;
    g_State.schedDirty        = true;
    g_State.moveNotifyEnabled = true;
    g_State.mapEntryTime      = 0;
}

// ── settings file (INI-style key=value) ──────────────────────────────────
static void BuildIniPath(char* out, size_t sz, const char* addonDir)
{
    snprintf(out, sz, "%s\\MetaTrain.ini", addonDir);
}

void StateSave(const char* addonDir)
{
    char path[512];
    BuildIniPath(path, sizeof(path), addonDir);

    FILE* f = fopen(path, "w");
    if (!f) return;

    std::lock_guard<std::mutex> lk(g_StateMutex);
    fprintf(f, "autoAnnounce=%d\n",      g_State.cfg.autoAnnounce      ? 1 : 0);
    fprintf(f, "autoAdvanceOnMap=%d\n", g_State.cfg.autoAdvanceOnMap  ? 1 : 0);
    fprintf(f, "moveNotify=%d\n",       g_State.moveNotifyEnabled     ? 1 : 0);
    fprintf(f, "startIdx=%d\n",         g_State.cfg.startIdx);
    fclose(f);
}

void StateLoad(const char* addonDir)
{
    char path[512];
    BuildIniPath(path, sizeof(path), addonDir);

    FILE* f = fopen(path, "r");
    if (!f) return;

    char line[128];
    std::lock_guard<std::mutex> lk(g_StateMutex);
    while (fgets(line, sizeof(line), f)) {
        int v;
        if (sscanf(line, "autoAnnounce=%d",      &v) == 1) g_State.cfg.autoAnnounce      = (v != 0);
        if (sscanf(line, "autoAdvanceOnMap=%d", &v) == 1) g_State.cfg.autoAdvanceOnMap  = (v != 0);
        if (sscanf(line, "moveNotify=%d",       &v) == 1) g_State.moveNotifyEnabled     = (v != 0);
        if (sscanf(line, "startIdx=%d",         &v) == 1) g_State.cfg.startIdx = v % META_EVENT_COUNT;
    }
    fclose(f);

    // Apply loaded start position
    g_State.currentIdx   = g_State.cfg.startIdx;
    g_State.loopStartIdx = g_State.cfg.startIdx;
}

void StateAdvance()
{
    std::lock_guard<std::mutex> lk(g_StateMutex);
    g_State.currentIdx = (g_State.currentIdx + 1) % META_EVENT_COUNT;
    g_State.announceFired = false;
    g_State.waypointUnlocked = false;
}

void StateSetCurrent(int idx)
{
    std::lock_guard<std::mutex> lk(g_StateMutex);
    g_State.currentIdx   = idx % META_EVENT_COUNT;
    g_State.announceFired = false;
    g_State.waypointUnlocked = false;
}

void StateToast(const char* msg)
{
    std::lock_guard<std::mutex> lk(g_StateMutex);
    g_State.toastText  = msg;
    g_State.toastAlpha = 1.0f;
}
