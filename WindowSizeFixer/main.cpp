#pragma once

#include "../bml_includes.hpp"
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#endif // !WIN32_MEAN_AND_LEAN

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class WindowSizeFixer : public IMod {

public:
  WindowSizeFixer(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "WindowSizeFixer"; }
  virtual iCKSTRING GetVersion() override { return "0.0.1"; }
  virtual iCKSTRING GetName() override { return "Window Size Fixer"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override {
    return "A mod for fixing your window size in windowed mode.\n\n"
    "The original Ballance Player has a problem where the window size is slightly"
    " smaller than the actual display size in windowed mode so that the bottom-right"
    " portion of the display is clipped and must be manually resized to reveal.\n"
    "This mod fixes the said problem by resizing the window to match its intended"
    " size and centering the resized window on startup.";
  }
  DECLARE_BML_VERSION;

  class smart_hkey {
  private:
    HKEY hKey{};
  public:
    HKEY &data() { return hKey; }
    smart_hkey(HKEY hKey = {}) : hKey(hKey) {}
    ~smart_hkey() { RegCloseKey(hKey); }
  };

  void OnLoad() override {
    smart_hkey hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\ballance\\Settings", 0, KEY_READ, &hKey.data())
        != ERROR_SUCCESS)
      return;
    DWORD fullscreen {}, buffer_size {sizeof(DWORD)};
    if (RegQueryValueEx(hKey.data(), "FullScreen", 0, 0, reinterpret_cast<LPBYTE>(&fullscreen), &buffer_size)
        != ERROR_SUCCESS || fullscreen)
      return;

    auto handle = static_cast<HWND>(m_bml->GetCKContext()->GetMainWindow());
    VxRect display_rect; m_bml->GetRenderContext()->GetWindowRect(display_rect);
    const int display_width = display_rect.GetWidth();
    const int display_height = display_rect.GetHeight();
    RECT window_rect; GetClientRect(handle, &window_rect);

    if (window_rect.right - window_rect.left >= display_width && window_rect.bottom - window_rect.top >= display_height)
      return;

    window_rect.right = window_rect.left + display_width;
    window_rect.bottom = window_rect.top + display_height;
    AdjustWindowRectEx(&window_rect, GetWindowLong(handle, GWL_STYLE), false, GetWindowLong(handle, GWL_EXSTYLE));
    const auto window_width = window_rect.right - window_rect.left;
    const auto window_height = window_rect.bottom - window_rect.top;
    SetWindowPos(handle, handle,
      (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2, (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2,
      window_width, window_height,
      SWP_NOZORDER
    );
  }
};

IMod* BMLEntry(IBML* bml) {
  return new WindowSizeFixer(bml);
}
