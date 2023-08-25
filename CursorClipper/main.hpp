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
  // can't really use IBML::IsPlaying because that doesn't fit our need
  bool is_playing = false;
  HWINEVENTHOOK hook{};

public:
  CursorClipper(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "CursorClipper"; }
  virtual iCKSTRING GetVersion() override { return "0.0.2"; }
  virtual iCKSTRING GetName() override { return "Cursor Clipper"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override {
    return "A mod for limiting your cursor inside the game window.";
  }
  DECLARE_BML_VERSION;

  void OnPreLoadLevel() override;
  void OnStartLevel() override;
  void OnUnpauseLevel() override;
  void OnPauseLevel() override;
  void OnLevelFinish() override;
  void OnPreExitLevel() override;

  void OnLoad() override;
  void OnUnload() override;

  // fail if the game window is not active or is not playing
  void clip_cursor(bool set_playing_status = true);
  void cancel_clip_cursor();
};
