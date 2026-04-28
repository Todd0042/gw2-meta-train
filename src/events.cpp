#include "events.h"
#include <cstring>
#include <algorithm>

// ═════════════════════════════════════════════════════════════════════════════
//  MAP IDs — verify via https://api.guildwars2.com/v2/maps/{id}
//  Confirmed (§30): Verdant Brink=1042, Auric Basin=1043, Tangled Depths=1045
// ═════════════════════════════════════════════════════════════════════════════
static constexpr uint32_t MAP_FROSTGORGE      = 227;   // verify
static constexpr uint32_t MAP_WAYFARER        = 18;    // verify (Svanir Shaman Chief)
static constexpr uint32_t MAP_SPARKFLY        = 53;    // verify (Tequatl)
static constexpr uint32_t MAP_CALEDON         = 34;    // verify (Great Jungle Wurm)
static constexpr uint32_t MAP_SOUTHSUN        = 873;   // verify (Karka Queen)
static constexpr uint32_t MAP_MAELSTROM       = 27;    // verify (Megadestroyer)
static constexpr uint32_t MAP_BLOODTIDE       = 50;    // verify (Triple Trouble)
static constexpr uint32_t MAP_BLAZERIDGE      = 17;    // verify (The Shatterer)
static constexpr uint32_t MAP_EYE_NORTH       = 1346;  // verify (Twisted Marionette / Dragonstorm)
static constexpr uint32_t MAP_METRICA         = 39;    // verify (Fire Elemental)
static constexpr uint32_t MAP_VERDANT         = 1042;  // confirmed §30
static constexpr uint32_t MAP_AURIC           = 1043;  // confirmed §30
static constexpr uint32_t MAP_TANGLED         = 1045;  // confirmed §30
static constexpr uint32_t MAP_TIMBERLINE      = 250;   // verify (Ley-Line rotation 1)
static constexpr uint32_t MAP_IRON_MARCHES    = 15;    // verify (Ley-Line rotation 2)
static constexpr uint32_t MAP_GENDARRAN       = 112;   // verify (Ley-Line rotation 3)
static constexpr uint32_t MAP_VABBI           = 1248;  // verify (Crystal Oasis / Vabbi)
static constexpr uint32_t MAP_ELON            = 1210;  // verify (Elon Riverlands)
static constexpr uint32_t MAP_DESOLATION      = 1235;  // verify (The Desolation)
static constexpr uint32_t MAP_ISTAN           = 1268;  // verify (Domain of Istan)
static constexpr uint32_t MAP_JAHAI           = 1271;  // verify (Jahai Bluffs)
static constexpr uint32_t MAP_SANDSWEPT       = 1301;  // verify (Sandswept Isles)
static constexpr uint32_t MAP_GROTHMAR        = 1343;  // verify (Grothmar Valley)
static constexpr uint32_t MAP_QUEENSDALE      = 15;    // verify (Shadow Behemoth) — same as MAP_IRON_MARCHES value; both need verification
static constexpr uint32_t MAP_BJORA           = 1371;  // verify (Bjora Marches)
static constexpr uint32_t MAP_SEITUNG         = 1428;  // verify (Seitung Province)
static constexpr uint32_t MAP_NEW_KAINENG     = 1422;  // verify (New Kaineng City)
static constexpr uint32_t MAP_ECHOVALD        = 1432;  // verify (Echovald Wilds)
static constexpr uint32_t MAP_DRAGONS_END     = 1438;  // verify (Dragon's End)

// ═════════════════════════════════════════════════════════════════════════════
//  WAYPOINT CODES — sourced from events.json (Snipppppy/GW2-Simple-Timer)
//  Confirmed in events.json:  [C]  Confirmed community/events.json.
//  TODO: verify all in-game and update if any differ.
// ═════════════════════════════════════════════════════════════════════════════

// ═════════════════════════════════════════════════════════════════════════════
//  EVENT TABLE  (35 events: 27 original + 8 world-boss fill events [F])
//
//  Timing corrections vs. prior version (source: events.json §30):
//   • Claw of Jormag   offset 150 (2:30 UTC) not 30
//   • Karka Queen      irregular times updated
//   • Triple Trouble   irregular times updated
//   • Night/Enemy      offset 10 (0:10 UTC) — verify in-game
//   • Path/Ascension   offset 90 (1:30 UTC) not 0
//   • Palawadan        offset 105 (1:45 UTC) not 0
//   • Oil Floes        offset 45 (0:45 UTC) not 0
//   • Flame Effigy     offset 10 not 0
//   • Doomlore         offset 40 not 15
//   • Ooze Pits        offset 65 (1:05 UTC) not 30
//   • Drakkar          offset 80 (1:20 UTC) not 90
//   • Aetherblade      offset 90 (1:30 UTC) not 0
//   • Aspenwood        offset 100 (1:40 UTC) not 60
//   • Gang War         offset 30 not 0
//   • Many WP codes updated from events.json
// ═════════════════════════════════════════════════════════════════════════════
static MetaEvent s_Events[META_EVENT_COUNT];

const MetaEvent* g_Events = s_Events;

void EventsInit()
{
    // ── 1 — Breaking the Claw of Jormag ──────────────────────────────────────
    // 3h cycle; first start 2:30 UTC → 2:30, 5:30, 8:30… (events.json confirmed)
    s_Events[0] = {
        1, "Breaking the Claw of Jormag",
        {MAP_FROSTGORGE, 0u, 0u}, 1,
        {"[&BH0CAAA=]", nullptr, nullptr},
        "Claw of Jormag \xc2\xbb WP [&BH0CAAA=] | Frostgorge Sound!",
        20, 10,
        SchedMode::Uniform, 180, 150,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 2 — Svanir Shaman Chief  [FILL — world boss] ─────────────────────────
    // 2h cycle, 0:15 UTC → 0:15, 2:15, 4:15… (events.json)
    s_Events[1] = {
        2, "Svanir Shaman Chief",
        {MAP_WAYFARER, 0u, 0u}, 1,
        {"[&BH4BAAA=]", nullptr, nullptr},
        "Svanir Shaman Chief \xc2\xbb WP [&BH4BAAA=] | Wayfarer Foothills!",
        10, 5,
        SchedMode::Uniform, 120, 15,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 3 — Tequatl the Sunless ───────────────────────────────────────────────
    // Irregular (events.json): 00:00, 03:00, 07:00, 11:30, 16:00, 19:00 UTC
    s_Events[2] = {
        3, "Tequatl the Sunless",
        {MAP_SPARKFLY, 0u, 0u}, 1,
        {"[&BNABAAA=]", nullptr, nullptr},
        "Tequatl \xc2\xbb WP [&BNABAAA=] | Sparkfly Fen!",
        15, 10,
        SchedMode::Irregular, 0, 0,
        {0, 180, 420, 690, 960, 1140, 0, 0}, 6
    };

    // ── 4 — Great Jungle Wurm  [FILL — world boss] ───────────────────────────
    // 2h cycle, 1:15 UTC → 1:15, 3:15, 5:15… (events.json)
    s_Events[3] = {
        4, "Great Jungle Wurm",
        {MAP_CALEDON, 0u, 0u}, 1,
        {"[&BEEFAAA=]", nullptr, nullptr},
        "Great Jungle Wurm \xc2\xbb WP [&BEEFAAA=] | Caledon Forest!",
        15, 8,
        SchedMode::Uniform, 120, 75,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 5 — Karka Queen ───────────────────────────────────────────────────────
    // Irregular (events.json): 02:00, 06:00, 10:30, 15:00, 18:00, 23:00 UTC
    s_Events[4] = {
        5, "Karka Queen",
        {MAP_SOUTHSUN, 0u, 0u}, 1,
        {"[&BNcGAAA=]", nullptr, nullptr},
        "Karka Queen \xc2\xbb WP [&BNcGAAA=] | Southsun Cove!",
        15, 8,
        SchedMode::Irregular, 0, 0,
        {120, 360, 630, 900, 1080, 1380, 0, 0}, 6
    };

    // ── 6 — Megadestroyer  [FILL — world boss] ───────────────────────────────
    // 3h cycle, 0:30 UTC → 0:30, 3:30, 6:30… (events.json)
    s_Events[5] = {
        6, "Megadestroyer",
        {MAP_MAELSTROM, 0u, 0u}, 1,
        {"[&BM0CAAA=]", nullptr, nullptr},
        "Megadestroyer \xc2\xbb WP [&BM0CAAA=] | Mount Maelstrom!",
        15, 8,
        SchedMode::Uniform, 180, 30,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 7 — Triple Trouble ────────────────────────────────────────────────────
    // Irregular (events.json): 01:00, 04:00, 08:00, 12:30, 17:00, 20:00 UTC
    s_Events[6] = {
        7, "Triple Trouble",
        {MAP_BLOODTIDE, 0u, 0u}, 1,
        {"[&BKoBAAA=]", nullptr, nullptr},
        "Triple Trouble \xc2\xbb WP [&BKoBAAA=] | Bloodtide Coast!",
        25, 10,
        SchedMode::Irregular, 0, 0,
        {60, 240, 480, 750, 1020, 1200, 0, 0}, 6
    };

    // ── 8 — The Shatterer  [FILL — world boss] ───────────────────────────────
    // 3h cycle, 1:00 UTC → 1:00, 4:00, 7:00… (events.json)
    s_Events[7] = {
        8, "The Shatterer",
        {MAP_BLAZERIDGE, 0u, 0u}, 1,
        {"[&BE4DAAA=]", nullptr, nullptr},
        "The Shatterer \xc2\xbb WP [&BE4DAAA=] | Blazeridge Steppes!",
        15, 8,
        SchedMode::Uniform, 180, 60,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 9 — Twisted Marionette ────────────────────────────────────────────────
    // 2h cycle, every even hour (events.json)
    s_Events[8] = {
        9, "Twisted Marionette",
        {MAP_EYE_NORTH, 0u, 0u}, 1,
        {"[&BAkMAAA=]", nullptr, nullptr},
        "Twisted Marionette \xc2\xbb WP [&BAkMAAA=] | Eye of the North!",
        20, 10,
        SchedMode::Uniform, 120, 0,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 10 — Fire Elemental  [FILL — world boss] ─────────────────────────────
    // 2h cycle, 0:45 UTC → 0:45, 2:45, 4:45… (events.json)
    s_Events[9] = {
        10, "Fire Elemental",
        {MAP_METRICA, 0u, 0u}, 1,
        {"[&BEcAAAA=]", nullptr, nullptr},
        "Fire Elemental \xc2\xbb WP [&BEcAAAA=] | Metrica Province!",
        8, 5,
        SchedMode::Uniform, 120, 45,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 11 — Night and the Enemy ──────────────────────────────────────────────
    // Verdant Brink night meta.  events.json lists night boss phase at :10.
    // Verify: VB 2h cycle — daytime ~0:00, night phase ~0:10 UTC (0:10, 2:10…)
    s_Events[10] = {
        11, "Night and the Enemy",
        {MAP_VERDANT, 0u, 0u}, 1,
        {"[&BAgIAAA=]", nullptr, nullptr},
        "Night and the Enemy \xc2\xbb WP [&BAgIAAA=] | Verdant Brink!",
        30, 10,
        SchedMode::Uniform, 120, 10,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 12 — Battle in Tarir ─────────────────────────────────────────────────
    // 2h cycle, 1:00 UTC → 1:00, 3:00… [C] WP confirmed
    s_Events[11] = {
        12, "Battle in Tarir",
        {MAP_AURIC, 0u, 0u}, 1,
        {"[&BN0HAAA=]", nullptr, nullptr},
        "Battle in Tarir \xc2\xbb WP [&BN0HAAA=] | Auric Basin!",
        25, 10,
        SchedMode::Uniform, 120, 60,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 13 — Chak Gerent ─────────────────────────────────────────────────────
    // 2h cycle, 0:30 UTC → 0:30, 2:30…
    s_Events[12] = {
        13, "Chak Gerent",
        {MAP_TANGLED, 0u, 0u}, 1,
        {"[&BPUHAAA=]", nullptr, nullptr},
        "Chak Gerent \xc2\xbb WP [&BPUHAAA=] | Tangled Depths!",
        20, 10,
        SchedMode::Uniform, 120, 30,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 14 — Ley-Line Anomaly ─────────────────────────────────────────────────
    // Rotates across 3 maps every 2h at :20. Show all 3 WPs; commander picks active.
    // Timberline: 0:20, 6:20, 12:20, 18:20  |  Iron Marches: 2:20, 8:20…
    // Gendarran: 4:20, 10:20…  → combined any-map occurrence every 2h at :20
    s_Events[13] = {
        14, "Ley-Line Anomaly",
        {MAP_TIMBERLINE, MAP_IRON_MARCHES, MAP_GENDARRAN}, 3,
        {"[&BEwCAAA=]", "[&BOcBAAA=]", "[&BOQAAAA=]"},
        "Ley-Line Anomaly \xc2\xbb Check active map: "
        "Timberline [&BEwCAAA=] | Iron Marches [&BOcBAAA=] | Gendarran [&BOQAAAA=]",
        10, 5,
        SchedMode::Uniform, 120, 20,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 15 — Choya Pinata / Casino Blitz ─────────────────────────────────────
    // 2h cycle, 0:21 UTC (events.json). Crystal Oasis, Domain of Vabbi staging area.
    s_Events[14] = {
        15, "Choya Pinata",
        {MAP_VABBI, 0u, 0u}, 1,
        {"[&BLsKAAA=]", nullptr, nullptr},
        "Choya Pinata / Casino Blitz \xc2\xbb WP [&BLsKAAA=] | Crystal Oasis!",
        15, 8,
        SchedMode::Uniform, 120, 21,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 16 — The Path to Ascension ───────────────────────────────────────────
    // 2h cycle, 1:30 UTC → 1:30, 3:30… (events.json, corrected from :00)
    s_Events[15] = {
        16, "The Path to Ascension",
        {MAP_ELON, 0u, 0u}, 1,
        {"[&BFMKAAA=]", nullptr, nullptr},
        "The Path to Ascension \xc2\xbb WP [&BFMKAAA=] | Elon Riverlands!",
        20, 10,
        SchedMode::Uniform, 120, 90,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 17 — Junundu Rising ──────────────────────────────────────────────────
    // 2h cycle, 0:30 UTC → 0:30, 2:30… (events.json)
    s_Events[16] = {
        17, "Junundu Rising",
        {MAP_DESOLATION, 0u, 0u}, 1,
        {"[&BMEKAAA=]", nullptr, nullptr},
        "Junundu Rising \xc2\xbb WP [&BMEKAAA=] | The Desolation!",
        20, 10,
        SchedMode::Uniform, 120, 30,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 18 — Maws of Torment  [FILL — PoF world boss] ────────────────────────
    // The Desolation. 2h cycle, 1:00 UTC → 1:00, 3:00, 5:00… (events.json)
    s_Events[17] = {
        18, "Maws of Torment",
        {MAP_DESOLATION, 0u, 0u}, 1,
        {"[&BKMKAAA=]", nullptr, nullptr},
        "Maws of Torment \xc2\xbb WP [&BKMKAAA=] | The Desolation!",
        15, 8,
        SchedMode::Uniform, 120, 60,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 19 — Forged with Fire ────────────────────────────────────────────────
    // 2h cycle, even hours 0:00, 2:00… Domain of Vabbi meta chain start.
    s_Events[18] = {
        19, "Forged with Fire",
        {MAP_VABBI, 0u, 0u}, 1,
        {"[&BO0KAAA=]", nullptr, nullptr},
        "Forged with Fire \xc2\xbb WP [&BO0KAAA=] | Domain of Vabbi!",
        15, 8,
        SchedMode::Uniform, 120, 0,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 20 — Serpents' Ire  [FILL — PoF event] ───────────────────────────────
    // Domain of Vabbi. 2h cycle, 0:30 UTC → 0:30, 2:30… (events.json)
    s_Events[19] = {
        20, "Serpents' Ire",
        {MAP_VABBI, 0u, 0u}, 1,
        {"[&BHQKAAA=]", nullptr, nullptr},
        "Serpents' Ire \xc2\xbb WP [&BHQKAAA=] | Domain of Vabbi!",
        20, 10,
        SchedMode::Uniform, 120, 30,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 21 — Palawadan, Jewel of Istan ───────────────────────────────────────
    // 2h cycle, 1:45 UTC → 1:45, 3:45… (events.json, corrected from :00)
    s_Events[20] = {
        21, "Palawadan, Jewel of Istan",
        {MAP_ISTAN, 0u, 0u}, 1,
        {"[&BAkLAAA=]", nullptr, nullptr},
        "Palawadan \xc2\xbb WP [&BAkLAAA=] | Domain of Istan!",
        15, 8,
        SchedMode::Uniform, 120, 105,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 22 — Death-Branded Shatterer ─────────────────────────────────────────
    // 2h cycle, 1:15 UTC → 1:15, 3:15… (events.json confirmed)
    s_Events[21] = {
        22, "Death-Branded Shatterer",
        {MAP_JAHAI, 0u, 0u}, 1,
        {"[&BJMLAAA=]", nullptr, nullptr},
        "Death-Branded Shatterer \xc2\xbb WP [&BJMLAAA=] | Jahai Bluffs!",
        15, 8,
        SchedMode::Uniform, 120, 75,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 23 — The Oil Floes ───────────────────────────────────────────────────
    // Sandswept Isles. 2h cycle, 0:45 UTC → 0:45, 2:45… (events.json, corrected from :00)
    s_Events[22] = {
        23, "The Oil Floes",
        {MAP_SANDSWEPT, 0u, 0u}, 1,
        {"[&BJ4KAAA=]", nullptr, nullptr},
        "The Oil Floes \xc2\xbb WP [&BJ4KAAA=] | Sandswept Isles!",
        20, 10,
        SchedMode::Uniform, 120, 45,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 24 — Flame Legion Effigy ─────────────────────────────────────────────
    // Grothmar Valley phase 1. 2h cycle, 0:10 UTC → 0:10, 2:10… (events.json, corrected from :00)
    s_Events[23] = {
        24, "Flame Legion Effigy",
        {MAP_GROTHMAR, 0u, 0u}, 1,
        {"[&BA4MAAA=]", nullptr, nullptr},
        "Flame Legion Effigy \xc2\xbb WP [&BA4MAAA=] | Grothmar Valley!",
        12, 5,
        SchedMode::Uniform, 120, 10,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 25 — Doomlore Shrine ─────────────────────────────────────────────────
    // Grothmar Valley phase 2. 2h cycle, 0:40 UTC → 0:40, 2:40… (events.json, corrected from :15)
    s_Events[24] = {
        25, "Doomlore Shrine",
        {MAP_GROTHMAR, 0u, 0u}, 1,
        {"[&BA4MAAA=]", nullptr, nullptr},
        "Doomlore Shrine \xc2\xbb WP [&BA4MAAA=] | Grothmar Valley!",
        15, 5,
        SchedMode::Uniform, 120, 40,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 26 — Ooze Pits ───────────────────────────────────────────────────────
    // Grothmar Valley phase 3. 2h cycle, 1:05 UTC → 1:05, 3:05… (events.json, corrected from :30)
    s_Events[25] = {
        26, "Ooze Pits",
        {MAP_GROTHMAR, 0u, 0u}, 1,
        {"[&BPgLAAA=]", nullptr, nullptr},
        "Ooze Pits \xc2\xbb WP [&BPgLAAA=] | Grothmar Valley!",
        15, 5,
        SchedMode::Uniform, 120, 65,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 27 — Shadow Behemoth  [FILL — world boss] ────────────────────────────
    // Queensdale. 2h cycle, 1:45 UTC → 1:45, 3:45… (events.json)
    s_Events[26] = {
        27, "Shadow Behemoth",
        {MAP_QUEENSDALE, 0u, 0u}, 1,
        {"[&BPcAAAA=]", nullptr, nullptr},
        "Shadow Behemoth \xc2\xbb WP [&BPcAAAA=] | Queensdale!",
        15, 8,
        SchedMode::Uniform, 120, 105,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 28 — Drakkar ─────────────────────────────────────────────────────────
    // Bjora Marches. 2h cycle, 1:20 UTC → 1:20, 3:20… (events.json, corrected from :90)
    s_Events[27] = {
        28, "Drakkar",
        {MAP_BJORA, 0u, 0u}, 1,
        {"[&BDkMAAA=]", nullptr, nullptr},
        "Drakkar \xc2\xbb WP [&BDkMAAA=] | Bjora Marches!",
        20, 10,
        SchedMode::Uniform, 120, 80,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 29 — Dragonstorm ─────────────────────────────────────────────────────
    // Eye of the North instanced. 2h cycle, 1:00 UTC → 1:00, 3:00… (events.json)
    s_Events[28] = {
        29, "Dragonstorm",
        {MAP_EYE_NORTH, 0u, 0u}, 1,
        {"[&BAkMAAA=]", nullptr, nullptr},
        "Dragonstorm \xc2\xbb WP [&BAkMAAA=] | Eye of the North!",
        20, 10,
        SchedMode::Uniform, 120, 60,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 30 — Adolescent Leviathan ────────────────────────────────────────────
    // New Kaineng City. Approximate schedule, 2h cycle :30.
    s_Events[29] = {
        30, "Adolescent Leviathan",
        {MAP_NEW_KAINENG, 0u, 0u}, 1,
        {"[&BOANAAA=]", nullptr, nullptr},
        "Adolescent Leviathan \xc2\xbb WP [&BOANAAA=] | New Kaineng City!",
        15, 8,
        SchedMode::Uniform, 120, 30,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 31 — Aetherblade Assault ─────────────────────────────────────────────
    // Seitung Province. 2h cycle, 1:30 UTC → 1:30, 3:30… (events.json, corrected from :00)
    s_Events[30] = {
        31, "Aetherblade Assault",
        {MAP_SEITUNG, 0u, 0u}, 1,
        {"[&BGUNAAA=]", nullptr, nullptr},
        "Aetherblade Assault \xc2\xbb WP [&BGUNAAA=] | Seitung Province!",
        20, 10,
        SchedMode::Uniform, 120, 90,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 32 — Kaineng Blackout ────────────────────────────────────────────────
    // [C] Confirmed: 2h, even hours. WP [&BBkNAAA=] (events.json confirmed)
    s_Events[31] = {
        32, "Kaineng Blackout",
        {MAP_NEW_KAINENG, 0u, 0u}, 1,
        {"[&BBkNAAA=]", nullptr, nullptr},
        "Kaineng Blackout \xc2\xbb WP [&BBkNAAA=] | New Kaineng City!",
        25, 10,
        SchedMode::Uniform, 120, 0,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 33 — Assault on Fort Aspenwood ───────────────────────────────────────
    // Echovald Wilds. 2h cycle, 1:40 UTC → 1:40, 3:40… (events.json, corrected from :60)
    s_Events[32] = {
        33, "Assault on Fort Aspenwood",
        {MAP_ECHOVALD, 0u, 0u}, 1,
        {"[&BPkMAAA=]", nullptr, nullptr},
        "Assault on Fort Aspenwood \xc2\xbb WP [&BPkMAAA=] | Echovald Wilds!",
        25, 10,
        SchedMode::Uniform, 120, 100,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 34 — The Gang War of Echovald ────────────────────────────────────────
    // Echovald Wilds. 2h cycle, 0:30 UTC → 0:30, 2:30… (events.json, corrected from :00)
    s_Events[33] = {
        34, "The Gang War of Echovald",
        {MAP_ECHOVALD, 0u, 0u}, 1,
        {"[&BMwMAAA=]", nullptr, nullptr},
        "Gang War of Echovald \xc2\xbb WP [&BMwMAAA=] | Echovald Wilds!",
        20, 8,
        SchedMode::Uniform, 120, 30,
        {0,0,0,0,0,0,0,0}, 0
    };

    // ── 35 — The Battle for the Jade Sea ─────────────────────────────────────
    // [C] Confirmed: 2h, odd hours. WP [&BKIMAAA=] (events.json confirmed)
    s_Events[34] = {
        35, "The Battle for the Jade Sea",
        {MAP_DRAGONS_END, 0u, 0u}, 1,
        {"[&BKIMAAA=]", nullptr, nullptr},
        "Battle for the Jade Sea \xc2\xbb WP [&BKIMAAA=] | Dragon's End!",
        30, 15,
        SchedMode::Uniform, 120, 60,
        {0,0,0,0,0,0,0,0}, 0
    };
}

// ═════════════════════════════════════════════════════════════════════════════
//  TIMING ENGINE
// ═════════════════════════════════════════════════════════════════════════════

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
    int s       = SecsIntoDay(nowUtc);
    int cycSec  = ev.cycleMinutes  * 60;
    int offSec  = (ev.offsetMinutes * 60) % cycSec;
    int durSec  = ev.durationMinutes * 60;

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
    if (ev.schedMode == SchedMode::Uniform)
        return UniformSecsUntilNext(ev, nowUtc);
    return IrregSecsUntilNext(ev, nowUtc);
}

int SecondsSinceLastStart(const MetaEvent& ev, time_t nowUtc)
{
    int until = SecondsUntilNext(ev, nowUtc);
    if (until == 0) {
        int s = SecsIntoDay(nowUtc);
        if (ev.schedMode == SchedMode::Uniform) {
            int cycSec = ev.cycleMinutes * 60;
            int offSec = (ev.offsetMinutes * 60) % cycSec;
            return ((s - offSec) % cycSec + cycSec) % cycSec;
        } else {
            int best = -1;
            for (int i = 0; i < ev.irregCount; i++) {
                int t = ev.irregTimes[i] * 60;
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

time_t ComputeLoopEndTime(int startIdx, time_t nowUtc)
{
    time_t cursor = nowUtc;
    for (int i = 0; i < META_EVENT_COUNT; i++) {
        int idx           = (startIdx + i) % META_EVENT_COUNT;
        const MetaEvent& ev = g_Events[idx];
        time_t start      = NextOccurrenceAfter(ev, cursor);
        cursor            = start + (time_t)(ev.durationMinutes * 60);
    }
    return cursor;
}

bool IsEventMap(const MetaEvent& ev, uint32_t mapId)
{
    for (int i = 0; i < ev.mapIdCount; i++)
        if (ev.mapIds[i] == mapId)
            return true;
    return false;
}

int GapSeconds(const MetaEvent& evA, time_t aStart, const MetaEvent& evB)
{
    time_t aEnd   = aStart + (time_t)(evA.durationMinutes * 60);
    time_t bStart = NextOccurrenceAfter(evB, aEnd);
    int gap       = (int)(bStart - aEnd);
    return gap < 0 ? 0 : gap;
}

void ComputeLoopSchedule(int startIdx, time_t fromTime,
                         ScheduledOccurrence out[META_EVENT_COUNT])
{
    time_t cursor  = fromTime;
    time_t prevEnd = fromTime;

    for (int i = 0; i < META_EVENT_COUNT; i++) {
        int idx           = (startIdx + i) % META_EVENT_COUNT;
        const MetaEvent& ev = g_Events[idx];

        time_t start      = NextOccurrenceAfter(ev, cursor);
        time_t end        = start + (time_t)(ev.durationMinutes * 60);
        int    gap        = (int)(start - prevEnd);

        out[i].eventIdx   = idx;
        out[i].startTime  = start;
        out[i].endTime    = end;
        out[i].gapBefore  = gap < 0 ? 0 : gap;

        prevEnd = end;
        cursor  = end;
    }
}
