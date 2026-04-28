#include "ui.h"
#include "events.h"
#include "state.h"
#include "chat.h"
#include "Nexus.h"
#include "icon_png.h"
#include "imgui.h"
#include <cstdio>
#include <ctime>
#include <cstring>
#include <string>
#include <algorithm>

extern AddonAPI_t* APIDefs;

// ─────────────────────────────────────────────────────────────────────────────
//  COLOUR PALETTE  (Lessons_Learned §28: use const, not constexpr for ImVec4)
// ─────────────────────────────────────────────────────────────────────────────
static const ImVec4 C_WHITE    = {1.00f, 1.00f, 1.00f, 1.00f};
static const ImVec4 C_GREY     = {0.55f, 0.55f, 0.55f, 1.00f};
static const ImVec4 C_DARK     = {0.35f, 0.35f, 0.35f, 1.00f};
static const ImVec4 C_GREEN    = {0.30f, 0.90f, 0.35f, 1.00f};
static const ImVec4 C_DKGREEN  = {0.18f, 0.55f, 0.22f, 1.00f};
static const ImVec4 C_YELLOW   = {1.00f, 0.88f, 0.25f, 1.00f};
static const ImVec4 C_ORANGE   = {1.00f, 0.58f, 0.10f, 1.00f};
static const ImVec4 C_RED      = {0.92f, 0.22f, 0.22f, 1.00f};
static const ImVec4 C_CYAN     = {0.20f, 0.88f, 0.95f, 1.00f};
static const ImVec4 C_LTBLUE   = {0.50f, 0.72f, 1.00f, 1.00f};
static const ImVec4 C_PURPLE   = {0.72f, 0.48f, 1.00f, 1.00f};
static const ImVec4 C_GOLD     = {1.00f, 0.82f, 0.35f, 1.00f};
static const ImVec4 C_TEAL     = {0.20f, 0.75f, 0.65f, 1.00f};

// Expansion-era accent colours (for the "Now" / "Next" event header bar)
// Core: silvery-blue │ HoT: jungle green │ PoF: desert gold │ IBS: icebrood blue
// EoD: jade teal     │ SotO: celestial purple
static const ImVec4 ERA_CORE[2]  = {{0.45f,0.55f,0.75f,1.f}, {0.20f,0.28f,0.45f,0.8f}};
static const ImVec4 ERA_HOT[2]   = {{0.25f,0.65f,0.30f,1.f}, {0.10f,0.32f,0.12f,0.8f}};
static const ImVec4 ERA_POF[2]   = {{0.75f,0.58f,0.18f,1.f}, {0.38f,0.28f,0.05f,0.8f}};
static const ImVec4 ERA_IBS[2]   = {{0.30f,0.52f,0.82f,1.f}, {0.10f,0.22f,0.48f,0.8f}};
static const ImVec4 ERA_EOD[2]   = {{0.20f,0.70f,0.60f,1.f}, {0.06f,0.32f,0.28f,0.8f}};

// Per-event era index: 0=Core, 1=HoT, 2=PoF, 3=IBS, 4=EoD
// Matches g_Events[0..34] — 35 events (27 original + 8 world-boss fills)
static const int s_Era[META_EVENT_COUNT] = {
    0, // [ 0] Claw of Jormag          — Core
    0, // [ 1] Svanir Shaman Chief [F]  — Core
    0, // [ 2] Tequatl                  — Core
    0, // [ 3] Great Jungle Wurm   [F]  — Core
    0, // [ 4] Karka Queen              — Core / LW1
    0, // [ 5] Megadestroyer       [F]  — Core
    0, // [ 6] Triple Trouble           — Core / LW2
    0, // [ 7] The Shatterer       [F]  — Core
    0, // [ 8] Twisted Marionette       — Core / LW1
    0, // [ 9] Fire Elemental      [F]  — Core
    1, // [10] Night and the Enemy      — HoT
    1, // [11] Battle in Tarir          — HoT
    1, // [12] Chak Gerent              — HoT
    0, // [13] Ley-Line Anomaly         — Core / LW3
    2, // [14] Choya Pinata             — PoF
    2, // [15] The Path to Ascension    — PoF
    2, // [16] Junundu Rising           — PoF
    2, // [17] Maws of Torment     [F]  — PoF
    2, // [18] Forged with Fire         — PoF
    2, // [19] Serpents' Ire       [F]  — PoF
    2, // [20] Palawadan                — PoF / LW4
    2, // [21] Death-Branded Shatterer  — PoF / LW4
    2, // [22] The Oil Floes            — PoF / LW4
    3, // [23] Flame Legion Effigy      — IBS
    3, // [24] Doomlore Shrine          — IBS
    3, // [25] Ooze Pits                — IBS
    0, // [26] Shadow Behemoth     [F]  — Core (fill between IBS events)
    3, // [27] Drakkar                  — IBS
    3, // [28] Dragonstorm              — IBS
    4, // [29] Adolescent Leviathan     — EoD
    4, // [30] Aetherblade Assault      — EoD
    4, // [31] Kaineng Blackout         — EoD
    4, // [32] Assault on Fort Aspenwood— EoD
    4, // [33] The Gang War of Echovald — EoD
    4, // [34] Battle for the Jade Sea  — EoD
};

static const ImVec4* EraAccents[5] = { ERA_CORE, ERA_HOT, ERA_POF, ERA_IBS, ERA_EOD };
static const char*   EraNames[5]   = { "Core", "HoT", "PoF", "IBS", "EoD" };

// Short map/location labels for each event (35 events)
static const char* s_Location[META_EVENT_COUNT] = {
    "Frostgorge Sound",   // [ 0] Claw of Jormag
    "Wayfarer Foothills", // [ 1] Svanir Shaman Chief [F]
    "Sparkfly Fen",       // [ 2] Tequatl
    "Caledon Forest",     // [ 3] Great Jungle Wurm [F]
    "Southsun Cove",      // [ 4] Karka Queen
    "Mount Maelstrom",    // [ 5] Megadestroyer [F]
    "Bloodtide Coast",    // [ 6] Triple Trouble
    "Blazeridge Steppes", // [ 7] The Shatterer [F]
    "Eye of the North",   // [ 8] Twisted Marionette
    "Metrica Province",   // [ 9] Fire Elemental [F]
    "Verdant Brink",      // [10] Night and the Enemy
    "Auric Basin",        // [11] Battle in Tarir
    "Tangled Depths",     // [12] Chak Gerent
    "Rotating (3 maps)",  // [13] Ley-Line Anomaly
    "Crystal Oasis",      // [14] Choya Pinata
    "Elon Riverlands",    // [15] Path to Ascension
    "The Desolation",     // [16] Junundu Rising
    "The Desolation",     // [17] Maws of Torment [F]
    "Domain of Vabbi",    // [18] Forged with Fire
    "Domain of Vabbi",    // [19] Serpents' Ire [F]
    "Domain of Istan",    // [20] Palawadan
    "Jahai Bluffs",       // [21] Death-Branded Shatterer
    "Sandswept Isles",    // [22] The Oil Floes
    "Grothmar Valley",    // [23] Flame Legion Effigy
    "Grothmar Valley",    // [24] Doomlore Shrine
    "Grothmar Valley",    // [25] Ooze Pits
    "Queensdale",         // [26] Shadow Behemoth [F]
    "Bjora Marches",      // [27] Drakkar
    "Eye of the North",   // [28] Dragonstorm
    "New Kaineng City",   // [29] Adolescent Leviathan
    "Seitung Province",   // [30] Aetherblade Assault
    "New Kaineng City",   // [31] Kaineng Blackout
    "Echovald Wilds",     // [32] Fort Aspenwood
    "Echovald Wilds",     // [33] Gang War
    "Dragon's End",       // [34] Battle for the Jade Sea
};

// ─────────────────────────────────────────────────────────────────────────────
//  UTILITY HELPERS
// ─────────────────────────────────────────────────────────────────────────────

// Converts UTC calendar fields to a time_t without relying on _mkgmtime/timegm.
// Uses the standard POSIX formula: counts days from epoch.
static time_t MakeUtcEpoch(int year, int month, int day, int hour, int minute, int second = 0)
{
    // Days from 1970-01-01 to year-01-01
    int y   = year - 1;
    int days = 365 * y + y / 4 - y / 100 + y / 400;
    // Subtract days up to 1969-12-31
    y = 1969;
    days -= 365 * y + y / 4 - y / 100 + y / 400;
    // Days in months of the current year
    static const int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    for (int m = 1; m < month; m++)
        days += mdays[m] + (m == 2 && leap ? 1 : 0);
    days += day - 1;
    return (time_t)days * 86400 + hour * 3600 + minute * 60 + second;
}

static void FmtCountdown(char* buf, size_t sz, int secs)
{
    if (secs <= 0) { snprintf(buf, sz, "0:00"); return; }
    int h = secs / 3600, m = (secs % 3600) / 60, s = secs % 60;
    if (h > 0) snprintf(buf, sz, "%d:%02d:%02d", h, m, s);
    else       snprintf(buf, sz, "%d:%02d", m, s);
}

static void FmtUtc(char* buf, size_t sz, time_t t)
{
    struct tm u;
#ifdef _WIN32
    gmtime_s(&u, &t);
#else
    gmtime_r(&t, &u);
#endif
    snprintf(buf, sz, "%02d:%02d UTC", u.tm_hour, u.tm_min);
}

// Thin horizontal progress bar, no cursor advance (draws directly)
static void ProgressBar(float frac, ImVec4 fill, float h = 6.0f)
{
    frac = std::max(0.0f, std::min(1.0f, frac));
    float  w   = ImGui::GetContentRegionAvail().x;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, {pos.x + w, pos.y + h}, IM_COL32(40,40,40,200), 2.f);
    if (frac > 0.f)
        dl->AddRectFilled(pos, {pos.x + w * frac, pos.y + h},
                          ImGui::ColorConvertFloat4ToU32(fill), 2.f);
    dl->AddRect(pos, {pos.x + w, pos.y + h}, IM_COL32(80,80,80,160), 2.f);
    ImGui::Dummy({w, h});
}

// Colored push/pop helpers
#define TC(col, ...) ImGui::PushStyleColor(ImGuiCol_Text,(col)); ImGui::Text(__VA_ARGS__); ImGui::PopStyleColor()
#define TCU(col, s)  ImGui::PushStyleColor(ImGuiCol_Text,(col)); ImGui::TextUnformatted(s); ImGui::PopStyleColor()

// ── Era-tinted section header bar ─────────────────────────────────────────
// Draws a colored rectangle behind a label.
static void EraHeader(const char* label, int era, float height = 22.0f)
{
    const ImVec4& bg = EraAccents[era][1];
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float  w   = ImGui::GetContentRegionAvail().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, {pos.x + w, pos.y + height},
                      ImGui::ColorConvertFloat4ToU32(bg), 4.f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.f);
    ImGui::PushStyleColor(ImGuiCol_Text, EraAccents[era][0]);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::Dummy({w, height - ImGui::GetTextLineHeight() - 3.f});
}

// ─────────────────────────────────────────────────────────────────────────────
//  AUTO-ANNOUNCE
// ─────────────────────────────────────────────────────────────────────────────
static void TickAutoAnnounce(int nextIdx, int secsUntilNext)
{
    if (!g_State.cfg.autoAnnounce) return;
    if (g_State.announceFired)     return;
    if (secsUntilNext <= 0)        return;

    const MetaEvent& nxt = g_Events[nextIdx];
    int thresh = nxt.prepMinutes * 60;
    if (secsUntilNext > thresh) return;

    {
        std::lock_guard<std::mutex> lk(g_StateMutex);
        g_State.announceFired   = true;
        g_State.lastAnnounceTime = time(nullptr);
    }
    char full[512];
    snprintf(full, sizeof(full), "/d %s", nxt.squadMsg);
    CopyToClipboard(full);
    StateToast(">> Auto-announce copied — paste into squad chat!");
}

// ─────────────────────────────────────────────────────────────────────────────
//  MAP-CHANGE HANDLER  (called from dllmain.cpp)
// ─────────────────────────────────────────────────────────────────────────────
void UIOnMapChanged(uint32_t newMapId)
{
    g_State.mapEntryTime = time(nullptr);

    if (!g_State.cfg.autoAdvanceOnMap) return;
    int nextIdx = (g_State.currentIdx + 1) % META_EVENT_COUNT;
    if (IsEventMap(g_Events[nextIdx], newMapId)) {
        StateAdvance();
        StateToast("Map detected — loop advanced!");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  MAIN RENDER
// ─────────────────────────────────────────────────────────────────────────────
void UIRender()
{
    // ── one-time ImGui context setup  (Lessons_Learned §1) ────────────────
    if (!g_State.imguiReady) {
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(APIDefs->ImguiContext));
        ImGui::SetAllocatorFunctions(
            reinterpret_cast<void*(*)(size_t,void*)>(APIDefs->ImguiMalloc),
            reinterpret_cast<void (*)(void*, void*)>(APIDefs->ImguiFree));
        g_State.imguiReady = true;
    }

    if (!g_State.windowOpen) return;

    time_t now = time(nullptr);

    // Grab a consistent snapshot of state for this frame
    std::unique_lock<std::mutex> lk(g_StateMutex);
    const int  curIdx   = g_State.currentIdx;
    const int  nextIdx  = (curIdx + 1) % META_EVENT_COUNT;
    const bool loopOn   = g_State.loopActive;
    const int  startIdx = g_State.loopStartIdx;
    lk.unlock();

    const MetaEvent& cur  = g_Events[curIdx];
    const MetaEvent& nxt  = g_Events[nextIdx];

    int secsSince    = SecondsSinceLastStart(cur, now);
    int secsUntilNxt = SecondsUntilNext(nxt, now);
    int durSec       = cur.durationMinutes * 60;
    bool curActive   = (secsSince >= 0 && secsSince < durSec);

    // Gap between cur end and nxt start
    time_t curStart  = curActive ? (now - secsSince)
                                 : NextOccurrenceAfter(cur, now);
    int gapSec       = GapSeconds(cur, curStart, nxt);

    // Waypoint unlocks once the current event has started
    {
        std::lock_guard<std::mutex> lk2(g_StateMutex);
        g_State.waypointUnlocked = curActive;
    }

    // Auto-announce + toast decay
    TickAutoAnnounce(nextIdx, secsUntilNxt);
    if (g_State.toastAlpha > 0.f)
        g_State.toastAlpha = std::max(0.f, g_State.toastAlpha - ImGui::GetIO().DeltaTime * 0.28f);

    // ─────────────────────────────────────────────────────────────────────
    //  WINDOW
    // ─────────────────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize({480, 0}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({360, 220}, {900, FLT_MAX});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  {8, 8});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    {6, 4});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.07f, 0.07f, 0.09f, 0.95f});

    ImGuiWindowFlags wf = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    bool open = g_State.windowOpen;
    if (!ImGui::Begin("##MetaTrain", &open, wf)) {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::End();
        g_State.windowOpen = open;
        return;
    }
    g_State.windowOpen = open;

    // ── Title row ────────────────────────────────────────────────────────
    {
        // Re-query each frame so the icon appears as soon as Nexus finishes loading it
        if (!g_AddonIcon)
            g_AddonIcon = APIDefs->Textures_GetOrCreateFromMemory(
                "METATRAIN_ICON", (void*)icon_png, (uint64_t)icon_png_len);
        if (g_AddonIcon && g_AddonIcon->Resource) {
            ImGui::Image((ImTextureID)g_AddonIcon->Resource, {20, 20});
            ImGui::SameLine();
        }
        ImGui::PushStyleColor(ImGuiCol_Text, C_GOLD);
        ImGui::Text("META TRAIN COMMANDER");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        float cw = ImGui::GetContentRegionAvail().x;
        char utcBuf[24];
        FmtUtc(utcBuf, sizeof(utcBuf), now);
        float tw = ImGui::CalcTextSize(utcBuf).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cw - tw);
        TCU(C_GREY, utcBuf);
    }

    // ── Loop status bar ──────────────────────────────────────────────────
    if (loopOn) {
        int done = 0;
        for (int i = 0; i < META_EVENT_COUNT; i++)
            if ((startIdx + i) % META_EVENT_COUNT == curIdx) { done = i; break; }

        time_t loopEnd  = ComputeLoopEndTime(startIdx, now);
        int    loopSecs = (int)(loopEnd - now);
        char lBuf[16]; FmtCountdown(lBuf, sizeof(lBuf), loopSecs);

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.12f,0.08f,0.20f,0.8f});
        ImGui::BeginChild("##loopbar", {0, 30}, false);
        TC(C_PURPLE, "Loop: %d / %d", done, META_EVENT_COUNT);
        ImGui::SameLine();
        float frac = (float)done / META_EVENT_COUNT;
        float bw   = ImGui::GetContentRegionAvail().x - 90.f;
        if (bw > 20.f) {
            ImVec2 p   = ImGui::GetCursorScreenPos();
            float  bh  = ImGui::GetTextLineHeight() * 0.6f;
            float  by  = p.y + (ImGui::GetTextLineHeight() - bh) * 0.5f;
            ImGui::GetWindowDrawList()->AddRectFilled({p.x,by},{p.x+bw,by+bh},IM_COL32(40,20,70,200),2.f);
            if (frac > 0.f)
                ImGui::GetWindowDrawList()->AddRectFilled({p.x,by},{p.x+bw*frac,by+bh},
                    ImGui::ColorConvertFloat4ToU32(C_PURPLE),2.f);
            ImGui::Dummy({bw, ImGui::GetTextLineHeight()});
        }
        ImGui::SameLine();
        TC(C_CYAN, "Loop ends: %s", lBuf);
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::Separator();

    // ═══════════════════════════════════════════════════════════════════════
    //  CURRENT EVENT
    // ═══════════════════════════════════════════════════════════════════════
    {
        int era = s_Era[curIdx];
        char hdr[80];
        snprintf(hdr, sizeof(hdr), "  NOW  |  #%d  %s", cur.id, cur.name);
        EraHeader(hdr, era, 24.f);
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.09f,0.11f,0.09f,0.85f});
    ImGui::BeginChild("##curpanel", {0, 96}, true);
    {
        TC(C_DARK, "%s", s_Location[curIdx]);
        ImGui::SameLine(0, 12);
        TC(C_DARK, "[%s]", EraNames[s_Era[curIdx]]);

        if (curActive) {
            int remSec = std::max(0, durSec - secsSince);
            float frac = (durSec > 0) ? (float)secsSince / durSec : 1.f;
            char remBuf[16]; FmtCountdown(remBuf, sizeof(remBuf), remSec);

            ProgressBar(frac, C_GREEN, 8.f);

            TC(C_GREEN, "IN PROGRESS");
            ImGui::SameLine();
            TC(C_WHITE, " — %s remaining", remBuf);
            ImGui::SameLine();
            TC(C_GREY, "/ ~%d min max", cur.durationMinutes);
        } else {
            ProgressBar(0.f, C_GREY, 8.f);
            int wait = (secsSince < 0) ? -secsSince : SecondsUntilNext(cur, now);
            time_t occ = NextOccurrenceAfter(cur, now);
            char wBuf[16], uBuf[16];
            FmtCountdown(wBuf, sizeof(wBuf), wait);
            FmtUtc(uBuf, sizeof(uBuf), occ);
            TC(C_YELLOW, "Starts in %s", wBuf);
            ImGui::SameLine();
            TC(C_GREY, "at %s", uBuf);
        }

        // Finish Event button — advance loop manually (e.g. event ended early)
        if (loopOn) {
            ImGui::SameLine(0, 16);
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.35f,0.18f,0.05f,0.9f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.60f,0.32f,0.08f,1.0f});
            if (ImGui::SmallButton("Finish Event \xe2\x9e\xa1")) {
                StateAdvance();
                StateToast("Event finished — moving to next!");
            }
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Event ended early? Click to advance to the next event.");
        }

        // Copy squad message for the current event
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.30f,0.22f,0.05f,0.9f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.55f,0.40f,0.08f,1.0f});
        if (ImGui::SmallButton("Copy /d Msg (this event)")) {
            char full[520];
            snprintf(full, sizeof(full), "/d %s", cur.squadMsg);
            CopyToClipboard(full);
            StateToast("Current event message copied!");
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Copies the squad announcement for the current event.\nPaste into GW2 chat.");
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    // ═══════════════════════════════════════════════════════════════════════
    //  BREAK INDICATOR  (only shown when gap > 5 min)
    // ═══════════════════════════════════════════════════════════════════════
    if (gapSec > 300) {
        char gBuf[16]; FmtCountdown(gBuf, sizeof(gBuf), gapSec);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.20f,0.14f,0.02f,0.85f});
        ImGui::BeginChild("##break", {0, 26}, false);
        TC(C_ORANGE, "\xe2\x8f\xb8 BREAK  %s  until next event", gBuf);
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  READY TO MOVE?  (shown when on map 15+ min and next event not imminent)
    // ═══════════════════════════════════════════════════════════════════════
    if (loopOn && g_State.moveNotifyEnabled && g_State.mapEntryTime > 0) {
        int secsOnMap   = (int)(now - g_State.mapEntryTime);
        int prepSec     = nxt.prepMinutes * 60;
        // Show when: been here 15+ min AND the next event is NOT yet in prep window
        if (secsOnMap >= 900 && secsUntilNxt > prepSec) {
            char moveBuf[16]; FmtCountdown(moveBuf, sizeof(moveBuf), secsUntilNxt - prepSec);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.05f,0.22f,0.28f,0.90f});
            ImGui::BeginChild("##movebanner", {0, 28}, false);
            TC(C_CYAN, "\xe2\x86\x92 Ready to move the train? Next move in %s", moveBuf);
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  NEXT EVENT
    // ═══════════════════════════════════════════════════════════════════════
    {
        int era = s_Era[nextIdx];
        char hdr[80];
        snprintf(hdr, sizeof(hdr), "  NEXT  |  #%d  %s", nxt.id, nxt.name);
        EraHeader(hdr, era, 24.f);
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.06f,0.10f,0.16f,0.85f});
    ImGui::BeginChild("##nxtpanel", {0, 108}, true);
    {
        TC(C_DARK, "%s", s_Location[nextIdx]);
        ImGui::SameLine(0, 12);
        TC(C_DARK, "[%s]", EraNames[s_Era[nextIdx]]);

        // Countdown line
        if (secsUntilNxt == 0) {
            TC(C_GREEN, "STARTING NOW!");
        } else {
            time_t occ = NextOccurrenceAfter(nxt, now);
            char cBuf[16], uBuf[16];
            FmtCountdown(cBuf, sizeof(cBuf), secsUntilNxt);
            FmtUtc(uBuf, sizeof(uBuf), occ);

            // Color the countdown: red < 5m, orange < 15m, yellow < 30m
            const ImVec4& tCol = (secsUntilNxt < 300)  ? C_RED    :
                                 (secsUntilNxt < 900)  ? C_ORANGE :
                                 (secsUntilNxt < 1800) ? C_YELLOW : C_CYAN;
            TC(tCol, "%s", cBuf);
            ImGui::SameLine();
            TC(C_GREY, "(%s)", uBuf);
        }

        // "Head there" cue
        {
            int prepSec = nxt.prepMinutes * 60;
            if (secsUntilNxt > 0 && secsUntilNxt <= prepSec) {
                TC(C_ORANGE, "\xe2\x96\xba HEAD THERE NOW!");
            } else if (secsUntilNxt > prepSec) {
                char mvBuf[16];
                FmtCountdown(mvBuf, sizeof(mvBuf), secsUntilNxt - prepSec);
                TC(C_GREY, "Move in: %s", mvBuf);
            }
        }

        ImGui::Spacing();

        // ── Waypoint / Message buttons ─────────────────────────────────
        bool wpOk = g_State.waypointUnlocked || (secsUntilNxt < nxt.prepMinutes * 60);

        if (wpOk) {
            if (nxt.mapIdCount == 1) {
                const char* wp = nxt.waypointCodes[0];
                char lbl[80];
                snprintf(lbl, sizeof(lbl), "  Copy WP  %s  ##cwp", wp ? wp : "(TBD)");
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f,0.35f,0.15f,0.9f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.22f,0.55f,0.22f,1.0f});
                if (ImGui::Button(lbl) && wp) {
                    CopyToClipboard(wp);
                    StateToast("Waypoint copied to clipboard!");
                }
                ImGui::PopStyleColor(2);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Copies WP chat link.\nPaste in GW2 chat to teleport.");
            } else {
                // Rotating map (Ley-Line Anomaly)
                const char* mapLbl[] = {"Timberline", "Iron Marches", "Gendarran"};
                TC(C_YELLOW, "Check active map:");
                for (int i = 0; i < nxt.mapIdCount; i++) {
                    if (!nxt.waypointCodes[i]) continue;
                    char lbl[64];
                    snprintf(lbl, sizeof(lbl), "%s##wp%d", mapLbl[i], i);
                    ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.28f,0.38f,0.9f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.15f,0.44f,0.60f,1.0f});
                    if (ImGui::Button(lbl)) {
                        CopyToClipboard(nxt.waypointCodes[i]);
                        StateToast("Waypoint copied!");
                    }
                    ImGui::PopStyleColor(2);
                    if (i + 1 < nxt.mapIdCount) ImGui::SameLine();
                }
            }

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.30f,0.22f,0.05f,0.9f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.55f,0.40f,0.08f,1.0f});
            if (ImGui::Button("  Copy /d Msg  ")) {
                char full[520];
                snprintf(full, sizeof(full), "/d %s", nxt.squadMsg);
                CopyToClipboard(full);
                StateToast("Squad message copied!");
            }
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Copies the squad announcement.\nPaste into GW2 chat.");

        } else {
            TCU(C_GREY, "  WP/Message buttons unlock once current event starts");
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    // ═══════════════════════════════════════════════════════════════════════
    //  WORLD BOSS TRACKER  (next 3 fill events by start time)
    // ═══════════════════════════════════════════════════════════════════════
    {
        static const int s_FillIdx[] = { 1, 3, 5, 7, 9, 17, 19, 26 };
        static const int s_FillCount = 8;

        // Sort fill events by SecondsUntilNext ascending, take top 3
        // Active events return 0 → they sort first (shown as IN PROGRESS)
        struct BossEntry { int evIdx; int until; int since; };
        BossEntry entries[s_FillCount];
        for (int fi = 0; fi < s_FillCount; fi++) {
            int i = s_FillIdx[fi];
            entries[fi] = { i, SecondsUntilNext(g_Events[i], now),
                               SecondsSinceLastStart(g_Events[i], now) };
        }
        // Insertion sort (8 elements)
        for (int a = 1; a < s_FillCount; a++) {
            BossEntry key = entries[a];
            int b = a - 1;
            while (b >= 0 && entries[b].until > key.until) { entries[b+1] = entries[b]; b--; }
            entries[b+1] = key;
        }

        ImGui::Spacing();
        TC(C_GREY, "WORLD BOSSES");
        ImGui::Separator();
        ImGui::Spacing();

        float avail  = ImGui::GetContentRegionAvail().x;
        float panelW = (avail - 12.f) / 3.f; // 3 columns, 6px gaps
        float panelH = 108.f;

        for (int col = 0; col < 3; col++) {
            const BossEntry& be  = entries[col];
            const MetaEvent& wb  = g_Events[be.evIdx];
            int dur              = wb.durationMinutes * 60;
            bool active          = (be.until == 0 && be.since >= 0 && be.since < dur);

            char childId[16]; snprintf(childId, sizeof(childId), "##wb%d", col);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.09f,0.09f,0.11f,0.85f});
            ImGui::BeginChild(childId, {panelW, panelH}, true);
            {
                ImGui::PushStyleColor(ImGuiCol_Text, EraAccents[s_Era[be.evIdx]][0]);
                ImGui::TextUnformatted(wb.name);
                ImGui::PopStyleColor();

                TC(C_DARK, "%s", s_Location[be.evIdx]);

                if (active) {
                    int remSec = dur - be.since;
                    char remBuf[16]; FmtCountdown(remBuf, sizeof(remBuf), remSec);
                    TC(C_GREEN, "NOW  -%s", remBuf);
                } else {
                    time_t occ = NextOccurrenceAfter(wb, now);
                    char untilBuf[16], utcBuf[16];
                    FmtCountdown(untilBuf, sizeof(untilBuf), be.until);
                    FmtUtc(utcBuf, sizeof(utcBuf), occ);
                    TC(C_CYAN, "In %s", untilBuf);
                    TC(C_DARK, "%s UTC", utcBuf);
                }

                if (wb.waypointCodes[0]) {
                    ImGui::Spacing();
                    char lbl[80];
                    snprintf(lbl, sizeof(lbl), "Copy WP##wbwp%d", col);
                    ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f,0.35f,0.15f,0.9f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.22f,0.55f,0.22f,1.0f});
                    if (ImGui::SmallButton(lbl)) {
                        CopyToClipboard(wb.waypointCodes[0]);
                        StateToast("Waypoint copied!");
                    }
                    ImGui::PopStyleColor(2);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", wb.waypointCodes[0]);
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();

            if (col < 2) ImGui::SameLine(0, 6);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CONTROLS  (loop selector, nav, auto-announce)
    // ═══════════════════════════════════════════════════════════════════════
    ImGui::Separator();
    ImGui::Spacing();

    // "Start Now" — auto-detects the currently running event from the UTC schedule
    {
        time_t nowT = time(nullptr);
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.38f,0.18f,0.90f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.18f,0.62f,0.30f,1.00f});
        if (ImGui::Button("  Start Now  ", {ImGui::GetContentRegionAvail().x, 0})) {
            // Find the event that started most recently and is still within its duration
            int bestIdx   = -1;
            int bestSince = INT_MAX;
            for (int i = 0; i < META_EVENT_COUNT; i++) {
                int since = SecondsSinceLastStart(g_Events[i], nowT);
                int dur   = g_Events[i].durationMinutes * 60;
                if (since >= 0 && since < dur && since < bestSince) {
                    bestSince = since;
                    bestIdx   = i;
                }
            }
            // Nothing active — fall back to soonest upcoming event
            if (bestIdx == -1) {
                int bestUntil = INT_MAX;
                for (int i = 0; i < META_EVENT_COUNT; i++) {
                    int until = SecondsUntilNext(g_Events[i], nowT);
                    if (until < bestUntil) { bestUntil = until; bestIdx = i; }
                }
            }
            if (bestIdx >= 0) {
                StateSetCurrent(bestIdx);
                {
                    std::lock_guard<std::mutex> lk2(g_StateMutex);
                    g_State.loopStartIdx  = bestIdx;
                    g_State.loopStartTime = nowT;
                    g_State.loopActive    = true;
                    g_State.cfg.startIdx  = bestIdx;
                }
                StateToast("Loop started!");
                const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
                if (dir) StateSave(dir);
            }
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Detects which event is currently running from the UTC schedule\n"
                              "and starts the loop there automatically.\n"
                              "Use this when joining a run already in progress.");
    }

    ImGui::Spacing();

    // Manual loop start selector (for overriding auto-detect)
    {
        static int s_sel = 0;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 130.f);
        // Preview shows just the name; dropdown items include location for context
        if (ImGui::BeginCombo("##startev", g_Events[s_sel].name)) {
            for (int i = 0; i < META_EVENT_COUNT; i++) {
                ImGui::PushStyleColor(ImGuiCol_Text, EraAccents[s_Era[i]][0]);
                bool sel = (i == s_sel);
                char lbl[128];
                snprintf(lbl, sizeof(lbl), "[%2d] %-32s %s",
                         g_Events[i].id, g_Events[i].name, s_Location[i]);
                if (ImGui::Selectable(lbl, sel)) s_sel = i;
                ImGui::PopStyleColor();
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Manually select which event to start at");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.18f,0.35f,0.55f,0.9f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.28f,0.55f,0.88f,1.0f});
        if (ImGui::Button("Start Here")) {
            StateSetCurrent(s_sel);
            {
                std::lock_guard<std::mutex> lk2(g_StateMutex);
                g_State.loopStartIdx  = s_sel;
                g_State.loopStartTime = time(nullptr);
                g_State.loopActive    = true;
                g_State.cfg.startIdx  = s_sel;
            }
            StateToast("Loop started!");
            const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
            if (dir) StateSave(dir);
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::Spacing();

    // Prev / Next / Auto-announce toggle
    if (ImGui::Button("< Prev")) {
        int ni = (g_State.currentIdx - 1 + META_EVENT_COUNT) % META_EVENT_COUNT;
        StateSetCurrent(ni);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Go back one event in the loop");
    ImGui::SameLine();
    if (ImGui::Button("Next >")) { StateAdvance(); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Advance to the next event in the loop");

    ImGui::SameLine(0, 16);
    {
        bool aa = g_State.cfg.autoAnnounce;
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        aa ? ImVec4{0.15f,0.35f,0.15f,0.9f}
                                                          : ImVec4{0.18f,0.18f,0.18f,0.9f});
        ImGui::PushStyleColor(ImGuiCol_CheckMark, C_GREEN);
        if (ImGui::Checkbox("Auto-Announce", &aa)) {
            std::lock_guard<std::mutex> lk2(g_StateMutex);
            g_State.cfg.autoAnnounce = aa;
            const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
            if (dir) StateSave(dir);
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Auto-copy /d squad message to clipboard before each event.\n"
                              "A toast notification tells you when to paste it.");
    }
    ImGui::SameLine();
    {
        bool am = g_State.cfg.autoAdvanceOnMap;
        ImGui::PushStyleColor(ImGuiCol_FrameBg,   am ? ImVec4{0.10f,0.28f,0.38f,0.9f}
                                                     : ImVec4{0.18f,0.18f,0.18f,0.9f});
        ImGui::PushStyleColor(ImGuiCol_CheckMark, C_CYAN);
        if (ImGui::Checkbox("Auto-Advance", &am)) {
            std::lock_guard<std::mutex> lk2(g_StateMutex);
            g_State.cfg.autoAdvanceOnMap = am;
            const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
            if (dir) StateSave(dir);
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Automatically advance the loop when you\nenter the next event's map.");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  UPCOMING EVENTS  (next 6 in the loop with gap markers)
    // ═══════════════════════════════════════════════════════════════════════
    ImGui::Separator();
    ImGui::Spacing();
    TCU(C_GREY, "Upcoming:");
    ImGui::BeginChild("##upcoming", {0, 130}, false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    time_t cursor = now;
    time_t prevEnd = curActive ? (curStart + (time_t)(cur.durationMinutes * 60)) : now;

    for (int i = 1; i <= 6; i++) {
        int idx           = (curIdx + i) % META_EVENT_COUNT;
        const MetaEvent&  ev = g_Events[idx];
        time_t occ        = NextOccurrenceAfter(ev, cursor);
        int    secsAway   = (int)(occ - now);
        cursor            = occ + (time_t)(ev.durationMinutes * 60);

        // Gap before this event
        int gap = (int)(occ - prevEnd);
        if (gap > 300) {
            char gBuf[16]; FmtCountdown(gBuf, sizeof(gBuf), gap);
            TC(C_ORANGE, "  ~~ %s break ~~", gBuf);
        }
        prevEnd = occ + (time_t)(ev.durationMinutes * 60);

        // Row: era colored name, UTC time, countdown
        const ImVec4& eCol = EraAccents[s_Era[idx]][0];
        char uBuf[16], cBuf[16];
        FmtUtc(uBuf, sizeof(uBuf), occ);
        FmtCountdown(cBuf, sizeof(cBuf), secsAway);

        TC(C_GREY, "  %2d.", ev.id);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, eCol);
        ImGui::Text("%-34s", ev.name);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        TC(C_CYAN, "%s", uBuf);
        ImGui::SameLine();
        const ImVec4& tCol = (secsAway < 300) ? C_RED : (secsAway < 900) ? C_ORANGE : C_GREY;
        TC(tCol, "(in %s)", cBuf);
    }

    ImGui::EndChild();

    // ═══════════════════════════════════════════════════════════════════════
    //  SCHEDULE A RUN  (plan a future loop)
    // ═══════════════════════════════════════════════════════════════════════
    ImGui::Separator();
    {
        bool show = g_State.showSchedule;
        if (ImGui::Checkbox("Schedule a Run", &show))
            g_State.showSchedule = show;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Enter a future date/time (UTC) to preview the full event schedule for that run.");
    }

    if (g_State.showSchedule) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.07f,0.10f,0.14f,0.92f});
        ImGui::BeginChild("##sched_panel", {0, 0}, true);

        // ── date/time input ─────────────────────────────────────────────
        TC(C_GOLD, "Planned Run Start  (UTC)");
        ImGui::Spacing();

        // Days-from-today selector (0 = today … 21 = three weeks out)
        {
            float avail = ImGui::GetContentRegionAvail().x;
            float dayW  = avail * 0.38f;
            float hrW   = avail * 0.16f;
            float minW  = avail * 0.16f;

            // Compute what date days-offset resolves to
            time_t nowLocal = time(nullptr);
            struct tm uu;
#ifdef _WIN32
            gmtime_s(&uu, &nowLocal);
#else
            gmtime_r(&nowLocal, &uu);
#endif
            time_t utcMidnight = nowLocal - (uu.tm_hour*3600 + uu.tm_min*60 + uu.tm_sec);
            // Date label for current offset
            time_t targetDay = utcMidnight + (time_t)g_State.schedDaysOffset * 86400;
            struct tm td;
#ifdef _WIN32
            gmtime_s(&td, &targetDay);
#else
            gmtime_r(&targetDay, &td);
#endif
            static const char* s_MonNames[] = {
                "Jan","Feb","Mar","Apr","May","Jun",
                "Jul","Aug","Sep","Oct","Nov","Dec"
            };
            static const char* s_DayNames[] = {
                "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
            };
            char dateLbl[32];
            snprintf(dateLbl, sizeof(dateLbl), "%s %s %d",
                     s_DayNames[td.tm_wday], s_MonNames[td.tm_mon], td.tm_mday);

            ImGui::SetNextItemWidth(dayW);
            if (ImGui::InputInt("##days", &g_State.schedDaysOffset, 1, 7)) {
                g_State.schedDaysOffset = std::max(0, std::min(g_State.schedDaysOffset, 21));
                g_State.schedDirty = true;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Days from today (0–21)");
            ImGui::SameLine(0, 6);
            TC(C_CYAN, "%s", dateLbl);

            ImGui::SameLine(0, 12);
            ImGui::SetNextItemWidth(hrW);
            if (ImGui::InputInt("h##sh", &g_State.schedHour, 1, 6)) {
                g_State.schedHour = std::max(0, std::min(g_State.schedHour, 23));
                g_State.schedDirty = true;
            }
            ImGui::SameLine(0, 6);
            ImGui::SetNextItemWidth(minW);
            if (ImGui::InputInt("m##sm", &g_State.schedMinute, 5, 15)) {
                g_State.schedMinute = std::max(0, std::min(g_State.schedMinute, 59));
                g_State.schedDirty = true;
            }
        }

        // ── starting event ──────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::SetNextItemWidth(240);
        if (ImGui::BeginCombo("Starting event##sse", g_Events[g_State.schedStartIdx].name)) {
            for (int i = 0; i < META_EVENT_COUNT; i++) {
                ImGui::PushStyleColor(ImGuiCol_Text, EraAccents[s_Era[i]][0]);
                bool sel = (i == g_State.schedStartIdx);
                char lbl[64];
                snprintf(lbl, sizeof(lbl), "[%2d] %s", g_Events[i].id, g_Events[i].name);
                if (ImGui::Selectable(lbl, sel)) {
                    g_State.schedStartIdx = i;
                    g_State.schedDirty    = true;
                }
                ImGui::PopStyleColor();
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // ── convert fields → time_t and compute schedule ────────────────
        static ScheduledOccurrence s_Schedule[META_EVENT_COUNT];
        static time_t s_SchedFrom = 0;

        if (g_State.schedDirty) {
            g_State.schedDirty = false;
            // Compute UTC epoch: midnight today + day offset + hour + minute
            time_t nowT = time(nullptr);
            struct tm ub;
#ifdef _WIN32
            gmtime_s(&ub, &nowT);
#else
            gmtime_r(&nowT, &ub);
#endif
            time_t midnight = nowT - ((time_t)ub.tm_hour*3600 + ub.tm_min*60 + ub.tm_sec);
            s_SchedFrom = midnight
                        + (time_t)g_State.schedDaysOffset * 86400
                        + (time_t)g_State.schedHour       * 3600
                        + (time_t)g_State.schedMinute      * 60;
            ComputeLoopSchedule(g_State.schedStartIdx, s_SchedFrom, s_Schedule);
        }

        // ── schedule table ───────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::Separator();
        {
            char fromBuf[32];
            FmtUtc(fromBuf, sizeof(fromBuf), s_SchedFrom);
            TC(C_CYAN, "Projected Schedule — loop starts %s", fromBuf);
        }
        ImGui::Spacing();

        // Column headers
        TC(C_GREY, "  #   Event                              Start       Duration   Gap Before");
        ImGui::Separator();

        ImGui::BeginChild("##sched_tbl", {0, 0}, false);
        time_t loopEndTime = 0;

        for (int i = 0; i < META_EVENT_COUNT; i++) {
            const ScheduledOccurrence& occ = s_Schedule[i];
            const MetaEvent&           ev  = g_Events[occ.eventIdx];
            const ImVec4& eCol             = EraAccents[s_Era[occ.eventIdx]][0];

            // Break marker
            if (occ.gapBefore > 300) {
                char gBuf[16];
                FmtCountdown(gBuf, sizeof(gBuf), occ.gapBefore);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.18f,0.12f,0.02f,0.7f});
                ImGui::BeginChild(("##gb" + std::to_string(i)).c_str(), {0, 18}, false);
                TC(C_ORANGE, "  ~~ %s break ~~", gBuf);
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }

            // Event row with era color
            char startBuf[20], durBuf[10];
            FmtUtc(startBuf, sizeof(startBuf), occ.startTime);

            struct tm eu;
#ifdef _WIN32
            gmtime_s(&eu, &occ.endTime);
#else
            gmtime_r(&occ.endTime, &eu);
#endif
            snprintf(durBuf, sizeof(durBuf), "~%d min", ev.durationMinutes);

            TC(C_GREY, "  %2d.", ev.id);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, eCol);
            ImGui::Text("%-34s", ev.name);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            TC(C_CYAN, "%s", startBuf);
            ImGui::SameLine();
            TC(C_GREY, "  %-10s", durBuf);

            // Copy WP button (small, inline)
            if (ev.waypointCodes[0]) {
                char lbl[32];
                snprintf(lbl, sizeof(lbl), "WP##wp%d", i);
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.25f,0.10f,0.8f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.15f,0.45f,0.15f,1.0f});
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {2,1});
                if (ImGui::SmallButton(lbl)) {
                    CopyToClipboard(ev.waypointCodes[0]);
                    StateToast("Waypoint copied!");
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
            }

            loopEndTime = occ.endTime;
        }

        // Loop total time
        ImGui::Separator();
        {
            char endBuf[20];
            FmtUtc(endBuf, sizeof(endBuf), loopEndTime);
            int totalSec = (int)(loopEndTime - s_SchedFrom);
            char totBuf[16]; FmtCountdown(totBuf, sizeof(totBuf), totalSec);
            TC(C_GOLD, "  Loop complete: %s  (total run time: ~%s)", endBuf, totBuf);
        }

        ImGui::EndChild(); // sched_tbl
        ImGui::EndChild(); // sched_panel
        ImGui::PopStyleColor();
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  TOAST NOTIFICATION  (fades out over ~3.5 s)
    // ═══════════════════════════════════════════════════════════════════════
    if (g_State.toastAlpha > 0.02f && !g_State.toastText.empty()) {
        ImGui::Separator();
        float alpha = g_State.toastAlpha;
        ImGui::PushStyleColor(ImGuiCol_Text, {C_GREEN.x, C_GREEN.y, C_GREEN.z, alpha});
        ImGui::TextUnformatted(g_State.toastText.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::PopStyleColor();  // WindowBg
    ImGui::PopStyleVar(2);   // WindowPadding, ItemSpacing
    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
//  OPTIONS TAB  (shown in Nexus addon settings panel)
// ─────────────────────────────────────────────────────────────────────────────
void UIRenderOptions()
{
    if (!g_State.imguiReady) return;

    TC(C_GOLD, "Meta Train Commander");
    ImGui::Separator();
    ImGui::Spacing();

    TC(C_WHITE, "Behaviour");
    ImGui::Indent();

    bool aa = g_State.cfg.autoAnnounce;
    if (ImGui::Checkbox("Auto-Announce squad messages", &aa)) {
        std::lock_guard<std::mutex> lk(g_StateMutex);
        g_State.cfg.autoAnnounce = aa;
        const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
        if (dir) StateSave(dir);
    }
    TCU(C_GREY, "Copies the /d message to your clipboard before each event.\n"
                "A toast notification tells you when to paste it into squad chat.");

    ImGui::Spacing();
    bool am = g_State.cfg.autoAdvanceOnMap;
    if (ImGui::Checkbox("Auto-advance loop on map change", &am)) {
        std::lock_guard<std::mutex> lk(g_StateMutex);
        g_State.cfg.autoAdvanceOnMap = am;
        const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
        if (dir) StateSave(dir);
    }
    TCU(C_GREY, "When you enter the next event's map the loop counter advances automatically.");

    ImGui::Spacing();
    bool mn = g_State.moveNotifyEnabled;
    if (ImGui::Checkbox("Ready-to-Move notifications", &mn)) {
        std::lock_guard<std::mutex> lk(g_StateMutex);
        g_State.moveNotifyEnabled = mn;
        const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
        if (dir) StateSave(dir);
    }
    TCU(C_GREY, "Shows a banner when you have been on the same map for 15+ minutes\n"
                "and the next event is not yet in its travel window.");

    ImGui::Unindent();
    ImGui::Spacing();
    ImGui::Separator();

    TC(C_WHITE, "Waypoint & Timing Notes");
    ImGui::Indent();
    TCU(C_GREY, "Confirmed WPs: Dragon's End, Kaineng Blackout, Karka Queen, Octovine/Tarir.");
    TCU(C_GREY, "All other WPs are best-guess — verify in-game and update squadMsg in events.cpp.");
    TCU(C_GREY, "Map IDs verified: Verdant Brink=1042, Auric Basin=1043, Tangled Depths=1045.");
    TCU(C_GREY, "Verify remaining IDs via https://api.guildwars2.com/v2/maps/{id}");
    ImGui::Unindent();
    ImGui::Spacing();

    TC(C_GREY, "Settings file: <GW2>\\addons\\MetaTrain\\MetaTrain.ini");
}

// Undefine local macros
#undef TC
#undef TCU
