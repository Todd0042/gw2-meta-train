#pragma once
#include <ctime>
#include <cstdint>

static const int MAX_LOOP_PLAN = 32;

enum class EventExpansion { Core = 0, HoT = 1, PoF = 2, IBS = 3, EoD = 4, SotO = 5, JW = 6, VoE = 7 };
enum class EventCategory  { WorldBoss = 0, Meta = 1 };

struct MetaEvent {
    const char*    name;
    const char*    mapName;
    uint32_t       mapId;
    const char*    waypointCode;   // nullptr = no waypoint / TBD
    const char*    squadMsg;

    int            durationMinutes;
    int            prepMinutes;

    EventExpansion expansion;
    EventCategory  category;

    // Scheduling: intervalMinutes > 0 = uniform cycle, 0 = use irregTimes[]
    int            intervalMinutes;
    int            offsetMinutes;
    int            irregTimes[8];  // minutes from UTC midnight
    int            irregCount;
};

// Global event tables — defined in events.cpp
extern MetaEvent g_WorldBosses[];
extern int       g_WorldBossCount;
extern MetaEvent g_Metas[];
extern int       g_MetaCount;

void EventsInit();   // no-op, kept for API compatibility with dllmain

// ── Timing helpers ────────────────────────────────────────────────────────────
// Seconds until the next start of ev.  Returns 0 if ev is currently active.
int    SecondsUntilNext     (const MetaEvent& ev, time_t nowUtc);
// Positive = seconds since last start (may exceed duration = event over).
// Negative = -SecondsUntilNext (event not yet started in this cycle).
int    SecondsSinceLastStart(const MetaEvent& ev, time_t nowUtc);
// First UTC epoch when ev starts at or after notBefore.
time_t NextOccurrenceAfter  (const MetaEvent& ev, time_t notBefore);
// True if mapId matches ev's map.
bool   IsEventMap           (const MetaEvent& ev, uint32_t mapId);
// Seconds between end of evA (started at aStart) and next start of evB.
int    GapSeconds           (const MetaEvent& evA, time_t aStart, const MetaEvent& evB);

// ── Schedule preview ──────────────────────────────────────────────────────────
struct ScheduledOccurrence {
    int    eventIdx;   // index into g_Metas[]
    time_t startTime;
    time_t endTime;
    int    gapBefore;  // seconds of gap before this event
};

// Compute a chained schedule for a commander's custom plan.
// plan[] contains indices into g_Metas[]; out must have planCount entries.
void ComputePlanSchedule(const int* plan, int planCount, time_t fromTime,
                         ScheduledOccurrence* out);
