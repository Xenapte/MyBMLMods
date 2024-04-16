#include "main.h"
#include <numbers>

IMod* BMLEntry(IBML* bml) {
  return new StaticCamera(bml);
}

// Vx(*)Vector::Normalize: not supported on Virtools 2.1?
inline bool normalize(float& x, float& y) {
  const auto square_sum = x * x + y * y;
  if (square_sum == 0) return false;
  const auto factor = std::sqrt(1 / square_sum);
  x *= factor, y *= factor;
  return true;
};

CKCamera* StaticCamera::get_ingame_cam() {
  return m_bml->GetTargetCameraByName("InGameCam");
}

CK3dObject* StaticCamera::get_current_ball() {
  return static_cast<CK3dObject*>((current_level_array_)->GetElementObject(0, 1));
}

void StaticCamera::load_config() {
  enabled_ = prop_enabled_->GetBoolean();
  key_sync_ = prop_key_sync_->GetKey();
  key_rotate_ = prop_key_rotate_->GetKey();
  if (m_bml->IsIngame()) {
    if (enabled_) enter_static_cam();
    else exit_static_cam();
  }
  if (prop_alt_key_check_->GetBoolean()) {
    is_key_down_ = [this](CKKEYBOARD key) { return m_bml->GetInputManager()->oIsKeyDown(key); };
    is_key_pressed_ = [this](CKKEYBOARD key) { return m_bml->GetInputManager()->oIsKeyPressed(key); };
  }
  else {
    is_key_down_ = [this](CKKEYBOARD key) { return m_bml->GetInputManager()->IsKeyDown(key); };
    is_key_pressed_ = [this](CKKEYBOARD key) { return m_bml->GetInputManager()->IsKeyPressed(key); };
  }
}

void StaticCamera::OnLoad() {
  static_cam_ = static_cast<CKCamera*>(m_bml->GetCKContext()->CreateObject(CKCID_CAMERA, "StaticCamera"));
  GetConfig()->SetCategoryComment("Main", "Main");
  prop_enabled_ = GetConfig()->GetProperty("Main", "Enabled");
  prop_enabled_->SetDefaultBoolean(true);
  enabled_ = prop_enabled_->GetBoolean();
  prop_key_rotate_ = GetConfig()->GetProperty("Main", "CameraRotateKey");
  prop_key_rotate_->SetDefaultKey(CKKEY_LSHIFT);
  prop_key_rotate_->SetComment("The key for rotating the camera.");
  prop_key_left_ = GetConfig()->GetProperty("Main", "LeftRotateKey");
  prop_key_left_->SetDefaultKey(CKKEY_LEFT);
  prop_key_right_ = GetConfig()->GetProperty("Main", "RightRotateKey");
  prop_key_right_->SetDefaultKey(CKKEY_RIGHT);
  prop_key_sync_ = GetConfig()->GetProperty("Main", "CameraSyncKey");
  prop_key_sync_->SetDefaultKey(CKKEY_RSHIFT);
  prop_key_sync_->SetComment("The key to sync your view with the default ingame camera.");
  prop_alt_key_check_ = GetConfig()->GetProperty("Main", "AlternativeKeyCheck");
  prop_alt_key_check_->SetDefaultBoolean(false);
  prop_alt_key_check_->SetComment("Whether to use the alternative keypress checking method.");
  load_config();
}

void StaticCamera::OnModifyConfig(iCKSTRING category, iCKSTRING key, IProperty* prop) {
  load_config();
}

void StaticCamera::OnPostStartMenu() {
  if (init_) return;

  current_level_array_ = m_bml->GetArrayByName("CurrentLevel");

  init_ = true;
}

void StaticCamera::OnBallOff() {
  pending_reset_ = true;
}

void StaticCamera::OnStartLevel() {
  pending_reset_ = true;
}

void StaticCamera::OnCamNavActive() {
  if (!enabled_ || !pending_reset_) return;
  enter_static_cam();
  pending_reset_ = false;
}

void StaticCamera::rotate_static_cam(float factor) {
  cam_pos_diff_ = {-cam_pos_diff_.z * factor, cam_pos_diff_.y, cam_pos_diff_.x * factor};
  static_cam_->Rotate({0, 1, 0}, std::numbers::pi_v<float> / 2 * factor);
}

void StaticCamera::OnProcess() {
  if (!enabled_ || !is_playing()) return;
  if (is_key_down_(key_sync_)) {
    reset_static_cam();
  }
  VxVector pos;
  get_current_ball()->GetPosition(&pos);
  pos += cam_pos_diff_;
  static_cam_->SetPosition(pos);
  if (is_key_down_(key_rotate_)) {
    float current_time = m_bml->GetTimeManager()->GetTime();
    if (current_time - last_rotation_timestamp_ < 250.0f) return;
    if (is_key_pressed_(prop_key_left_->GetKey())) rotate_static_cam(-1);
    else if (is_key_pressed_(prop_key_right_->GetKey())) rotate_static_cam(1);
    else return;
    last_rotation_timestamp_ = current_time;
  }
}

void StaticCamera::enter_static_cam() {
  CKCamera* cam = get_ingame_cam();
  int width, height;
  cam->GetAspectRatio(width, height);
  static_cam_->SetAspectRatio(width, height);
  reset_static_cam();
  m_bml->GetRenderContext()->AttachViewpointToCamera(static_cam_);
}

void StaticCamera::exit_static_cam() {
  m_bml->GetRenderContext()->AttachViewpointToCamera(get_ingame_cam());
}

void StaticCamera::reset_static_cam() {
  CKCamera* cam = get_ingame_cam();
  static_cam_->SetWorldMatrix(cam->GetWorldMatrix());
  static_cam_->SetFov(cam->GetFov());
  VxVector cam_pos, ball_pos;
  static_cam_->GetPosition(&cam_pos);
  get_current_ball()->GetPosition(&ball_pos);
  cam_pos_diff_ = cam_pos - ball_pos;
}
