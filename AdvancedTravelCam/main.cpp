#include "main.h"

IMod* BMLEntry(IBML* bml) {
  return new AdvancedTravelCam(bml);
}

// Vx(*)Vector::Normalize: not supported on Virtools 2.1?
inline bool normalize(float& x, float& y) {
  const auto square_sum = x * x + y * y;
  if (square_sum == 0) return false;
  const auto factor = std::sqrt(1 / square_sum);
  x *= factor, y *= factor;
  return true;
};

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

  key_forward = prop_key_forward->GetKey();
  key_backward = prop_key_backward->GetKey();
  key_left = prop_key_left->GetKey();
  key_right = prop_key_right->GetKey();
  key_up = prop_key_up->GetKey();
  key_down = prop_key_down->GetKey();
  key_boost = prop_key_boost->GetKey();
  key_zoom = prop_key_zoom->GetKey();
}

std::pair<CK3dEntity*, CKPICKRESULT> AdvancedTravelCam::pick_screen() {
  auto ctx = m_bml->GetRenderContext();
  Vx2DVector cursor_pos; input_manager_->GetMousePosition(cursor_pos, false);

  // CKRenderContext::Pick is different on Virtools 2.1
  // so we have to do this to get the actual data
  CKPICKRESULT pick_result;
  CKRenderObject* dest_obj;
  if (!(dest_obj = ctx->Pick(int(cursor_pos.x), int(cursor_pos.y),
#ifndef USING_BML_PLUS
                             reinterpret_cast<VxIntersectionDesc*>(&pick_result)
#else
                             &pick_result
#endif
      )))
    return {};
  if (pick_result.Sprite)
    return {};

  return { static_cast<CK3dEntity*>(dest_obj), pick_result };
}

std::pair<float, float> AdvancedTravelCam::get_distance_factors() {
  // float sin_yaw = 0, cos_yaw = 1;
  if (!translation_ref_) {
    /*  VxQuaternion q;
        travel_cam_->GetQuaternion(&q);
        const float siny_cosp = 2 * (q.w * q.z + q.x * q.y), cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
        yaw = std::atan2(siny_cosp, cosy_cosp);  */
    VxVector orient;
    travel_cam_->GetOrientation(&orient, nullptr);
    // yaw = std::atan2(orient.x, orient.z);
    if (normalize(orient.x, orient.z))
      return { orient.x, orient.z };
  }
  return { 0.0f, 1.0f };
}

void AdvancedTravelCam::OnLoad() {
  travel_cam_ = static_cast<CKCamera*>(m_bml->GetCKContext()->CreateObject(CKCID_CAMERA, "AdvancedTravelCam"));
  travel_cam_->SetFrontPlane(0.25f);
  input_manager_ = m_bml->GetInputManager();

  auto config = GetConfig();
  config->SetCategoryComment("Quantities", "Related to movement/rotation sensitivities.");
  config->SetCategoryComment("Qualities", "Related to different behaviors.");
  config->SetCategoryComment("Controls", "Additional notes: use the left mouse button to pick a point (cancellable with the right button) on the screen and move the camera to it.");
  prop_horizontal_sensitivity_ = config->GetProperty("Quantities", "HorizontalSpeed");
  prop_horizontal_sensitivity_->SetDefaultFloat(0.05f);
  prop_vertical_sensitivity_ = config->GetProperty("Quantities", "VerticalSpeed");
  prop_vertical_sensitivity_->SetDefaultFloat(0.04f);
  prop_mouse_sensitivity_ = config->GetProperty("Quantities", "MouseSensitivity");
  prop_mouse_sensitivity_->SetDefaultFloat(3.0f);
  prop_maximum_view_distance_ = config->GetProperty("Quantities", "MaximumViewDistance");
  prop_maximum_view_distance_->SetDefaultFloat(4800.0f);
  prop_relative_direction_ = config->GetProperty("Qualities", "RelativeDirection");
  prop_relative_direction_->SetDefaultBoolean(false);
  prop_relative_direction_->SetComment(
    "Whether to move the camera relative to its current direction. "
    "If true, horizontal motion inputs will also affect the camera's vertical position."
  );
  prop_cinematic_camera_ = config->GetProperty("Qualities", "CinematicCamera");
  prop_cinematic_camera_->SetDefaultBoolean(true);
  prop_cinematic_camera_->SetComment("Whether to enable cinematic camera (for smooth movement).");
  prop_cinematic_motion_speed_ = config->GetProperty("Quantities", "CinematicMotionSpeed");
  prop_cinematic_motion_speed_->SetDefaultFloat(0.11f);
  prop_cinematic_motion_speed_->SetComment("Must be between 0 and 1.");
  prop_cinematic_mouse_speed_ = config->GetProperty("Quantities", "CinematicMouseSpeed");
  prop_cinematic_mouse_speed_->SetDefaultFloat(0.2f);
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

  prop_key_forward = config->GetProperty("Controls", "MoveForward");
  prop_key_forward->SetDefaultKey(CKKEY_W);
  prop_key_backward = config->GetProperty("Controls", "MoveBackward");
  prop_key_backward->SetDefaultKey(CKKEY_S);
  prop_key_left = config->GetProperty("Controls", "MoveLeft");
  prop_key_left->SetDefaultKey(CKKEY_A);
  prop_key_right = config->GetProperty("Controls", "MoveRight");
  prop_key_right->SetDefaultKey(CKKEY_D);
  prop_key_up = config->GetProperty("Controls", "Ascend");
  prop_key_up->SetDefaultKey(CKKEY_SPACE);
  prop_key_down = config->GetProperty("Controls", "Descend");
  prop_key_down->SetDefaultKey(CKKEY_LSHIFT);
  prop_key_boost = config->GetProperty("Controls", "SpeedBoost");
  prop_key_boost->SetDefaultKey(CKKEY_LCONTROL);
  prop_key_boost->SetComment("Gives your camera a movement speed boost.");
  prop_key_zoom = config->GetProperty("Controls", "Zoom");
  prop_key_zoom->SetDefaultKey(CKKEY_Z);
  prop_key_zoom->SetComment("Zooms in your camera (adjustable with the scroll wheel!).");

  load_config_values();

  m_bml->RegisterCommand(new TravelCommand(this));
}

void AdvancedTravelCam::OnModifyConfig(iCKSTRING category, iCKSTRING key, IProperty* prop) {
  load_config_values();
}

void AdvancedTravelCam::OnUnpauseLevel() {
  if (!is_in_travel_cam())
    return;
  if (GetCursor() == cursor_cross)
    m_bml->AddTimer(CKDWORD(2), [this] {
      input_manager_->ShowCursor(true);
    });
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

  const auto boost_factor = (input_manager_->IsKeyDown(key_boost) ? 3 : 1);

  auto [sin_yaw, cos_yaw] = get_distance_factors();

  const auto delta_time = m_bml->GetTimeManager()->GetLastDeltaTime(),
    delta_distance = horizontal_sensitivity_ * delta_time * boost_factor;
  if (input_manager_->IsKeyDown(key_forward))
    remaining_horizontal_distance_ += { delta_distance * sin_yaw, 0, delta_distance * cos_yaw };
  if (input_manager_->IsKeyDown(key_backward))
    remaining_horizontal_distance_ += { -delta_distance * sin_yaw, 0, -delta_distance * cos_yaw };
  if (input_manager_->IsKeyDown(key_left))
    remaining_horizontal_distance_ += { -delta_distance * cos_yaw, 0, delta_distance * sin_yaw };
  if (input_manager_->IsKeyDown(key_right))
    remaining_horizontal_distance_ += { delta_distance * cos_yaw, 0, -delta_distance * sin_yaw };

  if (input_manager_->IsKeyDown(key_up))
    remaining_vertical_distance_ += vertical_sensitivity_ * delta_time * boost_factor;
  if (input_manager_->IsKeyDown(key_down))
    remaining_vertical_distance_ += -vertical_sensitivity_ * delta_time * boost_factor;

  VxVector delta_mouse;
  input_manager_->GetMouseRelativePosition(delta_mouse);
  if (input_manager_->GetCursorVisibility()) delta_mouse = {};

  float current_fov = travel_cam_->GetFov();
  if (input_manager_->IsKeyPressed(key_zoom))
    desired_fov_ = default_fov_ / 5;
  if (input_manager_->IsKeyReleased(key_zoom))
    desired_fov_ = default_fov_;
  if (input_manager_->IsKeyDown(key_zoom) && delta_mouse.z != 0)
    desired_fov_ = std::clamp(delta_mouse.z > 0 ? desired_fov_ * 0.88f : desired_fov_ / 0.88f,
                              default_fov_ * 0.04f, default_fov_ * 1.2f);
  if (float fov_diff = std::abs(current_fov - desired_fov_) / desired_fov_; fov_diff != 0)
    travel_cam_->SetFov(fov_diff > 0.0005f
                        ? std::lerp(current_fov, desired_fov_, delta_time * 0.0125f)
                        : desired_fov_);

  delta_mouse *= (current_fov / default_fov_);

  remaining_mouse_distance_ += { -delta_mouse.x * mouse_sensitivity_, -delta_mouse.y * mouse_sensitivity_, 0 };

  if (input_manager_->IsMouseClicked(CK_MOUSEBUTTON_LEFT)) {
    if (input_manager_->GetCursorVisibility()) {
      auto picked = pick_screen();
      if (!picked.first)
        return;

      VxVector dest_pos, cam_pos, pos_diff;
      picked.first->Transform(&dest_pos, picked.second.IntersectionPoint);
      travel_cam_->GetPosition(&cam_pos);
      travel_cam_->LookAt(dest_pos);

      char msg[256];
      std::snprintf(msg, sizeof(msg), "\"%s\" (%.2f, %.2f, %.2f) selected",
                    picked.first->GetName(), dest_pos.x, dest_pos.y, dest_pos.z);
      m_bml->SendIngameMessage(msg);

      const auto distance_ratio = (picked.second.Distance - 60.0f) / picked.second.Distance;
      pos_diff = (dest_pos - cam_pos) * distance_ratio;
      const auto horizontal_distance = Vx2DVector(pos_diff.x, pos_diff.z).Magnitude() * (distance_ratio > 0 ? 1 : -1);

      std::tie(sin_yaw, cos_yaw) = get_distance_factors();
      remaining_horizontal_distance_ = { horizontal_distance * sin_yaw, 0, horizontal_distance * cos_yaw };
      remaining_vertical_distance_ = translation_ref_ ? 0 : pos_diff.y;
      remaining_mouse_distance_ = {};

      SetCursor(cursor_arrow);
    }
    else {
      SetCursor(cursor_cross);
    }
    input_manager_->ShowCursor(!input_manager_->GetCursorVisibility());
  }
  if (input_manager_->IsMouseClicked(CK_MOUSEBUTTON_RIGHT) && input_manager_->GetCursorVisibility()) {
    input_manager_->ShowCursor(false);
    SetCursor(cursor_arrow);
  }

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

  VxVector orient_dir, orient_up;
  travel_cam_->GetOrientation(&orient_dir, &orient_up);
  constexpr auto min_angle = 1.0f / 4096;
  auto max_angle = std::abs(PI / 2 - std::acos(orient_up.y));
  if (max_angle <= min_angle) max_angle = 0;
  else max_angle -= min_angle;
  const auto display_width = m_bml->GetRenderContext()->GetWidth();
  const auto translation_mouse_y = translation_mouse.y / display_width;
  travel_cam_->Rotate({0, 1, 0}, translation_mouse.x / display_width);
  travel_cam_->Rotate({1, 0, 0},
                      (orient_dir.y > 0)
                      ? (std::min)(translation_mouse_y, max_angle) : (std::max)(translation_mouse_y, -max_angle),
                      travel_cam_);
}

void AdvancedTravelCam::enter_travel_cam() {
  CKCamera* cam = get_ingame_cam();
  travel_cam_->SetWorldMatrix(cam->GetWorldMatrix());
  int width, height;
  cam->GetAspectRatio(width, height);
  travel_cam_->SetAspectRatio(width, height);
  default_fov_ = desired_fov_ = cam->GetFov();
  travel_cam_->SetFov(default_fov_);
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

  remaining_horizontal_distance_ = {};
  remaining_vertical_distance_ = {};
  remaining_mouse_distance_ = {};

  is_in_travel_cam_ = false;
  m_bml->SendIngameMessage("Exited Advanced Travel Camera");
}

void AdvancedTravelCam::zoom_to_object(const char* name) {
  auto obj = m_bml->Get3dObjectByName(name);
  if (!obj) {
    m_bml->SendIngameMessage(std::string{"Error: \""}.append(name).append("\" not found.").c_str());
    return;
  }

  VxVector orient;
  travel_cam_->GetOrientation(&orient, nullptr);
  VxBbox bbox = obj->GetBoundingBox();
  VxVector cam_dest = bbox.GetCenter();
  cam_dest -= orient / orient.Magnitude() * (obj->GetRadius() / std::tan(travel_cam_->GetFov() / 2));
  travel_cam_->SetPosition(cam_dest);

  char msg[256];
  std::snprintf(msg, sizeof(msg), "Zoomed to \"%s\" (%.2f, %.2f, %.2f).",
                obj->GetName(), cam_dest.x, cam_dest.y, cam_dest.z);
}

void AdvancedTravelCam::TravelCommand::Execute(IBML* bml, const std::vector<std::string>& args) {
  if (!mod_->is_playing())
    return;
  if (args.size() >= 2) {
    if (args[1] == "true" || args[1] == "on")
      mod_->enter_travel_cam();
    else if (args[1] == "false" || args[1] == "off")
      mod_->exit_travel_cam();
    else {
      std::string obj_name;
      for (auto iter = args.cbegin() + 1; iter != args.cend(); ++iter)
        obj_name.append(*iter) += ' ';
      obj_name.erase(obj_name.length() - 1);
      mod_->zoom_to_object(obj_name.c_str());
    }
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
