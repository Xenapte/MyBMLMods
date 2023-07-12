#pragma once

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include <memory>
#include <BMLPlus/BMLAll.h>

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

typedef const char* C_CKSTRING;

class AdvancedTravelCam: public IMod {
private:
  bool is_in_travel_cam_ = false;
  CKCamera *travel_cam_{}, *translation_ref_{};
  decltype(m_bml->GetInputManager()) input_manager_{};
  IProperty *prop_horizontal_sensitivity_{}, *prop_vertical_sensitivity_{}, *prop_mouse_sensitivity_{},
    *prop_relative_direction_{}, *prop_cinematic_camera_{},
    *prop_cinematic_motion_speed_{}, *prop_cinematic_mouse_speed_{},
    *prop_maximum_view_distance_{},
    *prop_commands_[2]{},
    *prop_key_forward{}, *prop_key_backward{}, *prop_key_left{}, *prop_key_right{},
    *prop_key_up{}, *prop_key_down{}, *prop_key_boost{}, *prop_key_zoom{};
  CKKEYBOARD key_forward{}, key_backward{}, key_left{}, key_right{},
    key_up{}, key_down{}, key_boost{}, key_zoom{};
  float horizontal_sensitivity_{}, vertical_sensitivity_{}, mouse_sensitivity_{};
  bool cinematic_camera_{};
  float cinematic_motion_speed_{}, cinematic_mouse_speed_{};
  VxVector remaining_horizontal_distance_{}, remaining_mouse_distance_{};
  float remaining_vertical_distance_{};
  float default_fov_{}, desired_fov_{};

  CKCamera *get_ingame_cam();

  void load_config_values();

  void clip_cursor();
  void cancel_clip_cursor();
  const HCURSOR cursor_arrow = LoadCursor(NULL, IDC_ARROW), cursor_cross = LoadCursor(NULL, IDC_CROSS);

  std::pair<CK3dEntity*, CKPICKRESULT> pick_screen();
  std::pair<float, float> get_distance_factors();

public:
  AdvancedTravelCam(IBML* bml): IMod(bml) {}

  virtual C_CKSTRING GetID() override { return "AdvancedTravelCam"; }
  virtual C_CKSTRING GetVersion() override { return "0.1.0"; }
  virtual C_CKSTRING GetName() override { return "Advanced Travel Camera"; }
  virtual C_CKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual C_CKSTRING GetDescription() override {
    return "Advanced Travel Camera";
  }
  DECLARE_BML_VERSION;

  void OnDead() override;
  void OnLoad() override;
  void OnModifyConfig(C_CKSTRING category, C_CKSTRING key, IProperty* prop) override;
  void OnPauseLevel() override;
  void OnUnpauseLevel() override;
  void OnPreResetLevel() override;
  void OnPreEndLevel() override;
  void OnPreExitLevel() override;
  void OnPreNextLevel() override;
  void OnProcess() override;

  void enter_travel_cam();
  void exit_travel_cam(bool local_state_only = false);

  inline bool is_in_travel_cam() { return is_in_travel_cam_; }
  inline bool is_playing() { return m_bml->IsPlaying(); }

  inline std::string get_command_1() { return prop_commands_[0]->GetString(); }
  inline std::string get_command_2() { return prop_commands_[1]->GetString(); }

  class TravelCommand: public ICommand {
  private:
    AdvancedTravelCam *mod_;

    std::string GetName() override { return mod_->get_command_1(); };
    std::string GetAlias() override { return mod_->get_command_2(); };
    std::string GetDescription() override { return "Enter Advanced Travel Camera."; };
    bool IsCheat() override { return false; };

    void Execute(IBML* bml, const std::vector<std::string>& args) override;

    const std::vector<std::string> GetTabCompletion(IBML* bml, const std::vector<std::string>& args) override;

  public:
    TravelCommand(AdvancedTravelCam *mod): mod_(mod) {}
  };
};
