#pragma once
#include <ctime>
#include <cstdint>

#define META_EVENT_COUNT 35

enum class SchedMode { Uniform, Irregular };

// ── per-event data ────────────────────────────────────────────────────────────
// Ley-Line Anomaly (event 9) rotates between 3 maps and has 3 waypoints.
// All other events use mapIds[0] / waypointCodes[0] only.
struct MetaEvent {
    int         id;               // 1-based, matches Meta-Train.txt line number
    const char* name;

    uint32_t    mapIds[3];        // GW2 map ID(s) — VERIFY ALL via /v2/maps/{id} (Lessons_Learned §30)
    int         mapIdCount;       // number of valid mapIds entries

    const char* waypointCodes[3]; // chat-link codes matching mapIds[]
    const char* squadMsg;         // body of the /d announcement (no "/d " prefix)

    int  durationMinutes;         // approximate event length
    int  prepMinutes;             // announce "heading to X" this many min before it starts

    SchedMode schedMode;
    int  cycleMinutes;            // Uniform: repeat period (120 or 180)
    int  offsetMinutes;           // Uniform: UTC minutes from midnight for first daily start
    int  irregTimes[8];           // Irregular: UTC minutes from midnight (up to 8 per day)
    int  irregCount;
};

// ── global event table ────────────────────────────────────────────────────────
// Indexed [0..META_EVENT_COUNT-1].  g_Events[i].id == i+1.
void EventsInit();   // must be called from AddonLoad before any other Events_* function
extern const MetaEvent* g_Events;  // pointer into the static table populated by EventsInit()

// ── timing helpers ────────────────────────────────────────────────────────────

// Seconds until the next start of ev from nowUtc.
// Returns 0 if ev is currently active (last start was within durationMinutes).
int    SecondsUntilNext(const MetaEvent& ev, time_t nowUtc);

// Seconds since the last start of ev.  Negative means it hasn't started yet in
// this window (equivalent to -SecondsUntilNext).
int    SecondsSinceLastStart(const MetaEvent& ev, time_t nowUtc);

// First UTC epoch when ev starts at or after notBefore.
time_t NextOccurrenceAfter(const MetaEvent& ev, time_t notBefore);

// Estimated UTC epoch when a full 27-event loop completes:
// starts at event[startIdx], chains each subsequent event after the previous ends,
// and wraps around once.
time_t ComputeLoopEndTime(int startIdx, time_t nowUtc);

// Returns true if mapId is one of ev's registered map IDs.
bool   IsEventMap(const MetaEvent& ev, uint32_t mapId);

// Gap in seconds between the end of evA (which started at aStart) and the
// beginning of the next occurrence of evB.  Returns 0 if evB overlaps or
// starts within evA's duration.
int    GapSeconds(const MetaEvent& evA, time_t aStart, const MetaEvent& evB);

// ── schedule preview ─────────────────────────────────────────────────────────
struct ScheduledOccurrence {
    int    eventIdx;     // index into g_Events[]
    time_t startTime;    // UTC epoch when this occurrence starts
    time_t endTime;      // UTC epoch when it ends (startTime + duration)
    int    gapBefore;    // seconds of gap/break before this event (after previous ends)
};

// Compute the full projected schedule for a complete loop (all META_EVENT_COUNT events)
// starting at event[startIdx] at or after fromTime.
// Results are written into out[META_EVENT_COUNT] in loop order.
void ComputeLoopSchedule(int startIdx, time_t fromTime,
                         ScheduledOccurrence out[META_EVENT_COUNT]);
