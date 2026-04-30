#include "events.h"
#include <algorithm>

// Map IDs verified via https://api.guildwars2.com/v2/maps
// Timings verified against https://wiki.guildwars2.com/wiki/World_boss_timer
// and community event schedule sources.
//
// IMPORTANT: offset = minutes from UTC midnight at which the cycle phase-aligns.
// The event_tracks.json source has a systematic +60 min bias for all 120-min cycle
// events and must NOT be used directly for offsets.

// ── Irregular daily schedules (minutes from UTC midnight) ─────────────────────
static const int s_tequatlTimes[]    = { 0, 180, 420, 690,  960, 1140, 0, 0 };
static const int s_tripleTimes[]     = { 60, 240, 480, 750, 1020, 1200, 0, 0 };
static const int s_karkaQueenTimes[] = { 120, 360, 630, 900, 1080, 1380, 0, 0 };

// ── World Boss table ──────────────────────────────────────────────────────────
MetaEvent g_WorldBosses[] = {
    // ── 2-hour cycle ──────────────────────────────────────────────────────────
    { "Svanir Shaman Chief", "Wayfarer Foothills", 28,
      "[&BMIDAAA=]",
      "Svanir Shaman Chief \xc2\xbb WP [&BMIDAAA=] | Wayfarer Foothills!",
      10, 5, EventExpansion::Core, EventCategory::WorldBoss,
      120, 15, {0,0,0,0,0,0,0,0}, 0 },

    { "Fire Elemental", "Metrica Province", 35,
      "[&BEcAAAA=]",
      "Fire Elemental \xc2\xbb WP [&BEcAAAA=] | Metrica Province!",
      8, 5, EventExpansion::Core, EventCategory::WorldBoss,
      120, 45, {0,0,0,0,0,0,0,0}, 0 },

    { "Great Jungle Wurm", "Caledon Forest", 34,
      "[&BEEFAAA=]",
      "Great Jungle Wurm \xc2\xbb WP [&BEEFAAA=] | Caledon Forest!",
      15, 8, EventExpansion::Core, EventCategory::WorldBoss,
      120, 75, {0,0,0,0,0,0,0,0}, 0 },

    { "Shadow Behemoth", "Queensdale", 15,
      "[&BPcAAAA=]",
      "Shadow Behemoth \xc2\xbb WP [&BPcAAAA=] | Queensdale!",
      15, 8, EventExpansion::Core, EventCategory::WorldBoss,
      120, 105, {0,0,0,0,0,0,0,0}, 0 },

    // ── 3-hour cycle ──────────────────────────────────────────────────────────
    { "Megadestroyer", "Mount Maelstrom", 39,
      "[&BM0CAAA=]",
      "Megadestroyer \xc2\xbb WP [&BM0CAAA=] | Mount Maelstrom!",
      15, 8, EventExpansion::Core, EventCategory::WorldBoss,
      180, 30, {0,0,0,0,0,0,0,0}, 0 },

    { "The Shatterer", "Blazeridge Steppes", 20,
      "[&BE4DAAA=]",
      "The Shatterer \xc2\xbb WP [&BE4DAAA=] | Blazeridge Steppes!",
      15, 8, EventExpansion::Core, EventCategory::WorldBoss,
      180, 60, {0,0,0,0,0,0,0,0}, 0 },

    { "Modniir Ulgoth", "Harathi Hinterlands", 17,
      "[&BLAAAAA=]",
      "Modniir Ulgoth \xc2\xbb WP [&BLAAAAA=] | Harathi Hinterlands!",
      15, 8, EventExpansion::Core, EventCategory::WorldBoss,
      180, 90, {0,0,0,0,0,0,0,0}, 0 },

    { "Golem Mark II", "Brisban Wildlands", 54,
      "[&BNQCAAA=]",
      "Golem Mark II \xc2\xbb WP [&BNQCAAA=] | Brisban Wildlands!",
      15, 8, EventExpansion::Core, EventCategory::WorldBoss,
      180, 120, {0,0,0,0,0,0,0,0}, 0 },

    { "Claw of Jormag", "Frostgorge Sound", 30,
      "[&BHoCAAA=]",
      "Claw of Jormag \xc2\xbb WP [&BHoCAAA=] | Frostgorge Sound!",
      20, 10, EventExpansion::Core, EventCategory::WorldBoss,
      180, 150, {0,0,0,0,0,0,0,0}, 0 },

    // ── Irregular daily schedule ───────────────────────────────────────────────
    { "Tequatl the Sunless", "Sparkfly Fen", 53,
      "[&BNABAAA=]",
      "Tequatl \xc2\xbb WP [&BNABAAA=] | Sparkfly Fen!",
      15, 10, EventExpansion::Core, EventCategory::WorldBoss,
      0, 0, {0, 180, 420, 690, 960, 1140, 0, 0}, 6 },

    { "Triple Trouble", "Bloodtide Coast", 73,
      "[&BKoBAAA=]",
      "Triple Trouble \xc2\xbb WP [&BKoBAAA=] | Bloodtide Coast!",
      25, 10, EventExpansion::Core, EventCategory::WorldBoss,
      0, 0, {60, 240, 480, 750, 1020, 1200, 0, 0}, 6 },

    { "Taidha Covington", "Bloodtide Coast", 73,
      "[&BKgBAAA=]",
      "Taidha Covington \xc2\xbb WP [&BKgBAAA=] | Bloodtide Coast!",
      15, 8, EventExpansion::Core, EventCategory::WorldBoss,
      180, 0, {0,0,0,0,0,0,0,0}, 0 },

    { "Karka Queen", "Southsun Cove", 873,
      "[&BNcGAAA=]",
      "Karka Queen \xc2\xbb WP [&BNcGAAA=] | Southsun Cove!",
      15, 8, EventExpansion::Core, EventCategory::WorldBoss,
      0, 0, {120, 360, 630, 900, 1080, 1380, 0, 0}, 6 },
};
int g_WorldBossCount = (int)(sizeof(g_WorldBosses) / sizeof(g_WorldBosses[0]));

// ── Meta event table ──────────────────────────────────────────────────────────
MetaEvent g_Metas[] = {
    // ── Heart of Thorns ───────────────────────────────────────────────────────
    { "Night Bosses", "Verdant Brink", 1042,
      "[&BAgIAAA=]",
      "Night Bosses \xc2\xbb WP [&BAgIAAA=] | Verdant Brink!",
      30, 10, EventExpansion::HoT, EventCategory::Meta,
      120, 10, {0,0,0,0,0,0,0,0}, 0 },

    { "Chak Gerent", "Tangled Depths", 1045,
      "[&BPUHAAA=]",
      "Chak Gerent \xc2\xbb WP [&BPUHAAA=] | Tangled Depths!",
      20, 10, EventExpansion::HoT, EventCategory::Meta,
      120, 30, {0,0,0,0,0,0,0,0}, 0 },

    { "Octovine", "Auric Basin", 1043,
      "[&BN0HAAA=]",
      "Octovine (Battle in Tarir) \xc2\xbb WP [&BN0HAAA=] | Auric Basin!",
      20, 10, EventExpansion::HoT, EventCategory::Meta,
      120, 60, {0,0,0,0,0,0,0,0}, 0 },

    { "Dragon's Stand", "Dragon's Stand", 1041,
      "[&BBAIAAA=]",
      "Dragon's Stand \xc2\xbb WP [&BBAIAAA=] | Dragon's Stand!",
      90, 15, EventExpansion::HoT, EventCategory::Meta,
      120, 90, {0,0,0,0,0,0,0,0}, 0 },

    // ── Living World Season 3 ─────────────────────────────────────────────────
    { "Saidra's Haven", "Lake Doric", 0,
      "[&BK0JAAA=]",
      "Saidra's Haven \xc2\xbb WP [&BK0JAAA=] | Lake Doric!",
      45, 10, EventExpansion::HoT, EventCategory::Meta,
      120, 60, {0,0,0,0,0,0,0,0}, 0 },

    { "New Loamhurst", "Lake Doric", 0,
      "[&BLQJAAA=]",
      "New Loamhurst \xc2\xbb WP [&BLQJAAA=] | Lake Doric!",
      45, 10, EventExpansion::HoT, EventCategory::Meta,
      120, 105, {0,0,0,0,0,0,0,0}, 0 },

    { "Noran's Homestead", "Lake Doric", 0,
      "[&BK8JAAA=]",
      "Noran's Homestead \xc2\xbb WP [&BK8JAAA=] | Lake Doric!",
      30, 10, EventExpansion::HoT, EventCategory::Meta,
      120, 30, {0,0,0,0,0,0,0,0}, 0 },

    // ── Path of Fire ──────────────────────────────────────────────────────────
    { "Casino Blitz", "Crystal Oasis", 1210,
      "[&BLsKAAA=]",
      "Casino Blitz \xc2\xbb WP [&BLsKAAA=] | Crystal Oasis!",
      15, 8, EventExpansion::PoF, EventCategory::Meta,
      120, 5, {0,0,0,0,0,0,0,0}, 0 },

    { "Buried Treasure", "Desert Highlands", 0,
      "[&BGsKAAA=]",
      "Buried Treasure \xc2\xbb WP [&BGsKAAA=] | Desert Highlands!",
      20, 8, EventExpansion::PoF, EventCategory::Meta,
      120, 60, {0,0,0,0,0,0,0,0}, 0 },

    { "Path to Ascension", "Elon Riverlands", 1228,
      "[&BFMKAAA=]",
      "Path to Ascension \xc2\xbb WP [&BFMKAAA=] | Elon Riverlands!",
      25, 10, EventExpansion::PoF, EventCategory::Meta,
      120, 90, {0,0,0,0,0,0,0,0}, 0 },

    { "Doppelganger", "Elon Riverlands", 1228,
      "[&BFMKAAA=]",
      "Doppelganger \xc2\xbb WP [&BFMKAAA=] | Elon Riverlands!",
      20, 10, EventExpansion::PoF, EventCategory::Meta,
      120, 115, {0,0,0,0,0,0,0,0}, 0 },

    { "Junundu Rising", "The Desolation", 1226,
      "[&BMEKAAA=]",
      "Junundu Rising \xc2\xbb WP [&BMEKAAA=] | The Desolation!",
      20, 8, EventExpansion::PoF, EventCategory::Meta,
      60, 30, {0,0,0,0,0,0,0,0}, 0 },

    { "Maws of Torment", "The Desolation", 1226,
      "[&BKMKAAA=]",
      "Maws of Torment \xc2\xbb WP [&BKMKAAA=] | The Desolation!",
      15, 8, EventExpansion::PoF, EventCategory::Meta,
      120, 60, {0,0,0,0,0,0,0,0}, 0 },

    { "Forged with Fire", "Domain of Vabbi", 1248,
      "[&BO0KAAA=]",
      "Forged with Fire \xc2\xbb WP [&BO0KAAA=] | Domain of Vabbi!",
      20, 8, EventExpansion::PoF, EventCategory::Meta,
      60, 0, {0,0,0,0,0,0,0,0}, 0 },

    { "Serpent's Ire", "Domain of Vabbi", 1248,
      "[&BHQKAAA=]",
      "Serpent's Ire \xc2\xbb WP [&BHQKAAA=] | Domain of Vabbi!",
      20, 10, EventExpansion::PoF, EventCategory::Meta,
      120, 30, {0,0,0,0,0,0,0,0}, 0 },

    { "Palawadan", "Domain of Istan", 1263,
      "[&BAkLAAA=]",
      "Palawadan \xc2\xbb WP [&BAkLAAA=] | Domain of Istan!",
      15, 8, EventExpansion::PoF, EventCategory::Meta,
      120, 105, {0,0,0,0,0,0,0,0}, 0 },

    { "DB Shatterer", "Jahai Bluffs", 1301,
      "[&BJMLAAA=]",
      "Death-Branded Shatterer \xc2\xbb WP [&BJMLAAA=] | Jahai Bluffs!",
      15, 8, EventExpansion::PoF, EventCategory::Meta,
      120, 75, {0,0,0,0,0,0,0,0}, 0 },

    // ── Icebrood Saga (incl. LW4/LW5) ────────────────────────────────────────
    { "Thunderhead Keep", "Thunderhead Peaks", 1310,
      "[&BLsLAAA=]",
      "Thunderhead Keep \xc2\xbb WP [&BLsLAAA=] | Thunderhead Peaks!",
      20, 10, EventExpansion::IBS, EventCategory::Meta,
      120, 105, {0,0,0,0,0,0,0,0}, 0 },

    { "Oil Floes", "Thunderhead Peaks", 1310,
      "[&BKYLAAA=]",
      "The Oil Floes \xc2\xbb WP [&BKYLAAA=] | Thunderhead Peaks!",
      20, 8, EventExpansion::IBS, EventCategory::Meta,
      120, 45, {0,0,0,0,0,0,0,0}, 0 },

    { "Metal Concert", "Grothmar Valley", 1330,
      "[&BPgLAAA=]",
      "Metal Concert \xc2\xbb WP [&BPgLAAA=] | Grothmar Valley!",
      20, 10, EventExpansion::IBS, EventCategory::Meta,
      120, 100, {0,0,0,0,0,0,0,0}, 0 },

    { "Drakkar", "Bjora Marches", 1343,
      "[&BDkMAAA=]",
      "Drakkar \xc2\xbb WP [&BDkMAAA=] | Bjora Marches!",
      20, 10, EventExpansion::IBS, EventCategory::Meta,
      120, 80, {0,0,0,0,0,0,0,0}, 0 },

    { "Twisted Marionette", "Lion's Arch Aerodrome", 0,
      "[&BAkMAAA=]",
      "Twisted Marionette \xc2\xbb WP [&BAkMAAA=] | Lion's Arch Aerodrome!",
      20, 10, EventExpansion::IBS, EventCategory::Meta,
      120, 0, {0,0,0,0,0,0,0,0}, 0 },

    { "Dragonstorm", "Drizzlewood Coast", 0,
      "[&BAkMAAA=]",
      "Dragonstorm \xc2\xbb WP [&BAkMAAA=] | Lion's Arch Aerodrome!",
      20, 10, EventExpansion::IBS, EventCategory::Meta,
      120, 60, {0,0,0,0,0,0,0,0}, 0 },

    // ── End of Dragons ────────────────────────────────────────────────────────
    { "Dragon's End", "Dragon's End", 1422,
      "[&BKIMAAA=]",
      "Dragon's End \xc2\xbb WP [&BKIMAAA=] | Dragon's End!",
      30, 15, EventExpansion::EoD, EventCategory::Meta,
      120, 60, {0,0,0,0,0,0,0,0}, 0 },

    { "Aetherblade Assault", "Seitung Province", 1442,
      "[&BGUNAAA=]",
      "Aetherblade Assault \xc2\xbb WP [&BGUNAAA=] | Seitung Province!",
      20, 10, EventExpansion::EoD, EventCategory::Meta,
      120, 90, {0,0,0,0,0,0,0,0}, 0 },

    { "Kaineng Blackout", "New Kaineng City", 1438,
      "[&BBkNAAA=]",
      "Kaineng Blackout \xc2\xbb WP [&BBkNAAA=] | New Kaineng City!",
      25, 10, EventExpansion::EoD, EventCategory::Meta,
      120, 0, {0,0,0,0,0,0,0,0}, 0 },

    { "Gang War", "The Echovald Wilds", 1452,
      "[&BMwMAAA=]",
      "Gang War \xc2\xbb WP [&BMwMAAA=] | The Echovald Wilds!",
      20, 8, EventExpansion::EoD, EventCategory::Meta,
      120, 30, {0,0,0,0,0,0,0,0}, 0 },

    { "Aspenwood", "The Echovald Wilds", 1452,
      "[&BPkMAAA=]",
      "Aspenwood \xc2\xbb WP [&BPkMAAA=] | The Echovald Wilds!",
      20, 8, EventExpansion::EoD, EventCategory::Meta,
      120, 100, {0,0,0,0,0,0,0,0}, 0 },

    // ── Secrets of the Obscure ────────────────────────────────────────────────
    { "Wizard's Tower", "Skywatch Archipelago", 1510,
      "[&BL4NAAA=]",
      "Wizard's Tower \xc2\xbb WP [&BL4NAAA=] | Skywatch Archipelago!",
      20, 10, EventExpansion::SotO, EventCategory::Meta,
      120, 60, {0,0,0,0,0,0,0,0}, 0 },

    { "Defense of Amnytas", "Amnytas", 1517,
      "[&BDQOAAA=]",
      "Defense of Amnytas \xc2\xbb WP [&BDQOAAA=] | Amnytas!",
      20, 10, EventExpansion::SotO, EventCategory::Meta,
      120, 0, {0,0,0,0,0,0,0,0}, 0 },

    { "Outer Nayos", "Outer Nayos", 0,
      "[&BB8OAAA=]",
      "Outer Nayos Convergence \xc2\xbb WP [&BB8OAAA=] | Outer Nayos!",
      10, 8, EventExpansion::SotO, EventCategory::Meta,
      180, 90, {0,0,0,0,0,0,0,0}, 0 },

    // ── Janthir Wilds ─────────────────────────────────────────────────────────
    { "Of Mists and Monsters", "Janthir Syntri", 0,
      "[&BCoPAAA=]",
      "Of Mists and Monsters \xc2\xbb WP [&BCoPAAA=] | Janthir Syntri!",
      25, 10, EventExpansion::JW, EventCategory::Meta,
      120, 30, {0,0,0,0,0,0,0,0}, 0 },

    { "A Titanic Voyage", "Bava Nisos", 0,
      "[&BGEPAAA=]",
      "A Titanic Voyage \xc2\xbb WP [&BGEPAAA=] | Bava Nisos!",
      30, 10, EventExpansion::JW, EventCategory::Meta,
      120, 80, {0,0,0,0,0,0,0,0}, 0 },

    { "Mount Balrior", "Mount Balrior", 0,
      "[&BK4OAAA=]",
      "Mount Balrior Convergence \xc2\xbb WP [&BK4OAAA=] | Mount Balrior!",
      10, 8, EventExpansion::JW, EventCategory::Meta,
      180, 0, {0,0,0,0,0,0,0,0}, 0 },

    // ── Visions of Eternity ───────────────────────────────────────────────────
    { "Hammerhart Rumble!", "Shipwreck Strand", 0,
      "[&BJEPAAA=]",
      "Hammerhart Rumble! \xc2\xbb WP [&BJEPAAA=] | Shipwreck Strand!",
      15, 8, EventExpansion::VoE, EventCategory::Meta,
      120, 40, {0,0,0,0,0,0,0,0}, 0 },

    { "Secrets of the Weald", "Starlit Weald", 0,
      "[&BJ4PAAA=]",
      "Secrets of the Weald \xc2\xbb WP [&BJ4PAAA=] | Starlit Weald!",
      20, 10, EventExpansion::VoE, EventCategory::Meta,
      120, 105, {0,0,0,0,0,0,0,0}, 0 },
};
int g_MetaCount = (int)(sizeof(g_Metas) / sizeof(g_Metas[0]));

void EventsInit() {}

// ── Timing engine ─────────────────────────────────────────────────────────────

static int SecsIntoDay(time_t t)
{
    struct tm utc;
#ifdef _WIN32
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif
    return utc.tm_hour * 3600 + utc.tm_min * 60 + utc.tm_sec;
}

static int UniformSecsUntilNext(const MetaEvent& ev, time_t nowUtc)
{
    int s      = SecsIntoDay(nowUtc);
    int cycSec = ev.intervalMinutes * 60;
    int offSec = (ev.offsetMinutes * 60) % cycSec;
    int durSec = ev.durationMinutes * 60;

    int posInCycle = ((s - offSec) % cycSec + cycSec) % cycSec;
    if (posInCycle < durSec)
        return 0;
    return cycSec - posInCycle;
}

static int IrregSecsUntilNext(const MetaEvent& ev, time_t nowUtc)
{
    int s      = SecsIntoDay(nowUtc);
    int durSec = ev.durationMinutes * 60;
    int best   = 86400 + 1;

    for (int i = 0; i < ev.irregCount; i++) {
        int t    = ev.irregTimes[i] * 60;
        int diff = (t - s + 86400) % 86400;
        if (diff == 0 || (s > t && s < t + durSec))
            return 0;
        if (diff > 0 && diff < best)
            best = diff;
    }
    return (best > 86400) ? 86400 : best;
}

int SecondsUntilNext(const MetaEvent& ev, time_t nowUtc)
{
    if (ev.intervalMinutes > 0)
        return UniformSecsUntilNext(ev, nowUtc);
    return IrregSecsUntilNext(ev, nowUtc);
}

int SecondsSinceLastStart(const MetaEvent& ev, time_t nowUtc)
{
    int until = SecondsUntilNext(ev, nowUtc);
    if (until == 0) {
        int s = SecsIntoDay(nowUtc);
        if (ev.intervalMinutes > 0) {
            int cycSec = ev.intervalMinutes * 60;
            int offSec = (ev.offsetMinutes * 60) % cycSec;
            return ((s - offSec) % cycSec + cycSec) % cycSec;
        } else {
            int best = -1;
            for (int i = 0; i < ev.irregCount; i++) {
                int t   = ev.irregTimes[i] * 60;
                int age = (s - t + 86400) % 86400;
                if (age <= ev.durationMinutes * 60 && (best < 0 || age < best))
                    best = age;
            }
            return (best >= 0) ? best : 0;
        }
    }
    return -until;
}

time_t NextOccurrenceAfter(const MetaEvent& ev, time_t notBefore)
{
    for (int pass = 0; pass < 2; pass++) {
        time_t probe = notBefore + (time_t)(pass * 86400);
        int until = SecondsUntilNext(ev, probe);
        if (until == 0) {
            int since = SecondsSinceLastStart(ev, probe);
            return probe - since;
        }
        time_t candidate = probe + (time_t)until;
        if (candidate >= notBefore)
            return candidate;
    }
    return notBefore + 86400;
}

bool IsEventMap(const MetaEvent& ev, uint32_t mapId)
{
    return ev.mapId == mapId;
}

int GapSeconds(const MetaEvent& evA, time_t aStart, const MetaEvent& evB)
{
    time_t aEnd   = aStart + (time_t)(evA.durationMinutes * 60);
    time_t bStart = NextOccurrenceAfter(evB, aEnd);
    int gap       = (int)(bStart - aEnd);
    return gap < 0 ? 0 : gap;
}

void ComputePlanSchedule(const int* plan, int planCount, time_t fromTime,
                         ScheduledOccurrence* out)
{
    time_t cursor  = fromTime;
    time_t prevEnd = fromTime;

    for (int i = 0; i < planCount; i++) {
        const MetaEvent& ev = g_Metas[plan[i]];
        time_t start  = NextOccurrenceAfter(ev, cursor);
        time_t end    = start + (time_t)(ev.durationMinutes * 60);
        int    gap    = (int)(start - prevEnd);

        out[i].eventIdx  = plan[i];
        out[i].startTime = start;
        out[i].endTime   = end;
        out[i].gapBefore = gap < 0 ? 0 : gap;

        prevEnd = end;
        cursor  = end;
    }
}
