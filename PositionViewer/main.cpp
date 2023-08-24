#pragma once

#include "../bml_includes.hpp"
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#endif // !WIN32_MEAN_AND_LEAN
#include <thread>
#include <memory>

// Windows 7 does not have GetDpiForSystem
typedef UINT(WINAPI* GetDpiForSystemPtr) (void);
GetDpiForSystemPtr const get_system_dpi = [] {
  auto hMod = GetModuleHandleW(L"user32.dll");
  if (hMod) {
    return (GetDpiForSystemPtr)GetProcAddress(hMod, "GetDpiForSystem");
  }
  return (GetDpiForSystemPtr)nullptr;
}();

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class PositionViewer : public IMod {
  std::unique_ptr<BGui::Text> sprite;
  bool init = false;
  std::thread pos_thread;
  CKDataArray *current_level_array{};
  float y_pos = 0.9f;
  int font_size = 12;
  IProperty *prop_y{}, *prop_font_size{};

public:
  PositionViewer(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "PositionViewer"; }
  virtual iCKSTRING GetVersion() override { return "0.0.1"; }
  virtual iCKSTRING GetName() override { return "Position Viewer"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "Displays the coordinates of your player ball."; }
  DECLARE_BML_VERSION;

  void OnProcess() override {
    if (!(m_bml->IsPlaying() && sprite)) 
      return;

    auto* player_ball = static_cast<CK3dObject*>(current_level_array->GetElementObject(0, 1));
    VxVector pos; player_ball->GetPosition(&pos);
    char pos_text[128];
    snprintf(pos_text, sizeof(pos_text), "X: %.3f, Y: %.3f, Z: %.3f", pos.x, pos.y, pos.z);
    sprite->SetText(pos_text);
  }

  void OnPostStartMenu() override {
    if (init)
      return;

    current_level_array = m_bml->GetArrayByName("CurrentLevel");
    show_ball_pos();
    init = true;
  }

  void OnStartLevel() override {
    if (!sprite)
      show_ball_pos();
  }

  void OnPreExitLevel() override {
    if (sprite)
      hide_ball_pos();
  }

  void OnLoad() override {
    GetConfig()->SetCategoryComment("Main", "Main settings.");
    prop_y = GetConfig()->GetProperty("Main", "Y_Position");
    prop_y->SetComment("Y position of the top of sprite text.");
    prop_y->SetDefaultFloat(0.9f);
    prop_font_size = GetConfig()->GetProperty("Main", "Font_Size");
    prop_font_size->SetComment("Font size of sprite text.");
    prop_font_size->SetDefaultFloat(12);
    load_config();
  }

  void OnModifyConfig(iCKSTRING category, iCKSTRING key, IProperty* prop) override {
    load_config();
    if (sprite) {
      hide_ball_pos();
      show_ball_pos();
    }
  }

private:
  void show_ball_pos() {
    if (sprite)
      return;
    sprite = std::make_unique<decltype(sprite)::element_type>("PositionDisplay");
    sprite->SetSize({ 0.6f, 0.05f });
    sprite->SetPosition({ 0.2f, y_pos });
    sprite->SetAlignment(CKSPRITETEXT_CENTER);
    sprite->SetTextColor(0xffffffff);
    sprite->SetZOrder(128);
    sprite->SetFont("Arial", font_size, 400, false, false);
  }

  void hide_ball_pos() { sprite.reset(); }

  void load_config() {
    y_pos = prop_y->GetFloat();
    font_size = (int)std::round(m_bml->GetRenderContext()->GetHeight() / (768.0f / 119) * prop_font_size->GetFloat() / ((get_system_dpi == nullptr) ? 96 : get_system_dpi()));
  }
};

IMod* BMLEntry(IBML* bml) {
  return new PositionViewer(bml);
}
