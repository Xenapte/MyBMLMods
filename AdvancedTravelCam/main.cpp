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
  cinematic_motion_speed_ = prop_cinematic_motion_speed_->GetFloat() * 0.06f;
  cinematic_mouse_speed_ = prop_cinematic_mouse_speed_->GetFloat() * 0.06f;
  travel_cam_->SetBackPlane(prop_maximum_view_distance_->GetFloat());
}

void AdvancedTravelCam::clip_cursor() {
  const auto handle = static_cast<HWND>(m_bml->GetCKContext()->GetMainWindow());
  RECT client_rect;
  GetClientRect(handle, &client_rect);
  POINT top_left_corner = { client_rect.left, client_rect.top };
  ClientToScreen(handle, &top_left_corner);
  client_rect = { .left = top_left_corner.x, .top = top_left_corner.y,
    .right = client_rect.right + top_left_corner.x, .bottom = client_rect.bottom + top_left_corner.y };
  ClipCursor(&client_rect);
}

void AdvancedTravelCam::cancel_clip_cursor() {
  ClipCursor(NULL);
}

void AdvancedTravelCam::OnLoad() {
  travel_cam_ = static_cast<CKCamera*>(m_bml->GetCKContext()->CreateObject(CKCID_CAMERA, "AdvancedTravelCam"));
  input_manager_ = m_bml->GetInputManager();

  auto config = GetConfig();
  config->SetCategoryComment("Quantities", "Related to movement/rotation sensitivities.");
  config->SetCategoryComment("Qualities", "Related to different behaviors.");
  prop_horizontal_sensitivity_ = config->GetProperty("Quantities", "HorizontalSpeed");
  prop_horizontal_sensitivity_->SetDefaultFloat(0.05f);
  prop_vertical_sensitivity_ = config->GetProperty("Quantities", "VerticalSpeed");
  prop_vertical_sensitivity_->SetDefaultFloat(0.04f);
  prop_mouse_sensitivity_ = config->GetProperty("Quantities", "MouseSensitivity");
  prop_mouse_sensitivity_->SetDefaultFloat(3.2f);
  prop_maximum_view_distance_ = config->GetProperty("Quantities", "MaximumViewDistance");
  prop_maximum_view_distance_->SetDefaultFloat(4800.0f);
  prop_relative_direction_ = config->GetProperty("Qualities", "RelativeDirection");
  prop_relative_direction_->SetDefaultBoolean(true);
  prop_relative_direction_->SetComment(
    "Whether to move the camera relative to its current direction. "
    "If true, horizontal motion inputs will also affect the camera's vertical position."
  );
  prop_cinematic_camera_ = config->GetProperty("Qualities", "CinematicCamera");
  prop_cinematic_camera_->SetDefaultBoolean(true);
  prop_cinematic_camera_->SetComment("Whether to enable cinematic camera (for smooth movement).");
  prop_cinematic_motion_speed_ = config->GetProperty("Quantities", "CinematicMotionSpeed");
  prop_cinematic_motion_speed_->SetDefaultFloat(0.04f);
  prop_cinematic_motion_speed_->SetComment("Must be between 0 and 1.");
  prop_cinematic_mouse_speed_ = config->GetProperty("Quantities", "CinematicMouseSpeed");
  prop_cinematic_mouse_speed_->SetDefaultFloat(0.04f);
  prop_cinematic_mouse_speed_->SetComment("Must be between 0 and 1.");
  for (auto i = 0; i < sizeof(prop_commands_) / sizeof(prop_cinematic_camera_); ++i) {
    prop_commands_[i] = config->GetProperty("Qualities", ("Command" + std::to_string(i + 1)).c_str());
    prop_commands_[i]->SetComment(
      "Command for activating Advanced Travel Camera. Must be in lowercase. "
      "A restart is needed for the change to take effect."
    );
  }
  prop_commands_[0]->SetDefaultString("advancedtravel");
  prop_commands_[1]->SetDefaultString("+travel");

  load_config_values();

  m_bml->RegisterCommand(new TravelCommand(this));
}

void AdvancedTravelCam::OnModifyConfig(CKSTRING category, CKSTRING key, IProperty* prop) {
  load_config_values();
}

void AdvancedTravelCam::OnPauseLevel() {
  if (is_in_travel_cam())
    cancel_clip_cursor();
}

void AdvancedTravelCam::OnUnpauseLevel() {
  if (is_in_travel_cam())
    clip_cursor();
}

void AdvancedTravelCam::OnPreResetLevel() {
  if (is_in_travel_cam())
    exit_travel_cam(true);
}

void AdvancedTravelCam::OnPreEndLevel() {
  OnPauseLevel();
}

void AdvancedTravelCam::OnDead() {
  OnPauseLevel();
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

  const auto boost_factor = (input_manager_->IsKeyDown(CKKEY_LCONTROL) ? 3 : 1);

  float sin_yaw = 0, cos_yaw = 1;
  if (!translation_ref_) {
    /*  VxQuaternion q;
        travel_cam_->GetQuaternion(&q);
        const float siny_cosp = 2 * (q.w * q.z + q.x * q.y), cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
        yaw = std::atan2(siny_cosp, cosy_cosp);  */
    VxVector orient;
    travel_cam_->GetOrientation(&orient, NULL);
    // yaw = std::atan2(orient.x, orient.z);
    const auto square_sum = orient.x * orient.x + orient.z * orient.z;
    if (square_sum != 0) {
      const auto factor = std::sqrt(1 / square_sum);
      sin_yaw = orient.x * factor, cos_yaw = orient.z * factor;
    }
  }

  const auto delta_time = m_bml->GetTimeManager()->GetLastDeltaTime(),
    delta_distance = horizontal_sensitivity_ * delta_time * boost_factor;
  if (input_manager_->IsKeyDown(CKKEY_W))
    remaining_horizontal_distance_ += { delta_distance * sin_yaw, 0, delta_distance * cos_yaw };
  if (input_manager_->IsKeyDown(CKKEY_S))
    remaining_horizontal_distance_ += { -delta_distance * sin_yaw, 0, -delta_distance * cos_yaw };
  if (input_manager_->IsKeyDown(CKKEY_A))
    remaining_horizontal_distance_ += { -delta_distance * cos_yaw, 0, delta_distance * sin_yaw };
  if (input_manager_->IsKeyDown(CKKEY_D))
    remaining_horizontal_distance_ += { delta_distance * cos_yaw, 0, -delta_distance * sin_yaw };

  if (input_manager_->IsKeyDown(CKKEY_SPACE))
    remaining_vertical_distance_ += vertical_sensitivity_ * delta_time * boost_factor;
  if (input_manager_->IsKeyDown(CKKEY_LSHIFT))
    remaining_vertical_distance_ += -vertical_sensitivity_ * delta_time * boost_factor;

  VxVector delta_mouse;
  input_manager_->GetMouseRelativePosition(delta_mouse);
  remaining_mouse_distance_ += { -delta_mouse.x * mouse_sensitivity_, -delta_mouse.y * mouse_sensitivity_, 0 };

  // interpolation: we calculate our translation distances
  // and subtract them from our remaining distances
  VxVector translation_horizontal, translation_mouse;
  float translation_vertical{};
  if (cinematic_camera_) {
    translation_horizontal = Interpolate(cinematic_motion_speed_ * delta_time, {}, remaining_horizontal_distance_);
    translation_vertical = std::lerp(0.0f, remaining_vertical_distance_, cinematic_motion_speed_ * delta_time);
    translation_mouse = Interpolate(cinematic_mouse_speed_ * delta_time, {}, remaining_mouse_distance_);
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
  clip_cursor();

  is_in_travel_cam_ = true;
  m_bml->SendIngameMessage("Entered Advanced Travel Camera");
}

void AdvancedTravelCam::exit_travel_cam(bool local_state_only) {
  if (!local_state_only) {
    m_bml->GetRenderContext()->AttachViewpointToCamera(get_ingame_cam());
    m_bml->GetGroupByName("HUD_sprites")->Show();
    m_bml->GetGroupByName("LifeBalls")->Show();
  }

  remaining_horizontal_distance_ = {};
  remaining_vertical_distance_ = {};
  remaining_mouse_distance_ = {};
  cancel_clip_cursor();

  is_in_travel_cam_ = false;
  m_bml->SendIngameMessage("Exited Advanced Travel Camera");
}

void AdvancedTravelCam::TravelCommand::Execute(IBML* bml, const std::vector<std::string>& args) {
  if (!mod_->is_playing())
    return;
  if (args.size() >= 2) {
    if (args[1] == "true" || args[1] == "on")
      mod_->enter_travel_cam();
    else
      mod_->exit_travel_cam();
    return;
  }
  if (mod_->is_in_travel_cam())
    mod_->exit_travel_cam();
  else
    mod_->enter_travel_cam();
}

const std::vector<std::string> AdvancedTravelCam::TravelCommand::GetTabCompletion(IBML* bml, const std::vector<std::string>& args) {
  if (args.size() == 2)
    return { "true", "false", "on", "off" };
  else
    return {};
}
