#pragma once

// ── clipboard ──────────────────────────────────────────────────────────────
// Copies text to the Windows clipboard (CF_TEXT).
void CopyToClipboard(const char* text);

// ── squad chat ────────────────────────────────────────────────────────────
// Sends "/d <msg>" to GW2 via SendInput.
// Only fires if the GW2 window is the foreground window.
// Returns true if the message was sent, false if GW2 was not in focus.
// NOTE: This is opt-in; call only when g_State.cfg.autoAnnounce == true.
bool SendSquadMessage(const char* msg);
