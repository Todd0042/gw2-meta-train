#include "state.h"
#include "events.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

AddonState       g_State      = {};
CRITICAL_SECTION g_StateMutex = {};

void StateInit()
{
    InitializeCriticalSection(&g_StateMutex);
    CritLock lk(&g_StateMutex);

    g_State = {};
    g_State.windowOpen     = false;
    g_State.imguiReady     = false;
    g_State.loopActive     = false;
    g_State.currentIdx     = 0;
    g_State.loopStartIdx   = 0;
    g_State.loopPlanCount  = 0;
    g_State.loopPlanPos    = 0;
    g_State.toastAlpha     = 0.0f;
    g_State.savedRunsCount = 0;
    g_State.savedRunsSel   = 0;
    g_State.saveNameBuf[0] = '\0';
    g_State.cfg.autoAnnounce     = false;
    g_State.cfg.autoAdvanceOnMap = true;
    g_State.cfg.startIdx         = 0;

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

// ── settings file ─────────────────────────────────────────────────────────
static void BuildIniPath(char* out, size_t sz, const char* addonDir)
{
    snprintf(out, sz, "%s\\MetaTrain.ini", addonDir);
}

void StateSave(const char* addonDir)
{
    char path[512];
    BuildIniPath(path, sizeof(path), addonDir);

    // Ensure the directory exists — Nexus returns the path but may not have created it yet.
    CreateDirectoryA(addonDir, nullptr);

    FILE* f = fopen(path, "w");
    if (!f) return;

    // Snapshot under lock, then write outside lock so disk I/O doesn't block callbacks.
    bool   autoAnnounce     = false;
    bool   autoAdvanceOnMap = false;
    bool   moveNotify       = false;
    int    startIdx         = 0;
    int    numSaved         = 0;
    int    planCount        = 0;
    int    planCopy[MAX_LOOP_PLAN] = {};
    SavedRun runs[MAX_SAVED_RUNS] = {};

    {
        CritLock lk(&g_StateMutex);
        autoAnnounce     = g_State.cfg.autoAnnounce;
        autoAdvanceOnMap = g_State.cfg.autoAdvanceOnMap;
        moveNotify       = g_State.moveNotifyEnabled;
        startIdx         = g_State.cfg.startIdx;
        numSaved         = g_State.savedRunsCount;
        planCount        = g_State.loopPlanCount;
        for (int i = 0; i < planCount; i++)
            planCopy[i] = g_State.loopPlan[i];
        for (int r = 0; r < numSaved; r++)
            runs[r] = g_State.savedRuns[r];
    }

    fprintf(f, "autoAnnounce=%d\n",     autoAnnounce     ? 1 : 0);
    fprintf(f, "autoAdvanceOnMap=%d\n", autoAdvanceOnMap ? 1 : 0);
    fprintf(f, "moveNotify=%d\n",       moveNotify       ? 1 : 0);
    fprintf(f, "startIdx=%d\n",         startIdx);

    // Active plan — restored on load so the user can resume without re-building
    fprintf(f, "planCount=%d\n", planCount);
    if (planCount > 0) {
        fprintf(f, "plan=");
        for (int i = 0; i < planCount; i++) {
            if (i > 0) fprintf(f, ",");
            fprintf(f, "%d", planCopy[i]);
        }
        fprintf(f, "\n");
    }

    fprintf(f, "numSavedRuns=%d\n", numSaved);

    for (int r = 0; r < numSaved; r++) {
        const SavedRun& sr = runs[r];
        fprintf(f, "run%dName=%s\n", r, sr.name);
        fprintf(f, "run%dN=%d\n",    r, sr.count);
        if (sr.count > 0) {
            fprintf(f, "run%d=", r);
            for (int i = 0; i < sr.count; i++) {
                if (i > 0) fprintf(f, ",");
                fprintf(f, "%d", sr.plan[i]);
            }
            fprintf(f, "\n");
        }
    }

    fclose(f);
}

void StateLoad(const char* addonDir)
{
    char path[512];
    BuildIniPath(path, sizeof(path), addonDir);

    FILE* f = fopen(path, "r");
    if (!f) return;

    char line[256];
    CritLock lk(&g_StateMutex);

    while (fgets(line, sizeof(line), f)) {
        int v;

        if (sscanf(line, "autoAnnounce=%d",     &v) == 1) g_State.cfg.autoAnnounce     = (v != 0);
        if (sscanf(line, "autoAdvanceOnMap=%d", &v) == 1) g_State.cfg.autoAdvanceOnMap = (v != 0);
        if (sscanf(line, "moveNotify=%d",       &v) == 1) g_State.moveNotifyEnabled    = (v != 0);
        if (sscanf(line, "startIdx=%d",         &v) == 1) g_State.cfg.startIdx = v % g_MetaCount;

        // Active plan
        if (sscanf(line, "planCount=%d", &v) == 1)
            g_State.loopPlanCount = (v < 0) ? 0 : (v > MAX_LOOP_PLAN) ? MAX_LOOP_PLAN : v;

        if (strncmp(line, "plan=", 5) == 0) {
            const char* p = line + 5;
            int n = 0, cap = g_State.loopPlanCount;
            while (*p && *p != '\n' && n < cap) {
                int idx = 0;
                if (sscanf(p, "%d", &idx) == 1) {
                    g_State.loopPlan[n++] = ((idx % g_MetaCount) + g_MetaCount) % g_MetaCount;
                    while (*p && *p != ',' && *p != '\n') p++;
                    if (*p == ',') p++;
                } else break;
            }
            g_State.loopPlanCount = n;
        }

        if (sscanf(line, "numSavedRuns=%d", &v) == 1)
            g_State.savedRunsCount = (v < 0) ? 0 : (v > MAX_SAVED_RUNS) ? MAX_SAVED_RUNS : v;

        // per-run entries: run<r>Name=..., run<r>N=<count>, run<r>=idx,...
        for (int r = 0; r < MAX_SAVED_RUNS; r++) {
            char fmtKey[40];

            // Name
            snprintf(fmtKey, sizeof(fmtKey), "run%dName=", r);
            if (strncmp(line, fmtKey, strlen(fmtKey)) == 0) {
                const char* val = line + strlen(fmtKey);
                size_t len = strlen(val);
                while (len > 0 && (val[len-1] == '\n' || val[len-1] == '\r')) len--;
                size_t cap = sizeof(g_State.savedRuns[r].name) - 1;
                if (len > cap) len = cap;
                memcpy(g_State.savedRuns[r].name, val, len);
                g_State.savedRuns[r].name[len] = '\0';
            }

            // Count
            snprintf(fmtKey, sizeof(fmtKey), "run%dN=%%d", r);
            if (sscanf(line, fmtKey, &v) == 1)
                g_State.savedRuns[r].count =
                    (v < 0) ? 0 : (v > MAX_LOOP_PLAN) ? MAX_LOOP_PLAN : v;

            // Plan indices
            snprintf(fmtKey, sizeof(fmtKey), "run%d=", r);
            if (strncmp(line, fmtKey, strlen(fmtKey)) == 0) {
                const char* p = line + strlen(fmtKey);
                int n = 0, cap2 = g_State.savedRuns[r].count;
                while (*p && *p != '\n' && n < cap2) {
                    int idx = 0;
                    if (sscanf(p, "%d", &idx) == 1) {
                        g_State.savedRuns[r].plan[n++] =
                            ((idx % g_MetaCount) + g_MetaCount) % g_MetaCount;
                        while (*p && *p != ',' && *p != '\n') p++;
                        if (*p == ',') p++;
                    } else break;
                }
                g_State.savedRuns[r].count = n;
            }
        }
    }
    fclose(f);

    g_State.currentIdx   = g_State.cfg.startIdx;
    g_State.loopStartIdx = g_State.cfg.startIdx;
}

void StateAdvance()
{
    CritLock lk(&g_StateMutex);
    if (g_State.loopPlanCount > 0) {
        g_State.loopPlanPos = (g_State.loopPlanPos + 1) % g_State.loopPlanCount;
        g_State.currentIdx  = g_State.loopPlan[g_State.loopPlanPos];
    } else {
        g_State.currentIdx = (g_State.currentIdx + 1) % g_MetaCount;
    }
    g_State.announceFired    = false;
    g_State.waypointUnlocked = false;
}

void StateSetCurrent(int idx)
{
    CritLock lk(&g_StateMutex);
    g_State.currentIdx       = idx % g_MetaCount;
    g_State.announceFired    = false;
    g_State.waypointUnlocked = false;
}

void StateToast(const char* msg)
{
    CritLock lk(&g_StateMutex);
    g_State.toastText  = msg;
    g_State.toastAlpha = 1.0f;
}
