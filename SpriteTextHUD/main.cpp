#pragma once

#include "../bml_includes.hpp"
#include <memory>
#include "../utils.hpp"

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class SpriteTextHUD : public IMod {
  bool init = false;
  IProperty* prop_sr{}, * prop_fps{},
    * prop_sr_color{}, * prop_fps_color{}, * prop_sr_font{}, * prop_fps_font{};
  bool sr_active = false;
  float sr_timer = 0;
  int fps_count = 0, fps_timer = 0;
  CKDWORD sr_color = 0xffffffff, fps_color = 0xffffffff;

  std::unique_ptr<BGui::Text> sr_timer_title, sr_timer_score, fps_text;

  utils utils;

  void init_gui() {
    if (prop_sr->GetBoolean()) {
      sr_timer_title = std::make_unique<decltype(sr_timer_title)::element_type>("SpriteTextHUD_SR_Title");
      sr_timer_title->SetSize({ 0.2f, 0.03f });
      sr_timer_title->SetPosition({ 0.03f, 0.8f });
      sr_timer_title->SetAlignment(CKSPRITETEXT_LEFT);
      sr_timer_title->SetTextColor(sr_color);
      sr_timer_title->SetZOrder(128);
      sr_timer_title->SetFont(prop_sr_font->GetString(), utils.get_bgui_font_size(12), 400, false, false);
      sr_timer_title->SetText("SR Timer");
      sr_timer_title->SetVisible(m_bml->IsIngame());

      sr_timer_score = std::make_unique<decltype(sr_timer_title)::element_type>("SpriteTextHUD_SR_Score");
      sr_timer_score->SetSize({ 0.2f, 0.03f });
      sr_timer_score->SetPosition({ 0.05f, 0.83f });
      sr_timer_score->SetAlignment(CKSPRITETEXT_LEFT);
      sr_timer_score->SetTextColor(sr_color);
      sr_timer_score->SetZOrder(128);
      sr_timer_score->SetFont(prop_sr_font->GetString(), utils.get_bgui_font_size(12), 400, false, false);
      sr_timer_score->SetVisible(m_bml->IsIngame());
      if (m_bml->IsIngame())
        update_sr_text();
    }

    if (prop_fps->GetBoolean()) {
      fps_text = std::make_unique<decltype(sr_timer_title)::element_type>("SpriteTextHUD_FPS");
      fps_text->SetSize({ 0.2f, 0.03f });
      fps_text->SetPosition({ 0.005f, 0.005f });
      fps_text->SetAlignment(CKSPRITETEXT_LEFT);
      fps_text->SetTextColor(fps_color);
      fps_text->SetZOrder(128);
      fps_text->SetFont(prop_fps_font->GetString(), utils.get_bgui_font_size(12), 400, false, false);
      fps_text->SetVisible(true);
    }
  }

  // color: fallback color
  CKDWORD parse_prop_color(IProperty* prop, CKDWORD color) {
    char color_text[8]{};
    try {
      color = (CKDWORD)std::stoul(prop->GetString(), nullptr, 16);
    }
    catch (const std::exception& e) {
      GetLogger()->Warn("Error parsing the color code: %s. Resetting to %06X.", e.what(), color & 0x00FFFFFF);
    }
    snprintf(color_text, sizeof(color_text), "%06X", color & 0x00FFFFFF);
    prop->SetString(color_text);
    return color | 0xFF000000;
  }

  void load_config() {
    sr_color = parse_prop_color(prop_sr_color, 0xFFFFFFFF);
    fps_color = parse_prop_color(prop_fps_color, 0xFFFFFFFF);
    sr_timer_title.reset();
    sr_timer_score.reset();
    fps_text.reset();
    init_gui();
  }

  void update_sr_text() {
    int counter = int(sr_timer);
    int ms = counter % 1000;
    counter /= 1000;
    int s = counter % 60;
    counter /= 60;
    int m = counter % 60;
    counter /= 60;
    int h = counter % 100;
    static char time[16];
    snprintf(time, sizeof(time), "%02d:%02d:%02d.%03d", h, m, s, ms);
    sr_timer_score->SetText(time);
  }

public:
  SpriteTextHUD(IBML* bml) : IMod(bml), utils(bml) {}

  virtual iCKSTRING GetID() override { return "SpriteTextHUD"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "SpriteTextHUD"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "Unhides hidden objects in your game."; }
  DECLARE_BML_VERSION;

  void OnLoad() override {
    auto config = GetConfig();
    prop_sr = config->GetProperty("Main", "ShowSRTimer");
    prop_sr->SetDefaultBoolean(true);
    prop_fps = config->GetProperty("Main", "ShowFPS");
    prop_fps->SetDefaultBoolean(true);
    prop_sr_color = config->GetProperty("Style", "SRTimerColor");
    prop_sr_color->SetDefaultString("FFFFFF");
    prop_sr_color->SetComment("Color of SR Timer in HEX format.");
    prop_fps_color = config->GetProperty("Style", "FPSColor");
    prop_fps_color->SetDefaultString("FFFFFF");
    prop_fps_color->SetComment("Color of FPS Display in HEX format.");
    prop_sr_font = config->GetProperty("Style", "SRTimerFont");
    prop_sr_font->SetDefaultString("Arial");
    prop_sr_font->SetComment("Font of SR Timer.");
    prop_fps_font = config->GetProperty("Style", "FPSFont");
    prop_fps_font->SetDefaultString("Arial");
    prop_fps_font->SetComment("Font of FPS Display.");
  }

  void OnPostStartMenu() override {
    if (init) return;

    load_config();

    init = true;
  }

  void OnProcess() override {
    if (sr_active) {
      sr_timer += m_bml->GetTimeManager()->GetLastDeltaTime();
      if (sr_timer_score) update_sr_text();
    }
    if (fps_text) {
      CKStats stats;
      m_bml->GetCKContext()->GetProfileStats(&stats);
      fps_count += int(1000 / stats.TotalFrameTime);
      if (++fps_timer == 90) {
        char text[16];
        snprintf(text, sizeof(text), "FPS: %d", (fps_count + 45) / 90);
        fps_text->SetText(text);
        fps_timer = 0;
        fps_count = 0;
      }
    }
  }

  void OnModifyConfig(CKSTRING category, CKSTRING key, IProperty* prop) {
    load_config();
  }

  void OnStartLevel() override {
    sr_timer = 0;
    update_sr_text();
    sr_timer_title->SetVisible(true);
    sr_timer_score->SetVisible(true);
  }

  void OnPostExitLevel() override {
    sr_timer_title->SetVisible(false);
    sr_timer_score->SetVisible(false);
  }

  void OnPauseLevel() override {
    sr_active = false;
  }

  void OnUnpauseLevel() override {
    sr_active = true;
  }

  void OnCounterActive() override {
    sr_active = true;
  }

  void OnCounterInactive() override {
    sr_active = false;
  }
};

IMod* BMLEntry(IBML* bml) {
  return new SpriteTextHUD(bml);
}
