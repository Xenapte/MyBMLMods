#include "main.hpp"

CursorClipper* mod{};

IMod* BMLEntry(IBML* bml) {
  mod = new CursorClipper(bml);
  return mod;
}

VOID CALLBACK WinEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
  mod->clip_cursor(false);
}

inline void CursorClipper::OnPreLoadLevel() {
  clip_cursor();
}

inline void CursorClipper::OnStartLevel() {
  clip_cursor();
}

inline void CursorClipper::OnUnpauseLevel() {
  clip_cursor();
}

inline void CursorClipper::OnPauseLevel() {
  cancel_clip_cursor();
}

inline void CursorClipper::OnLevelFinish() {
  cancel_clip_cursor();
}

inline void CursorClipper::OnPreExitLevel() {
  cancel_clip_cursor();
}

inline void CursorClipper::OnLoad() {
  hook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND,
                        EVENT_SYSTEM_FOREGROUND, NULL,
                        WinEventProcCallback, 0, 0,
                        WINEVENT_OUTOFCONTEXT);
}

inline void CursorClipper::OnUnload() {
  UnhookWinEvent(hook);
}

inline void CursorClipper::clip_cursor(bool set_playing_status) {
  if (set_playing_status)
    is_playing = true;
  const auto handle = static_cast<HWND>(m_bml->GetCKContext()->GetMainWindow());
  if (handle != GetActiveWindow() || !is_playing)
    return;
  RECT client_rect;
  GetClientRect(handle, &client_rect);
  POINT top_left_corner = { client_rect.left, client_rect.top };
  ClientToScreen(handle, &top_left_corner);
  client_rect = { .left = top_left_corner.x, .top = top_left_corner.y,
      .right = client_rect.right + top_left_corner.x, .bottom = client_rect.bottom + top_left_corner.y };
  ClipCursor(&client_rect);
}

inline void CursorClipper::cancel_clip_cursor() {
    is_playing = false;
    ClipCursor(NULL);
}
