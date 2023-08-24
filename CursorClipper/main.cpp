#pragma once

#include "../bml_includes.hpp"
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#endif // !WIN32_MEAN_AND_LEAN

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class CursorClipper : public IMod {
  // fail if the game window is not active
  void clip_cursor() {
    const auto handle = static_cast<HWND>(m_bml->GetCKContext()->GetMainWindow());
    if (handle != GetActiveWindow())
      return;
    RECT client_rect;
    GetClientRect(handle, &client_rect);
    POINT top_left_corner = { client_rect.left, client_rect.top };
    ClientToScreen(handle, &top_left_corner);
    client_rect = { .left = top_left_corner.x, .top = top_left_corner.y,
      .right = client_rect.right + top_left_corner.x, .bottom = client_rect.bottom + top_left_corner.y };
    ClipCursor(&client_rect);
  }

  void cancel_clip_cursor() {
    ClipCursor(NULL);
  }

public:
  CursorClipper(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "CursorClipper"; }
  virtual iCKSTRING GetVersion() override { return "0.0.1"; }
  virtual iCKSTRING GetName() override { return "Cursor Clipper"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override {
    return "A mod for limiting your cursor inside the game window.";
  }
  DECLARE_BML_VERSION;

  void OnPreLoadLevel() override {
    clip_cursor();
  }

  void OnStartLevel() override {
    clip_cursor();
  }
  
  void OnUnpauseLevel() override {
    clip_cursor();
  }

  void OnPauseLevel() override {
    cancel_clip_cursor();
  }

  void OnLevelFinish() override {
    cancel_clip_cursor();
  }

  void OnPreExitLevel() override {
    cancel_clip_cursor();
  }
};

IMod* BMLEntry(IBML* bml) {
  return new CursorClipper(bml);
}
