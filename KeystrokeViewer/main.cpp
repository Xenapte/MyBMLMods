#pragma once

#include <thread>
#include <memory>
#include <unordered_map>
#include <functional>
#include "../bml_includes.hpp"
#include "../utils.hpp"


extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class KeystrokeViewer : public IMod {
  std::unique_ptr<BGui::Text> sprite_;
  CKSPRITETEXT_ALIGNMENT sprite_alignment_ = CKSPRITETEXT_CENTER;
  bool init_ = false;
  float x_pos_ = 0.7f, y_pos_ = 0.7f;
  float x_width_ = 0.25f;
  int font_size_ = 16;
  float max_line_length_{};
  utils utils_;
  IProperty *prop_y_{}, *prop_font_size_{}, *prop_x_{}, *prop_x_width_{},
      *prop_all_keys_mode_{};
  decltype(m_bml->GetInputManager()) input_manager_{};
  std::string old_text_;
  inline static std::string key_names[] =
      { "F1", "F2", "Enter", "Shift", "↑", "Space", "←", "↓", "→" };
  inline static constexpr const int keys_length = sizeof(key_names) / sizeof(std::string);
  inline static int key_name_lengths[keys_length] = {};
  CKKEYBOARD keys_[keys_length] =
      { CKKEY_F1, CKKEY_F2, CKKEY_RETURN, CKKEY_LSHIFT, CKKEY_UP,
        CKKEY_SPACE, CKKEY_LEFT, CKKEY_DOWN, CKKEY_RIGHT };
  IProperty *prop_keys_[keys_length]{};
  std::function<std::string()> get_display_text{}; // I'm too lazy to use classes

  std::string get_all_keys_status() {
    std::string text = "\n";
    auto* all_keys = input_manager_->GetKeyboardState();
    for (int i = 0; i < 256; i++) {
      if (!all_keys[i])
        continue;
      char name[32];
      input_manager_->GetKeyName(i, name);
      if (text.length() - text.rfind('\n') > size_t(max_line_length_)) text += '\n';
      else if (text[text.length() - 1] != '\n') text += ' ';
      text += name;
    }
    text.erase(0, 1);
    return text;
  }
  std::string get_gameplay_keys_status() {
    std::string text;
    text.reserve(64);
    auto* all_keys = input_manager_->GetKeyboardState();
    static constexpr const char *separators[keys_length] =
        { " ", " ", "\n", "   ", "  \n", " ", " ", " ", "" };
    for (int i = 0; i < keys_length; i++) {
      char name[12];
      std::snprintf(name, sizeof(name), "%*s%s",
                    key_name_lengths[i], all_keys[keys_[i]] ? key_names[i].c_str() : "",
                    separators[i]);
      text.append(name);
    }
    return text;
  };

public:
  KeystrokeViewer(IBML* bml) : IMod(bml), utils_(bml) {
    for (int i = 0; i < keys_length; i++) {
      auto wname = utils::utf8_to_wide(key_names[i]);
      key_name_lengths[i] = int(wname.length());
      key_names[i] = utils::wide_to_ansi(wname);
    }
  }

  virtual iCKSTRING GetID() override { return "KeystrokeViewer"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "Keystroke Viewer"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "Displays keys you're pressing on your keyboard."; }
  DECLARE_BML_VERSION;

  void OnProcess() override {
    if (!(m_bml->IsIngame() && sprite_))
      return;

    auto text = get_display_text();
    if (text == old_text_)
      return;
    old_text_ = text;
    sprite_->SetText(text.c_str());
  }

  void OnPostStartMenu() override {
    if (init_)
      return;

    show_sprite();
    init_ = true;
  }

  void OnStartLevel() override {
    if (!sprite_)
      show_sprite();
  }

  void OnPreExitLevel() override {
    if (sprite_)
      hide_sprite();
  }

  void OnLoad() override {
    input_manager_ = m_bml->GetInputManager();
    GetConfig()->SetCategoryComment("Appearances", "The look and feel of your key display.");
    prop_x_ = GetConfig()->GetProperty("Main", "X_Position");
    prop_x_->SetComment("X position of the left of sprite text.");
    prop_x_->SetDefaultFloat(x_pos_);
    prop_x_width_ = GetConfig()->GetProperty("Main", "X_Width");
    prop_x_width_->SetComment("Maximum width of the sprite text.");
    prop_x_width_->SetDefaultFloat(x_width_);
    prop_y_ = GetConfig()->GetProperty("Main", "Y_Position");
    prop_y_->SetComment("Y position of the top of sprite text.");
    prop_y_->SetDefaultFloat(y_pos_);
    prop_font_size_ = GetConfig()->GetProperty("Main", "Font_Size");
    prop_font_size_->SetComment("Font size of sprite text.");
    prop_font_size_->SetDefaultFloat(float(font_size_));
    GetConfig()->SetCategoryComment("Mode", "Mode.");
    prop_all_keys_mode_ = GetConfig()->GetProperty("Mode", "All_Keys_Mode");
    prop_all_keys_mode_->SetComment("Whether to display all pressed all_keys.");
    prop_all_keys_mode_->SetDefaultBoolean(false);
    GetConfig()->SetCategoryComment("Keys", "Keys to track.");
    for (int i = 0; i < keys_length; i++) {
      if (key_names[i] == "Enter") continue;
      char name[16]{};
      input_manager_->GetKeyName(keys_[i], name);
      for (size_t j = 0; j < strlen(name); j++)
        if (std::isspace(name[j])) name[j] = '_';
      prop_keys_[i] = GetConfig()->GetProperty("Keys", (std::string{name} + "_Key").c_str());
      prop_keys_[i]->SetDefaultKey(keys_[i]);
    }
    load_config();
  }

  void OnModifyConfig(iCKSTRING category, iCKSTRING key, IProperty* prop) override {
    load_config();
    if (sprite_) {
      hide_sprite();
      show_sprite();
    }
  }

private:
  void show_sprite() {
    if (sprite_)
      return;
    sprite_ = std::make_unique<decltype(sprite_)::element_type>("KeyDisplay");
    sprite_->SetSize({ x_width_, 0.4f });
    sprite_->SetPosition({ x_pos_, y_pos_ });
    sprite_->SetAlignment(sprite_alignment_);
    sprite_->SetTextColor(0xffffffff);
    sprite_->SetZOrder(128);
    sprite_->SetFont("Consolas", font_size_, 400, false, false);
    sprite_->SetVisible(true);
  }

  void hide_sprite() { sprite_.reset(); }

  void load_config() {
    x_pos_ = prop_x_->GetFloat();
    x_width_ = prop_x_width_->GetFloat();
    y_pos_ = prop_y_->GetFloat();
    font_size_ = utils_.get_bgui_font_size(prop_font_size_->GetFloat());
    max_line_length_ = x_width_ * 1024 / font_size_ / 1.05f;
    if (prop_all_keys_mode_->GetBoolean()) {
      get_display_text = [this] { return get_all_keys_status(); };
      sprite_alignment_ = CKSPRITETEXT_CENTER;
    }
    else {
      get_display_text = [this] { return get_gameplay_keys_status(); };
      sprite_alignment_ = CKSPRITETEXT_LEFT;
    }
    for (int i = 0; i < keys_length; i++) {
      if (key_names[i] == "Enter") continue;
      keys_[i] = prop_keys_[i]->GetKey();
    }
  }
};

IMod* BMLEntry(IBML* bml) {
  return new KeystrokeViewer(bml);
}
