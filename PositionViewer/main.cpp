#pragma once

#include "../bml_includes.hpp"
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#endif // !WIN32_MEAN_AND_LEAN
#include <thread>
#include <memory>
#include "../utils.hpp"


extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class PositionViewer : public IMod {
  std::unique_ptr<BGui::Text> sprite;
  bool init = false;
  CKDataArray *current_level_array{};
  float y_pos = 0.9f;
  int font_size = 12;
  float pos_interval = 0, speed_interval = 0;
  IProperty *prop_y{}, *prop_font_size{}, *prop_pos_interval{}, *prop_speed_interval{};
  VxVector last_pos{}, ref_pos{};
  float last_speed = 0, last_speed_timestamp = 0;
  float next_pos_update = 0, next_speed_update = 0;
  utils utils;

  auto get_player_ball() {
    return static_cast<CK3dObject*>(current_level_array->GetElementObject(0, 1));
  }

public:
  PositionViewer(IBML* bml) : IMod(bml), utils(bml) {}

  virtual iCKSTRING GetID() override { return "PositionViewer"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "Position Viewer"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "Displays the coordinates of your player ball."; }
  DECLARE_BML_VERSION;

  void OnProcess() override {
    if (!(m_bml->IsPlaying() && sprite)) 
      return;

    bool pos_update = false, speed_update = false;

    float current_time = m_bml->GetTimeManager()->GetTime();
    float pos_diff = current_time - next_pos_update, speed_diff = current_time - next_speed_update;
    if (pos_diff >= 0) {
      pos_update = true;
      if (pos_diff >= 10000)
        next_pos_update = current_time;
      next_pos_update += pos_interval;
    }
    if (speed_diff >= 0) {
      speed_update = true;
      if (speed_diff >= 10000)
        next_speed_update = current_time;
      next_speed_update += speed_interval;
    }

    if (pos_update)
      get_player_ball()->GetPosition(&last_pos);
    if (speed_update) {
      VxVector current_pos; get_player_ball()->GetPosition(&current_pos);
      last_speed = Magnitude(current_pos - ref_pos) / (current_time - last_speed_timestamp) * 1e3f;
      last_speed_timestamp = current_time;
      ref_pos = current_pos;
    }
    if (pos_update || speed_update) {
      char text[128];
      snprintf(text, sizeof(text), "X: %.3f, Y: %.3f, Z: %.3f\n%.2f m/s",
               last_pos.x, last_pos.y, last_pos.z, last_speed);
      sprite->SetText(text);
    };
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
    prop_pos_interval = GetConfig()->GetProperty("Main", "Position_Update_Interval");
    prop_pos_interval->SetComment("Minimum update interval for position data. Default: 50.0 (milliseconds)");
    prop_pos_interval->SetDefaultFloat(50.0f);
    prop_speed_interval = GetConfig()->GetProperty("Main", "Speed_Update_Interval");
    prop_speed_interval->SetComment("Minimum update interval for speed data. Default: 100.0 (milliseconds)");
    prop_speed_interval->SetDefaultFloat(100.0f);
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
    sprite->SetSize({ 0.6f, 0.12f });
    sprite->SetPosition({ 0.2f, y_pos });
    sprite->SetAlignment(CKSPRITETEXT_CENTER);
    sprite->SetTextColor(0xffffffff);
    sprite->SetZOrder(128);
    sprite->SetFont("Arial", font_size, 400, false, false);
    next_pos_update = next_speed_update = last_speed_timestamp = m_bml->GetTimeManager()->GetTime();
  }

  void hide_ball_pos() { sprite.reset(); }

  void load_config() {
    y_pos = prop_y->GetFloat();
    font_size = utils.get_bgui_font_size(prop_font_size->GetFloat());
    pos_interval = prop_pos_interval->GetFloat();
    speed_interval = prop_speed_interval->GetFloat();
  }
};

IMod* BMLEntry(IBML* bml) {
  return new PositionViewer(bml);
}
