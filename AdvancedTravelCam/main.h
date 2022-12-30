#pragma once

#include <memory>
#include <BML/BMLAll.h>

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class AdvancedTravelCam: public IMod {
private:
  bool is_in_travel_cam_ = false;
  CKCamera *travel_cam_{}, *translation_ref_{};
  decltype(m_bml->GetInputManager()) input_manager_{};
  IProperty *prop_horizontal_sensitivity_{}, *prop_vertical_sensitivity_{}, *prop_mouse_sensitivity_{},
    *prop_relative_direction_{}, *prop_cinematic_camera_{},
    *prop_cinematic_motion_speed_{}, *prop_cinematic_mouse_speed_{},
    *prop_maximum_view_distance_{};
  float horizontal_sensitivity_{}, vertical_sensitivity_{}, mouse_sensitivity_{};
  bool cinematic_camera_{};
  float cinematic_motion_speed_{}, cinematic_mouse_speed_{};
  VxVector remaining_horizontal_distance_{}, remaining_mouse_distance_{};
  float remaining_vertical_distance_{};

  CKCamera *get_ingame_cam();

  void load_config_values();

  void clip_cursor();
  void cancel_clip_cursor();

public:
  AdvancedTravelCam(IBML* bml): IMod(bml) {}

  virtual CKSTRING GetID() override { return "AdvancedTravelCam"; }
  virtual CKSTRING GetVersion() override { return "0.0.3"; }
  virtual CKSTRING GetName() override { return "Advanced Travel Camera"; }
  virtual CKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual CKSTRING GetDescription() override {
    return
      "Advanced Travel Camera\n\n"
      "Controls:\n"
      "W/A/S/D: horizontal movement;\n"
      "Space/Shift: vertical movement;\n"
      "Additionally, the left Ctrl key can be used for a movement speed boost.";
  }
  DECLARE_BML_VERSION;

  void OnDead() override;
  void OnLoad() override;
  void OnModifyConfig(CKSTRING category, CKSTRING key, IProperty* prop) override;
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

  class TravelCommand: public ICommand {
  private:
    AdvancedTravelCam *mod;

    std::string GetName() override { return "advancedtravel"; };
    std::string GetAlias() override { return "+travel"; };
    std::string GetDescription() override { return "Enter Advanced Travel Camera."; };
    bool IsCheat() override { return false; };

    void Execute(IBML* bml, const std::vector<std::string>& args) override;

    const std::vector<std::string> GetTabCompletion(IBML* bml, const std::vector<std::string>& args) override;

  public:
    TravelCommand(AdvancedTravelCam *mod): mod(mod) {}
  };
};
