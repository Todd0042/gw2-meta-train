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
static const ImVec4 C_YELLOW   = {1.00f, 0.88f, 0.25f, 1.00f};
static const ImVec4 C_ORANGE   = {1.00f, 0.58f, 0.10f, 1.00f};
static const ImVec4 C_RED      = {0.92f, 0.22f, 0.22f, 1.00f};
static const ImVec4 C_CYAN     = {0.20f, 0.88f, 0.95f, 1.00f};
static const ImVec4 C_PURPLE   = {0.72f, 0.48f, 1.00f, 1.00f};
static const ImVec4 C_GOLD     = {1.00f, 0.82f, 0.35f, 1.00f};

// Expansion-era accent colours: [0]=bright text, [1]=dark header background
// Core: silvery-blue | HoT: jungle green | PoF: desert gold | IBS: icebrood blue
// EoD: jade teal     | SotO: celestial purple
static const ImVec4 ERA_CORE[2] = {{0.45f,0.55f,0.75f,1.f}, {0.20f,0.28f,0.45f,0.8f}};
static const ImVec4 ERA_HOT[2]  = {{0.25f,0.65f,0.30f,1.f}, {0.10f,0.32f,0.12f,0.8f}};
static const ImVec4 ERA_POF[2]  = {{0.75f,0.58f,0.18f,1.f}, {0.38f,0.28f,0.05f,0.8f}};
static const ImVec4 ERA_IBS[2]  = {{0.30f,0.52f,0.82f,1.f}, {0.10f,0.22f,0.48f,0.8f}};
static const ImVec4 ERA_EOD[2]  = {{0.20f,0.70f,0.60f,1.f}, {0.06f,0.32f,0.28f,0.8f}};
static const ImVec4 ERA_SOTO[2] = {{0.72f,0.48f,1.00f,1.f}, {0.28f,0.12f,0.48f,0.8f}};
static const ImVec4 ERA_JW[2]   = {{0.20f,0.75f,0.85f,1.f}, {0.05f,0.32f,0.40f,0.8f}};
static const ImVec4 ERA_VOE[2]  = {{0.85f,0.40f,0.90f,1.f}, {0.38f,0.10f,0.42f,0.8f}};

static const ImVec4* EraAccents[8] = { ERA_CORE, ERA_HOT, ERA_POF, ERA_IBS, ERA_EOD, ERA_SOTO, ERA_JW, ERA_VOE };
static const char*   EraNames[8]   = { "Core", "HoT", "PoF", "IBS", "EoD", "SotO", "JW", "VoE" };

static inline int EraOf(const MetaEvent& ev) { return (int)ev.expansion; }

// ─────────────────────────────────────────────────────────────────────────────
//  UTILITY HELPERS
// ─────────────────────────────────────────────────────────────────────────────
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

#define TC(col, ...) ImGui::PushStyleColor(ImGuiCol_Text,(col)); ImGui::Text(__VA_ARGS__); ImGui::PopStyleColor()
#define TCU(col, s)  ImGui::PushStyleColor(ImGuiCol_Text,(col)); ImGui::TextUnformatted(s); ImGui::PopStyleColor()

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
static void TickAutoAnnounce(int nextMetaIdx, int secsUntilNext)
{
    if (!g_State.cfg.autoAnnounce) return;
    if (g_State.announceFired)     return;
    if (secsUntilNext <= 0)        return;

    const MetaEvent& nxt = g_Metas[nextMetaIdx];
    int thresh = nxt.prepMinutes * 60;
    if (secsUntilNext > thresh) return;

    {
        CritLock lk(&g_StateMutex);
        g_State.announceFired    = true;
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

    // Determine which meta is "next" in the plan or by index
    int nextMetaIdx;
    if (g_State.loopPlanCount > 0) {
        int nextPos = (g_State.loopPlanPos + 1) % g_State.loopPlanCount;
        nextMetaIdx = g_State.loopPlan[nextPos];
    } else {
        nextMetaIdx = (g_State.currentIdx + 1) % g_MetaCount;
    }

    if (IsEventMap(g_Metas[nextMetaIdx], newMapId)) {
        StateAdvance();
        StateToast("Map detected — loop advanced!");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  MAIN RENDER
// ─────────────────────────────────────────────────────────────────────────────
void UIRender()
{
    // One-time ImGui context setup (Lessons_Learned §1)
    if (!g_State.imguiReady) {
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(APIDefs->ImGuiContext));
        ImGui::SetAllocatorFunctions(
            reinterpret_cast<void*(*)(size_t,void*)>(APIDefs->ImGuiMalloc),
            reinterpret_cast<void (*)(void*, void*)>(APIDefs->ImGuiFree));
        g_State.imguiReady = true;
    }

    if (!g_State.windowOpen) return;

    time_t now = time(nullptr);

    // Snapshot of state for this frame
    EnterCriticalSection(&g_StateMutex);
    const int  curIdx        = g_State.currentIdx;
    const bool loopOn        = g_State.loopActive;
    const int  planCount     = g_State.loopPlanCount;
    const int  planPos       = g_State.loopPlanPos;
    LeaveCriticalSection(&g_StateMutex);

    const MetaEvent& cur = g_Metas[curIdx];

    // Determine NEXT meta index
    int nextIdx;
    if (loopOn && planCount > 0) {
        nextIdx = g_State.loopPlan[(planPos + 1) % planCount];
    } else {
        // Soonest upcoming meta that differs from current (or same if only one)
        int bestUntil = -1;
        nextIdx = (curIdx + 1) % g_MetaCount;
        for (int i = 0; i < g_MetaCount; i++) {
            if (i == curIdx) continue;
            int u = SecondsUntilNext(g_Metas[i], now);
            if (bestUntil < 0 || u < bestUntil) { bestUntil = u; nextIdx = i; }
        }
    }
    const MetaEvent& nxt = g_Metas[nextIdx];

    int secsSince    = SecondsSinceLastStart(cur, now);
    int secsUntilNxt = SecondsUntilNext(nxt, now);
    int durSec       = cur.durationMinutes * 60;
    bool curActive   = (secsSince >= 0 && secsSince < durSec);

    time_t curStart  = curActive ? (now - secsSince)
                                 : NextOccurrenceAfter(cur, now);
    int gapSec       = GapSeconds(cur, curStart, nxt);

    {
        CritLock lk2(&g_StateMutex);
        g_State.waypointUnlocked = curActive;
    }

    TickAutoAnnounce(nextIdx, secsUntilNxt);
    if (g_State.toastAlpha > 0.f)
        g_State.toastAlpha = std::max(0.f, g_State.toastAlpha - ImGui::GetIO().DeltaTime * 0.28f);

    // ── Window ───────────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize({480, 0}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({360, 220}, {900, FLT_MAX});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {8, 8});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   {6, 4});
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

    // ── Title row ─────────────────────────────────────────────────────────
    {
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

    ImGui::Separator();

    // ═══════════════════════════════════════════════════════════════════════════
    //  NOW PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    {
        char hdr[80];
        snprintf(hdr, sizeof(hdr), "  NOW  |  %s", cur.name);
        EraHeader(hdr, EraOf(cur), 24.f);
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.09f,0.11f,0.09f,0.85f});
    ImGui::BeginChild("##curpanel", {0, 96}, true);
    {
        TC(C_DARK, "%s", cur.mapName);
        ImGui::SameLine(0, 12);
        TC(C_DARK, "[%s]", EraNames[EraOf(cur)]);

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

    // ═══════════════════════════════════════════════════════════════════════════
    //  BREAK INDICATOR
    // ═══════════════════════════════════════════════════════════════════════════
    if (gapSec > 300) {
        char gBuf[16]; FmtCountdown(gBuf, sizeof(gBuf), gapSec);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.20f,0.14f,0.02f,0.85f});
        ImGui::BeginChild("##break", {0, 26}, false);
        TC(C_ORANGE, "\xe2\x8f\xb8 BREAK  %s  until next event", gBuf);
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    // ═══════════════════════════════════════════════════════════════════════════
    //  READY TO MOVE?
    // ═══════════════════════════════════════════════════════════════════════════
    if (loopOn && g_State.moveNotifyEnabled && g_State.mapEntryTime > 0) {
        int secsOnMap = (int)(now - g_State.mapEntryTime);
        int prepSec   = nxt.prepMinutes * 60;
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

    // ═══════════════════════════════════════════════════════════════════════════
    //  NEXT PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    {
        char hdr[80];
        snprintf(hdr, sizeof(hdr), "  NEXT  |  %s", nxt.name);
        EraHeader(hdr, EraOf(nxt), 24.f);
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.06f,0.10f,0.16f,0.85f});
    ImGui::BeginChild("##nxtpanel", {0, 108}, true);
    {
        TC(C_DARK, "%s", nxt.mapName);
        ImGui::SameLine(0, 12);
        TC(C_DARK, "[%s]", EraNames[EraOf(nxt)]);

        if (secsUntilNxt == 0) {
            TC(C_GREEN, "STARTING NOW!");
        } else {
            time_t occ = NextOccurrenceAfter(nxt, now);
            char cBuf[16], uBuf[16];
            FmtCountdown(cBuf, sizeof(cBuf), secsUntilNxt);
            FmtUtc(uBuf, sizeof(uBuf), occ);
            const ImVec4& tCol = (secsUntilNxt < 300)  ? C_RED    :
                                 (secsUntilNxt < 900)  ? C_ORANGE :
                                 (secsUntilNxt < 1800) ? C_YELLOW : C_CYAN;
            TC(tCol, "%s", cBuf);
            ImGui::SameLine();
            TC(C_GREY, "(%s)", uBuf);
        }

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

        bool wpOk = g_State.waypointUnlocked || (secsUntilNxt < nxt.prepMinutes * 60);
        if (wpOk) {
            const char* wp = nxt.waypointCode;
            if (wp) {
                char lbl[80];
                snprintf(lbl, sizeof(lbl), "  Copy WP  %s  ##cwp", wp);
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f,0.35f,0.15f,0.9f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.22f,0.55f,0.22f,1.0f});
                if (ImGui::Button(lbl)) {
                    CopyToClipboard(wp);
                    StateToast("Waypoint copied to clipboard!");
                }
                ImGui::PopStyleColor(2);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Copies WP chat link.\nPaste in GW2 chat to teleport.");
                ImGui::SameLine();
            }
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

    // ═══════════════════════════════════════════════════════════════════════════
    //  WORLD BOSS TRACKER  (next 3 world bosses by start time)
    // ═══════════════════════════════════════════════════════════════════════════
    {
        struct BossEntry { int idx; int until; int since; };
        BossEntry entries[32]; // max world boss count
        int count = std::min(g_WorldBossCount, 32);
        for (int i = 0; i < count; i++) {
            entries[i] = { i,
                SecondsUntilNext(g_WorldBosses[i], now),
                SecondsSinceLastStart(g_WorldBosses[i], now) };
        }
        // Sort ascending by until (active events = 0 sort first)
        for (int a = 1; a < count; a++) {
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
        float panelW = (avail - 12.f) / 3.f;
        float panelH = 100.f;

        for (int col = 0; col < 3 && col < count; col++) {
            const BossEntry& be = entries[col];
            const MetaEvent& wb = g_WorldBosses[be.idx];
            int dur    = wb.durationMinutes * 60;
            bool active = (be.until == 0 && be.since >= 0 && be.since < dur);

            char childId[16]; snprintf(childId, sizeof(childId), "##wb%d", col);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.09f,0.09f,0.11f,0.85f});
            ImGui::BeginChild(childId, {panelW, panelH}, true);
            {
                TCU(ERA_CORE[0], wb.name);
                TC(C_DARK, "%s", wb.mapName);

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
                    TC(C_DARK, "%s", utcBuf);
                }

                if (wb.waypointCode) {
                    ImGui::Spacing();
                    char lbl[32]; snprintf(lbl, sizeof(lbl), "Copy WP##wbwp%d", col);
                    ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f,0.35f,0.15f,0.9f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.22f,0.55f,0.22f,1.0f});
                    if (ImGui::SmallButton(lbl)) {
                        CopyToClipboard(wb.waypointCode);
                        StateToast("Waypoint copied!");
                    }
                    ImGui::PopStyleColor(2);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", wb.waypointCode);
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            if (col < 2) ImGui::SameLine(0, 6);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    //  BUILD YOUR RUN
    // ═══════════════════════════════════════════════════════════════════════════
    ImGui::Separator();
    ImGui::Spacing();

    // Sort all metas by SecondsUntilNext for the dropdown
    struct MetaSorted { int idx; int secs; bool active; };
    static MetaSorted s_sorted[64]; // fixed upper bound
    int sortCount = std::min(g_MetaCount, 64);
    for (int i = 0; i < sortCount; i++) {
        int u = SecondsUntilNext(g_Metas[i], now);
        int s2 = SecondsSinceLastStart(g_Metas[i], now);
        bool act = (u == 0 && s2 >= 0 && s2 < g_Metas[i].durationMinutes * 60);
        s_sorted[i] = { i, u, act };
    }
    for (int a = 1; a < sortCount; a++) {
        MetaSorted key = s_sorted[a];
        int b = a - 1;
        while (b >= 0 && s_sorted[b].secs > key.secs) { s_sorted[b+1] = s_sorted[b]; b--; }
        s_sorted[b+1] = key;
    }

    TC(C_GOLD, "BUILD YOUR RUN");
    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.08f,0.10f,0.12f,0.90f});
    ImGui::BeginChild("##buildpanel", {0, 0}, true);
    {
        static int s_addSel = 0; // declared here so remove handler can reset it

        // Plan list
        if (g_State.loopPlanCount == 0) {
            TCU(C_DARK, "  No events in plan — add events below.");
        } else {
            for (int i = 0; i < g_State.loopPlanCount; i++) {
                int mi = g_State.loopPlan[i];
                const MetaEvent& me = g_Metas[mi];
                bool isCurrent = (loopOn && i == planPos);

                // Remove button
                char rmLbl[16]; snprintf(rmLbl, sizeof(rmLbl), "x##rm%d", i);
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.40f,0.10f,0.10f,0.8f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.70f,0.20f,0.20f,1.0f});
                if (ImGui::SmallButton(rmLbl)) {
                    // Remove entry i from plan
                    CritLock lk2(&g_StateMutex);
                    for (int j = i; j < g_State.loopPlanCount - 1; j++)
                        g_State.loopPlan[j] = g_State.loopPlan[j+1];
                    g_State.loopPlanCount--;
                    if (g_State.loopPlanPos >= g_State.loopPlanCount)
                        g_State.loopPlanPos = 0;
                    if (g_State.loopPlanCount == 0) g_State.loopActive = false;
                    s_addSel = 0;
                }
                ImGui::PopStyleColor(2);
                ImGui::SameLine(0, 4);

                // Event name with era color; highlight current
                char planLbl[80];
                snprintf(planLbl, sizeof(planLbl), "%d. %s", i + 1, me.name);
                const ImVec4& nameCol = isCurrent ? C_GREEN : EraAccents[EraOf(me)][0];
                ImGui::PushStyleColor(ImGuiCol_Text, nameCol);
                ImGui::Text("%-32s", planLbl);
                ImGui::PopStyleColor();
                ImGui::SameLine(0, 8);
                TC(C_DARK, "%s", me.mapName);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Add-event dropdown.
        // Empty plan  → sorted by time from now (same as the upcoming panel).
        // Plan has ≥1 event → sorted by gap after the last plan event ends.

        struct DropItem { int idx; int secs; };
        DropItem dropItems[64];
        int dropCount = std::min(g_MetaCount, 64);

        const int planCountSnap = g_State.loopPlanCount; // single read, render-thread only
        if (planCountSnap == 0) {
            for (int i = 0; i < dropCount; i++)
                dropItems[i] = { s_sorted[i].idx, s_sorted[i].secs };
        } else {
            // Walk the plan chain to find when the last event ends.
            time_t cursor = now;
            for (int i = 0; i < planCountSnap; i++) {
                const MetaEvent& ev = g_Metas[g_State.loopPlan[i]];
                time_t start = NextOccurrenceAfter(ev, cursor);
                cursor = start + (time_t)(ev.durationMinutes * 60);
            }
            // cursor == chainEnd; sort each meta by gap from chainEnd to its next start.
            for (int i = 0; i < dropCount; i++) {
                time_t next = NextOccurrenceAfter(g_Metas[i], cursor);
                int gap = (int)(next - cursor);
                dropItems[i] = { i, gap < 0 ? 0 : gap };
            }
            for (int a = 1; a < dropCount; a++) {
                DropItem key = dropItems[a];
                int b = a - 1;
                while (b >= 0 && dropItems[b].secs > key.secs) { dropItems[b+1] = dropItems[b]; b--; }
                dropItems[b+1] = key;
            }
        }
        if (s_addSel >= dropCount) s_addSel = 0;

        // Build combo preview string.
        char previewBuf[80];
        {
            const DropItem& di = dropItems[s_addSel];
            const MetaEvent& me = g_Metas[di.idx];
            char cBuf[12]; FmtCountdown(cBuf, sizeof(cBuf), di.secs);
            if (planCountSnap == 0) {
                if (di.secs <= 0)
                    snprintf(previewBuf, sizeof(previewBuf), "\xe2\x96\xba NOW  %s", me.name);
                else
                    snprintf(previewBuf, sizeof(previewBuf), "in %s  %s", cBuf, me.name);
            } else {
                snprintf(previewBuf, sizeof(previewBuf), "+%s  %s", cBuf, me.name);
            }
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 84.f);
        if (ImGui::BeginCombo("##addev", previewBuf)) {
            for (int di = 0; di < dropCount; di++) {
                const DropItem& item = dropItems[di];
                const MetaEvent& me  = g_Metas[item.idx];
                bool sel = (di == s_addSel);

                char dropLbl[128];
                char cBuf[12]; FmtCountdown(cBuf, sizeof(cBuf), item.secs);
                if (planCountSnap == 0) {
                    if (item.secs <= 0)
                        snprintf(dropLbl, sizeof(dropLbl), "\xe2\x96\xba NOW  %-28s %s", me.name, me.mapName);
                    else
                        snprintf(dropLbl, sizeof(dropLbl), "%-8s  %-28s %s", cBuf, me.name, me.mapName);
                } else {
                    snprintf(dropLbl, sizeof(dropLbl), "+%-7s  %-28s %s", cBuf, me.name, me.mapName);
                }

                ImGui::PushStyleColor(ImGuiCol_Text, EraAccents[EraOf(me)][0]);
                if (ImGui::Selectable(dropLbl, sel)) s_addSel = di;
                ImGui::PopStyleColor();
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(planCountSnap == 0
                ? "Events sorted by time until start."
                : "Events sorted by gap after last plan event ends.");

        ImGui::SameLine(0, 6);
        bool canAdd = (g_State.loopPlanCount < MAX_LOOP_PLAN);
        if (!canAdd) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.18f,0.35f,0.55f,0.9f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.28f,0.55f,0.88f,1.0f});
        if (ImGui::Button("+ Add##addplan") && canAdd) {
            int addIdx = dropItems[s_addSel].idx;
            {
                CritLock lk2(&g_StateMutex);
                g_State.loopPlan[g_State.loopPlanCount++] = addIdx;
            }
            s_addSel = 0; // jump to soonest after new plan end
        }
        ImGui::PopStyleColor(2);
        if (!canAdd) ImGui::PopStyleVar();
        if (ImGui::IsItemHovered() && canAdd)
            ImGui::SetTooltip("Add this event to your run plan.");

        ImGui::Spacing();

        // Clear / Start buttons
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.35f,0.10f,0.10f,0.9f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.65f,0.18f,0.18f,1.0f});
        if (ImGui::Button("Clear Plan")) {
            CritLock lk2(&g_StateMutex);
            g_State.loopPlanCount = 0;
            g_State.loopPlanPos   = 0;
            g_State.loopActive    = false;
            s_addSel = 0;
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0, 10);

        bool canStart = (g_State.loopPlanCount > 0);
        if (!canStart) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.38f,0.18f,0.90f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.18f,0.62f,0.30f,1.00f});
        if (ImGui::Button("\xe2\x96\xba Start Run") && canStart) {
            {
                CritLock lk2(&g_StateMutex);
                g_State.loopPlanPos   = 0;
                g_State.currentIdx    = g_State.loopPlan[0];
                g_State.loopStartIdx  = g_State.loopPlan[0];
                g_State.loopStartTime = time(nullptr);
                g_State.loopActive    = true;
                g_State.announceFired = false;
                g_State.cfg.startIdx  = g_State.loopPlan[0];
            }
            const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
            if (dir) StateSave(dir);
            StateToast("Run started!");
        }
        ImGui::PopStyleColor(2);
        if (!canStart) ImGui::PopStyleVar();
        if (ImGui::IsItemHovered() && canStart)
            ImGui::SetTooltip("Start tracking your custom run plan from event 1.");

        // Quick "Start Now" — detects which meta is currently running
        ImGui::SameLine(0, 10);
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.20f,0.20f,0.35f,0.9f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.35f,0.35f,0.60f,1.0f});
        if (ImGui::Button("Start Now")) {
            // Find currently-active meta, or else soonest
            int bestIdx = -1, bestSince = INT_MAX;
            for (int i = 0; i < g_MetaCount; i++) {
                int since = SecondsSinceLastStart(g_Metas[i], now);
                int dur   = g_Metas[i].durationMinutes * 60;
                if (since >= 0 && since < dur && since < bestSince) {
                    bestSince = since; bestIdx = i;
                }
            }
            if (bestIdx < 0) {
                int bestUntil = INT_MAX;
                for (int i = 0; i < g_MetaCount; i++) {
                    int u = SecondsUntilNext(g_Metas[i], now);
                    if (u < bestUntil) { bestUntil = u; bestIdx = i; }
                }
            }
            if (bestIdx >= 0) {
                StateSetCurrent(bestIdx);
                {
                    CritLock lk2(&g_StateMutex);
                    g_State.loopStartIdx  = bestIdx;
                    g_State.loopStartTime = now;
                    g_State.loopActive    = true;
                    g_State.loopPlanCount = 0;
                    g_State.cfg.startIdx  = bestIdx;
                }
                const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
                if (dir) StateSave(dir);
                StateToast("Started at current event!");
            }
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Jump straight to whichever meta is running or starting soonest,\n"
                              "without building a plan. Use Prev/Next to navigate manually.");

        ImGui::Spacing();
        ImGui::Separator();

        // ── Save / Load named runs ─────────────────────────────────────────
        TC(C_GOLD, "Saved Runs");
        ImGui::Spacing();

        // Save row: name field + Save button
        {
            bool canSave = (g_State.loopPlanCount > 0)
                        && (g_State.savedRunsCount < MAX_SAVED_RUNS);
            float nameW = ImGui::GetContentRegionAvail().x - 72.f;
            if (nameW < 60.f) nameW = 60.f;
            ImGui::SetNextItemWidth(nameW);
            ImGui::InputText("##saveName", g_State.saveNameBuf, sizeof(g_State.saveNameBuf));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Name for this run (leave blank for auto-name).");
            ImGui::SameLine(0, 6);
            if (!canSave) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.25f,0.20f,0.40f,0.9f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.42f,0.34f,0.68f,1.0f});
            if (ImGui::Button("Save##saverun") && canSave) {
                {
                    CritLock lk2(&g_StateMutex);
                    SavedRun& slot = g_State.savedRuns[g_State.savedRunsCount];
                    if (g_State.saveNameBuf[0] == '\0')
                        snprintf(slot.name, sizeof(slot.name), "Run %d", g_State.savedRunsCount + 1);
                    else {
                        strncpy(slot.name, g_State.saveNameBuf, sizeof(slot.name) - 1);
                        slot.name[sizeof(slot.name) - 1] = '\0';
                    }
                    slot.count = g_State.loopPlanCount;
                    for (int i = 0; i < g_State.loopPlanCount; i++)
                        slot.plan[i] = g_State.loopPlan[i];
                    g_State.savedRunsCount++;
                    g_State.savedRunsSel   = g_State.savedRunsCount - 1;
                    g_State.saveNameBuf[0] = '\0';
                }
                const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
                if (dir) StateSave(dir);
                StateToast("Run saved!");
            }
            ImGui::PopStyleColor(2);
            if (!canSave) ImGui::PopStyleVar();
        }

        // Load / Delete row — only when at least one run is saved
        if (g_State.savedRunsCount > 0) {
            if (g_State.savedRunsSel >= g_State.savedRunsCount)
                g_State.savedRunsSel = 0;

            float dropW = ImGui::GetContentRegionAvail().x - 84.f;
            if (dropW < 80.f) dropW = 80.f;
            ImGui::SetNextItemWidth(dropW);
            const char* selName = g_State.savedRuns[g_State.savedRunsSel].name;
            if (ImGui::BeginCombo("##savedSel", selName)) {
                for (int r = 0; r < g_State.savedRunsCount; r++) {
                    bool sel = (r == g_State.savedRunsSel);
                    char lbl[72];
                    snprintf(lbl, sizeof(lbl), "%s  (%d events)",
                             g_State.savedRuns[r].name, g_State.savedRuns[r].count);
                    if (ImGui::Selectable(lbl, sel)) g_State.savedRunsSel = r;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine(0, 6);
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.18f,0.28f,0.18f,0.9f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.30f,0.48f,0.30f,1.0f});
            if (ImGui::Button("Load##loadrun")) {
                {
                    CritLock lk2(&g_StateMutex);
                    const SavedRun& sr = g_State.savedRuns[g_State.savedRunsSel];
                    g_State.loopPlanCount = sr.count;
                    for (int i = 0; i < sr.count; i++)
                        g_State.loopPlan[i] = sr.plan[i];
                    g_State.loopPlanPos = 0;
                    g_State.loopActive  = false;
                }
                StateToast("Run loaded!");
            }
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Load this saved run into the current plan.");

            ImGui::SameLine(0, 4);
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.40f,0.10f,0.10f,0.8f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.70f,0.20f,0.20f,1.0f});
            if (ImGui::Button("x##delrun")) {
                {
                    CritLock lk2(&g_StateMutex);
                    int sel2 = g_State.savedRunsSel;
                    for (int r = sel2; r < g_State.savedRunsCount - 1; r++)
                        g_State.savedRuns[r] = g_State.savedRuns[r+1];
                    g_State.savedRunsCount--;
                    if (g_State.savedRunsSel >= g_State.savedRunsCount && g_State.savedRunsCount > 0)
                        g_State.savedRunsSel = g_State.savedRunsCount - 1;
                }
                const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
                if (dir) StateSave(dir);
                StateToast("Saved run deleted.");
            }
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete this saved run.");
        } else {
            TCU(C_DARK, "  Build a plan above, name it, and click Save.");
        }

        ImGui::Spacing();

        // Manual Prev / Next navigation
        if (ImGui::Button("< Prev")) {
            int ni = (g_State.currentIdx - 1 + g_MetaCount) % g_MetaCount;
            StateSetCurrent(ni);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Go to previous meta");
        ImGui::SameLine();
        if (ImGui::Button("Next >")) { StateAdvance(); }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Advance to next meta");

        ImGui::SameLine(0, 16);
        {
            bool aa = g_State.cfg.autoAnnounce;
            ImGui::PushStyleColor(ImGuiCol_FrameBg,   aa ? ImVec4{0.15f,0.35f,0.15f,0.9f}
                                                         : ImVec4{0.18f,0.18f,0.18f,0.9f});
            ImGui::PushStyleColor(ImGuiCol_CheckMark, C_GREEN);
            if (ImGui::Checkbox("Auto-Announce", &aa)) {
                { CritLock lk2(&g_StateMutex); g_State.cfg.autoAnnounce = aa; }
                const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
                if (dir) StateSave(dir);
            }
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Auto-copy /d announcement to clipboard before each event.");
        }
        ImGui::SameLine();
        {
            bool am = g_State.cfg.autoAdvanceOnMap;
            ImGui::PushStyleColor(ImGuiCol_FrameBg,   am ? ImVec4{0.10f,0.28f,0.38f,0.9f}
                                                         : ImVec4{0.18f,0.18f,0.18f,0.9f});
            ImGui::PushStyleColor(ImGuiCol_CheckMark, C_CYAN);
            if (ImGui::Checkbox("Auto-Advance", &am)) {
                { CritLock lk2(&g_StateMutex); g_State.cfg.autoAdvanceOnMap = am; }
                const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
                if (dir) StateSave(dir);
            }
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Advance the loop when you enter the next event's map.");
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    // ═══════════════════════════════════════════════════════════════════════════
    //  UPCOMING EVENTS
    // ═══════════════════════════════════════════════════════════════════════════
    ImGui::Separator();
    ImGui::Spacing();
    TCU(C_GREY, "Upcoming:");
    ImGui::BeginChild("##upcoming", {0, 130}, false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    {
        // Show upcoming metas by soonest occurrence
        time_t cursor  = now;
        time_t prevEnd = curActive ? (curStart + (time_t)(cur.durationMinutes * 60)) : now;

        for (int i = 0; i < 6 && i < sortCount; i++) {
            const MetaSorted& ms = s_sorted[i];
            if (ms.idx == curIdx && ms.active) continue; // skip current active
            const MetaEvent& ev = g_Metas[ms.idx];
            time_t occ   = NextOccurrenceAfter(ev, cursor);
            int secsAway = (int)(occ - now);
            cursor = occ + (time_t)(ev.durationMinutes * 60);

            int gap = (int)(occ - prevEnd);
            if (gap > 300) {
                char gBuf[16]; FmtCountdown(gBuf, sizeof(gBuf), gap);
                TC(C_ORANGE, "  ~~ %s break ~~", gBuf);
            }
            prevEnd = cursor;

            char uBuf[16], cBuf[16];
            FmtUtc(uBuf, sizeof(uBuf), occ);
            FmtCountdown(cBuf, sizeof(cBuf), secsAway > 0 ? secsAway : 0);
            ImGui::PushStyleColor(ImGuiCol_Text, EraAccents[EraOf(ev)][0]);
            ImGui::Text("  %-28s", ev.name);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            TC(C_CYAN, "%s", uBuf);
            ImGui::SameLine();
            const ImVec4& tCol = (secsAway < 300) ? C_RED : (secsAway < 900) ? C_ORANGE : C_GREY;
            TC(tCol, "(in %s)", cBuf);
        }
    }

    ImGui::EndChild();

    // ═══════════════════════════════════════════════════════════════════════════
    //  SCHEDULE A RUN  (plan a future run)
    // ═══════════════════════════════════════════════════════════════════════════
    ImGui::Separator();
    {
        bool show = g_State.showSchedule;
        if (ImGui::Checkbox("Schedule a Run", &show))
            g_State.showSchedule = show;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Preview the full event schedule for a future run date/time.");
    }

    if (g_State.showSchedule) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.07f,0.10f,0.14f,0.92f});
        ImGui::BeginChild("##sched_panel", {0, 0}, true);

        TC(C_GOLD, "Planned Run Start  (UTC)");
        ImGui::Spacing();

        // Date/time inputs
        {
            float avail = ImGui::GetContentRegionAvail().x;
            float dayW  = avail * 0.38f;
            float hrW   = avail * 0.16f;
            float minW  = avail * 0.16f;

            time_t nowLocal = time(nullptr);
            struct tm uu;
#ifdef _WIN32
            gmtime_s(&uu, &nowLocal);
#else
            gmtime_r(&nowLocal, &uu);
#endif
            time_t utcMidnight = nowLocal - (uu.tm_hour*3600 + uu.tm_min*60 + uu.tm_sec);
            time_t targetDay   = utcMidnight + (time_t)g_State.schedDaysOffset * 86400;
            struct tm td;
#ifdef _WIN32
            gmtime_s(&td, &targetDay);
#else
            gmtime_r(&targetDay, &td);
#endif
            static const char* s_MonNames[] = {
                "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
            };
            static const char* s_DayNames[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
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

        // Starting event selector
        ImGui::Spacing();
        if (g_State.schedStartIdx >= g_MetaCount) g_State.schedStartIdx = 0;
        ImGui::SetNextItemWidth(260);
        if (ImGui::BeginCombo("Starting event##sse", g_Metas[g_State.schedStartIdx].name)) {
            for (int i = 0; i < g_MetaCount; i++) {
                ImGui::PushStyleColor(ImGuiCol_Text, EraAccents[EraOf(g_Metas[i])][0]);
                bool sel = (i == g_State.schedStartIdx);
                char lbl[64];
                snprintf(lbl, sizeof(lbl), "[%2d] %s", i + 1, g_Metas[i].name);
                if (ImGui::Selectable(lbl, sel)) {
                    g_State.schedStartIdx = i;
                    g_State.schedDirty    = true;
                }
                ImGui::PopStyleColor();
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Build plan array for schedule: all g_Metas in order starting from schedStartIdx
        static ScheduledOccurrence s_Schedule[64];
        static time_t s_SchedFrom = 0;

        if (g_State.schedDirty) {
            g_State.schedDirty = false;
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
                        + (time_t)g_State.schedHour        * 3600
                        + (time_t)g_State.schedMinute       * 60;

            // Use current plan if available, else all metas from schedStartIdx
            int schedCount = std::min(g_MetaCount, 64);
            int plan[64];
            if (g_State.loopPlanCount > 0 && g_State.loopPlanCount <= 64) {
                schedCount = g_State.loopPlanCount;
                for (int i = 0; i < schedCount; i++)
                    plan[i] = g_State.loopPlan[i];
            } else {
                for (int i = 0; i < schedCount; i++)
                    plan[i] = (g_State.schedStartIdx + i) % g_MetaCount;
            }
            ComputePlanSchedule(plan, schedCount, s_SchedFrom, s_Schedule);
        }

        // Schedule table
        ImGui::Spacing();
        ImGui::Separator();
        {
            char fromBuf[32]; FmtUtc(fromBuf, sizeof(fromBuf), s_SchedFrom);
            if (g_State.loopPlanCount > 0) {
                TC(C_CYAN, "Projected Schedule — your plan from %s", fromBuf);
            } else {
                TC(C_CYAN, "Projected Schedule — all events from %s", fromBuf);
            }
        }
        ImGui::Spacing();
        TC(C_GREY, "  #   Event                          Start       Duration");
        ImGui::Separator();

        int schedCount = (g_State.loopPlanCount > 0) ? g_State.loopPlanCount : g_MetaCount;
        schedCount = std::min(schedCount, 64);
        time_t loopEndTime = 0;

        ImGui::BeginChild("##sched_tbl", {0, 0}, false);
        for (int i = 0; i < schedCount; i++) {
            const ScheduledOccurrence& occ = s_Schedule[i];
            const MetaEvent& ev = g_Metas[occ.eventIdx];

            if (occ.gapBefore > 300) {
                char gBuf[16]; FmtCountdown(gBuf, sizeof(gBuf), occ.gapBefore);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.18f,0.12f,0.02f,0.7f});
                char gbId[16]; snprintf(gbId, sizeof(gbId), "##gb%d", i);
                ImGui::BeginChild(gbId, {0, 18}, false);
                TC(C_ORANGE, "  ~~ %s break ~~", gBuf);
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }

            char startBuf[20];
            FmtUtc(startBuf, sizeof(startBuf), occ.startTime);
            char durBuf[12];
            snprintf(durBuf, sizeof(durBuf), "~%d min", ev.durationMinutes);

            TC(C_GREY, "  %2d.", i + 1);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, EraAccents[EraOf(ev)][0]);
            ImGui::Text("%-28s", ev.name);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            TC(C_CYAN, "%s", startBuf);
            ImGui::SameLine();
            TC(C_GREY, "  %-10s", durBuf);

            if (ev.waypointCode) {
                char lbl[32]; snprintf(lbl, sizeof(lbl), "WP##wp%d", i);
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.25f,0.10f,0.8f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.15f,0.45f,0.15f,1.0f});
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {2,1});
                if (ImGui::SmallButton(lbl)) {
                    CopyToClipboard(ev.waypointCode);
                    StateToast("Waypoint copied!");
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
            }

            loopEndTime = occ.endTime;
        }

        ImGui::Separator();
        {
            char endBuf[20]; FmtUtc(endBuf, sizeof(endBuf), loopEndTime);
            int totalSec = (int)(loopEndTime - s_SchedFrom);
            char totBuf[16]; FmtCountdown(totBuf, sizeof(totBuf), totalSec);
            TC(C_GOLD, "  Complete: %s  (total: ~%s)", endBuf, totBuf);
        }

        ImGui::EndChild(); // sched_tbl
        ImGui::EndChild(); // sched_panel
        ImGui::PopStyleColor();
    }

    // ═══════════════════════════════════════════════════════════════════════════
    //  TOAST NOTIFICATION
    // ═══════════════════════════════════════════════════════════════════════════
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
//  OPTIONS TAB
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
        { CritLock lk(&g_StateMutex); g_State.cfg.autoAnnounce = aa; }
        const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
        if (dir) StateSave(dir);
    }
    TCU(C_GREY, "Copies the /d message to clipboard before each event.");

    ImGui::Spacing();
    bool am = g_State.cfg.autoAdvanceOnMap;
    if (ImGui::Checkbox("Auto-advance loop on map change", &am)) {
        { CritLock lk(&g_StateMutex); g_State.cfg.autoAdvanceOnMap = am; }
        const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
        if (dir) StateSave(dir);
    }
    TCU(C_GREY, "Advances the loop when you enter the next event's map.");

    ImGui::Spacing();
    bool mn = g_State.moveNotifyEnabled;
    if (ImGui::Checkbox("Ready-to-Move notifications", &mn)) {
        { CritLock lk(&g_StateMutex); g_State.moveNotifyEnabled = mn; }
        const char* dir = APIDefs->Paths_GetAddonDirectory("MetaTrain");
        if (dir) StateSave(dir);
    }
    TCU(C_GREY, "Shows a banner when you have been on the same map 15+ minutes\n"
                "and the next event is not yet in its travel window.");

    ImGui::Unindent();
    ImGui::Spacing();
    ImGui::Separator();

    TC(C_WHITE, "Event Data");
    ImGui::Indent();
    TCU(C_GREY, "19 meta events + 13 world bosses.");
    TCU(C_GREY, "Timings sourced from GW2 wiki / GW2-Simple-Timer data.");
    TCU(C_GREY, "Unverified WPs are marked TBD — verify in-game and update events.cpp.");
    TCU(C_GREY, "Map IDs verified via https://api.guildwars2.com/v2/maps/{id}");
    ImGui::Unindent();
    ImGui::Spacing();

    TC(C_GREY, "Settings file: <GW2>\\addons\\MetaTrain\\MetaTrain.ini");
}

#undef TC
#undef TCU
