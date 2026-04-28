#include "chat.h"
#include <windows.h>
#include <cstring>
#include <string>

// ── clipboard ──────────────────────────────────────────────────────────────
void CopyToClipboard(const char* text)
{
    if (!text || !*text) return;
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();

    size_t len   = strlen(text) + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    if (hMem) {
        char* dst = static_cast<char*>(GlobalLock(hMem));
        if (dst) {
            memcpy(dst, text, len);
            GlobalUnlock(hMem);
        }
        SetClipboardData(CF_TEXT, hMem);
    }
    CloseClipboard();
}

// ── squad chat via clipboard paste ────────────────────────────────────────
// Copies "/d <msg>" to clipboard, opens GW2 chat with Enter, pastes with
// Ctrl+V, and submits with Enter.  Using paste avoids per-character Unicode
// events triggering movement keys (w/a/s/d) while chat is still opening.
bool SendSquadMessage(const char* msg)
{
    if (!msg || !*msg) return false;

    HWND gw2 = FindWindowA("ArenaNet_Dx_Window_Class", nullptr);
    if (!gw2) gw2 = FindWindowA(nullptr, "Guild Wars 2");
    if (!gw2 || GetForegroundWindow() != gw2) return false;

    // Stage the message on the clipboard first (no keystrokes yet)
    std::string full = std::string("/d ") + msg;
    CopyToClipboard(full.c_str());

    auto sendVk = [](WORD vk, DWORD flags = 0) {
        INPUT in = {};
        in.type   = INPUT_KEYBOARD;
        in.ki.wVk = vk;
        in.ki.dwFlags = flags;
        SendInput(1, &in, sizeof(INPUT));
    };

    // Open chat
    sendVk(VK_RETURN);
    sendVk(VK_RETURN, KEYEVENTF_KEYUP);
    Sleep(150); // wait for GW2 to open the chat input box

    // Select-all then paste to replace any existing text
    sendVk(VK_CONTROL);
    sendVk('A');
    sendVk('A', KEYEVENTF_KEYUP);
    sendVk(VK_CONTROL, KEYEVENTF_KEYUP);
    Sleep(20);

    sendVk(VK_CONTROL);
    sendVk('V');
    sendVk('V', KEYEVENTF_KEYUP);
    sendVk(VK_CONTROL, KEYEVENTF_KEYUP);
    Sleep(50);

    // Submit
    sendVk(VK_RETURN);
    sendVk(VK_RETURN, KEYEVENTF_KEYUP);

    return true;
}
