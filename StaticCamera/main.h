#pragma once

#include <memory>
#include <functional>
#include "../bml_includes.hpp"


extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class StaticCamera: public IMod {
private:
  bool enabled_ = false, init_ = false;
  bool pending_reset_ = false;
  CKCamera *static_cam_{};
  IProperty *prop_enabled_{}, *prop_key_rotate_{}, *prop_key_sync_{}, *prop_alt_key_check_{};
  IProperty *prop_key_left_{}, *prop_key_right_{};
  std::function<bool(CKKEYBOARD)> is_key_down_{}, is_key_pressed_{};
  VxVector cam_pos_diff_{};
  CKDataArray *current_level_array_{};
  CKKEYBOARD key_sync_{}, key_rotate_{};
  float last_rotation_timestamp_ = 0;

  CKCamera *get_ingame_cam();
  CK3dObject *get_current_ball();
  void load_config();
  void rotate_static_cam(float factor);

public:
  StaticCamera(IBML* bml): IMod(bml) {}

  virtual iCKSTRING GetID() override { return "StaticCamera"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "Static Camera"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override {
    return "Static Camera is an alternative ingame camera for Ballance where the camera always keeps the same distance/angle to the player ball.";
  }
  DECLARE_BML_VERSION;

  void OnLoad() override;
  void OnModifyConfig(iCKSTRING category, iCKSTRING key, IProperty* prop) override;
  void OnPostStartMenu() override;
  void OnBallOff() override;
  void OnStartLevel() override;
  void OnCamNavActive() override;
  void OnProcess() override;

  void enter_static_cam();
  void exit_static_cam();
  void reset_static_cam();

  inline bool is_playing() { return m_bml->IsPlaying(); }
};
