#include "main.h"

IMod* BMLEntry(IBML* bml) {
  return new AdvancedTravelCam(bml);
}

CKCamera* AdvancedTravelCam::get_ingame_cam() {
  return m_bml->GetTargetCameraByName("InGameCam");
}

void AdvancedTravelCam::load_config_values() {
  horizontal_sensitivity_ = prop_horizontal_sensitivity_->GetFloat();
  vertical_sensitivity_ = prop_vertical_sensitivity_->GetFloat();
  mouse_sensitivity_ = prop_mouse_sensitivity_->GetFloat();
  translation_ref_ = prop_relative_direction_->GetBoolean() ? travel_cam_ : nullptr;
  cinematic_camera_ = prop_cinematic_camera_->GetBoolean();
  cinematic_motion_speed_ = prop_cinematic_motion_speed_->GetFloat();
  cinematic_mouse_speed_ = prop_cinematic_mouse_speed_->GetFloat();
}

void AdvancedTravelCam::OnLoad() {
  m_bml->RegisterCommand(new TravelCommand(this));
  travel_cam_ = static_cast<CKCamera*>(m_bml->GetCKContext()->CreateObject(CKCID_CAMERA, "AdvancedTravelCam"));
  input_manager_ = m_bml->GetInputManager();

  auto config = GetConfig();
  config->SetCategoryComment("Quantities", "Related to movement/rotation sensitivities.");
  config->SetCategoryComment("Qualities", "Related to different behaviors.");
  prop_horizontal_sensitivity_ = config->GetProperty("Quantities", "HorizontalSensitivity");
  prop_horizontal_sensitivity_->SetDefaultFloat(0.05f);
  prop_vertical_sensitivity_ = config->GetProperty("Quantities", "VerticalSensitivity");
  prop_vertical_sensitivity_->SetDefaultFloat(0.04f);
  prop_mouse_sensitivity_ = config->GetProperty("Quantities", "MouseSensitivity");
  prop_mouse_sensitivity_->SetDefaultFloat(2.2f);
  prop_relative_direction_ = config->GetProperty("Qualities", "RelativeDirection");
  prop_relative_direction_->SetDefaultBoolean(true);
  prop_relative_direction_->SetComment(
    "Whether to move the camera relative to its current direction. "
    "If true, horizontal motion inputs will also affect the camera's vertical position."
  );
  prop_cinematic_camera_ = config->GetProperty("Qualities", "CinematicCamera");
  prop_cinematic_camera_->SetDefaultBoolean(false);
  prop_cinematic_camera_->SetComment("Whether to enable cinematic camera (for smooth movement).");
  prop_cinematic_motion_speed_ = config->GetProperty("Quantities", "CinematicMotionSpeed");
  prop_cinematic_motion_speed_->SetDefaultFloat(0.04f);
  prop_cinematic_motion_speed_->SetComment("Must be between 0 and 1.");
  prop_cinematic_mouse_speed_ = config->GetProperty("Quantities", "CinematicMouseSpeed");
  prop_cinematic_mouse_speed_->SetDefaultFloat(0.04f);
  prop_cinematic_mouse_speed_->SetComment("Must be between 0 and 1.");

  load_config_values();
}

void AdvancedTravelCam::OnModifyConfig(C_CKSTRING category, C_CKSTRING key, IProperty* prop) {
  load_config_values();
}

void AdvancedTravelCam::OnPreResetLevel() {
  if (is_in_travel_cam())
    exit_travel_cam(true);
}

void AdvancedTravelCam::OnPreExitLevel() {
  OnPreResetLevel();
}

void AdvancedTravelCam::OnPreNextLevel() {
  OnPreResetLevel();
}

void AdvancedTravelCam::OnProcess() {
  if (!(m_bml->IsPlaying() && is_in_travel_cam()))
    return;

  float yaw = 0;
  if (!translation_ref_) {
    VxQuaternion q;
    travel_cam_->GetQuaternion(&q);
    const float siny_cosp = 2 * (q.w * q.z + q.x * q.y), cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
    yaw = std::atan2(siny_cosp, cosy_cosp);
  }
  const auto delta_time = m_bml->GetTimeManager()->GetLastDeltaTime()
      * (input_manager_->IsKeyDown(CKKEY_LCONTROL) ? 3 : 1),
    delta_distance = horizontal_sensitivity_ * delta_time;
  const auto sin_yaw = std::sin(yaw), cos_yaw = std::cos(yaw);
  if (input_manager_->IsKeyDown(CKKEY_W))
    remaining_horizontal_distance_ += { delta_distance * sin_yaw, 0, delta_distance * cos_yaw };
  if (input_manager_->IsKeyDown(CKKEY_S))
    remaining_horizontal_distance_ += { -delta_distance * sin_yaw, 0, -delta_distance * cos_yaw };
  if (input_manager_->IsKeyDown(CKKEY_A))
    remaining_horizontal_distance_ += { -delta_distance * cos_yaw, 0, delta_distance * sin_yaw };
  if (input_manager_->IsKeyDown(CKKEY_D))
    remaining_horizontal_distance_ += { delta_distance * cos_yaw, 0, -delta_distance * sin_yaw };
  if (input_manager_->IsKeyDown(CKKEY_SPACE))
    remaining_vertical_distance_ += vertical_sensitivity_ * delta_time;
  if (input_manager_->IsKeyDown(CKKEY_LSHIFT))
    remaining_vertical_distance_ += -vertical_sensitivity_ * delta_time;
  VxVector delta_mouse;
  input_manager_->GetMouseRelativePosition(delta_mouse);
  remaining_mouse_distance_ += { -delta_mouse.x * mouse_sensitivity_, -delta_mouse.y * mouse_sensitivity_, 0 };

  VxVector translation_horizontal, translation_mouse;
  float translation_vertical{};
  if (cinematic_camera_) {
    translation_horizontal = Interpolate(cinematic_motion_speed_, {}, remaining_horizontal_distance_);
    translation_vertical = std::lerp(0.0f, remaining_vertical_distance_, cinematic_motion_speed_);
    translation_mouse = Interpolate(cinematic_mouse_speed_, {}, remaining_mouse_distance_);
  }
  else {
    translation_horizontal = remaining_horizontal_distance_;
    translation_vertical = remaining_vertical_distance_;
    translation_mouse = remaining_mouse_distance_;
  }
  remaining_horizontal_distance_ -= translation_horizontal;
  remaining_vertical_distance_ -= translation_vertical;
  remaining_mouse_distance_ -= translation_mouse;

  travel_cam_->Translate(translation_horizontal, translation_ref_);
  travel_cam_->Translate({ 0, translation_vertical, 0 });

  const auto display_width = m_bml->GetRenderContext()->GetWidth();
  travel_cam_->Rotate({0, 1, 0}, translation_mouse.x / display_width);
  travel_cam_->Rotate({1, 0, 0}, translation_mouse.y / display_width, travel_cam_);
}

void AdvancedTravelCam::enter_travel_cam() {
  CKCamera* cam = get_ingame_cam();
  travel_cam_->SetWorldMatrix(cam->GetWorldMatrix());
  int width, height;
  cam->GetAspectRatio(width, height);
  travel_cam_->SetAspectRatio(width, height);
  travel_cam_->SetFov(cam->GetFov());
  m_bml->GetRenderContext()->AttachViewpointToCamera(travel_cam_);
  m_bml->GetGroupByName("HUD_sprites")->Show(CKHIDE);
  m_bml->GetGroupByName("LifeBalls")->Show(CKHIDE);

  is_in_travel_cam_ = true;
  m_bml->SendIngameMessage("Entered Advanced Travel Camera");
}

void AdvancedTravelCam::exit_travel_cam(bool local_state_only) {
  if (!local_state_only) {
    m_bml->GetRenderContext()->AttachViewpointToCamera(get_ingame_cam());
    m_bml->GetGroupByName("HUD_sprites")->Show();
    m_bml->GetGroupByName("LifeBalls")->Show();
  }

  is_in_travel_cam_ = false;
  m_bml->SendIngameMessage("Exited Advanced Travel Camera");
}

void AdvancedTravelCam::TravelCommand::Execute(IBML* bml, const std::vector<std::string>& args) {
  if (!mod->is_playing())
    return;
  if (args.size() >= 2) {
    if (args[1] == "true" || args[1] == "on")
      mod->enter_travel_cam();
    else
      mod->exit_travel_cam();
    return;
  }
  if (mod->is_in_travel_cam())
    mod->exit_travel_cam();
  else
    mod->enter_travel_cam();
}

const std::vector<std::string> AdvancedTravelCam::TravelCommand::GetTabCompletion(IBML* bml, const std::vector<std::string>& args) {
  if (args.size() == 2)
    return { "true", "false", "on", "off" };
  else
    return {};
}
