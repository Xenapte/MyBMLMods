#include "main.h"

IMod* BMLEntry(IBML* bml) {
  return new ReducedGravity(bml);
}

void ReducedGravity::set_physics() {
#ifdef ALLOW_CUSTOM_GRAVITY
  VxVector gravity = default_gravity * gravity_factor;
#endif
  for (const auto& bb: physics_bb) {
    auto* gravity_param = bb->GetInputParameter(0)->GetDirectSource();
    gravity_param->SetValue(&gravity, sizeof(gravity));
    if (bb->GetInputParameterCount() > 1) {
      auto* time_param = bb->GetInputParameter(1)->GetDirectSource();
      time_param->SetValue(&time_factor, sizeof(time_factor));
    }
    bb->ActivateInput(0);
    bb->Execute(0);
  }
}

// make elevators functional by adjusting the upward force
void ReducedGravity::set_modul03_spring_force() {
  if (!modul03_script)
    return;
  auto* spring_bb = ScriptHelper::FindFirstBB(modul03_script, "Set Physics Spring");
  int count = spring_bb->GetInputParameterCount();
  for (int i = 0; i < count; i++) {
    auto* param = spring_bb->GetInputParameter(i)->GetDirectSource();
    if (std::strcmp(param->GetName(), "Constant") != 0) continue;
    float force;
    param->GetValue(&force);
    force *= gravity_factor;
    param->SetValue(&force, sizeof(force));
  }
}

inline void ReducedGravity::update_status_text() {
  if (!status)
    return;
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "Reduced Gravity Enabled | %g%%", gravity_factor * 100);
  status->SetText(buffer);
}

void ReducedGravity::OnLoad() {
  status = std::make_unique<BGui::Label>("ReducedGravityStatus");
  status->SetAlignment(ALIGN_BOTTOM);
  status->SetPosition({ 0.5f, 0.998f });
  update_status_text();
  status->SetFont(ExecuteBB::GAMEFONT_03);
  status->SetZOrder(24);
  status->SetVisible(true);

#ifdef ALLOW_CUSTOM_GRAVITY
  m_bml->RegisterCommand(new GravityCommand(this));
#endif
}

void ReducedGravity::OnPostLoadLevel() {
  if (init || disabled)
    return;

  CKBehavior* script;

  script = m_bml->GetScriptByName("Gameplay_Ingame");
  physics_bb.push_back(ScriptHelper::FindFirstBB(ScriptHelper::FindFirstBB(script,
    "Init Ingame"), "Set Physics Globals"));

  script = m_bml->GetScriptByName("Gameplay_Refresh");
  physics_bb.push_back(ScriptHelper::FindFirstBB(ScriptHelper::FindFirstBB(script,
    "Tutorial freeze/unfreeze"), "Set Physics Globals"));

  script = m_bml->GetScriptByName("Gameplay_Tutorial");
  auto* kapitel_aktion = ScriptHelper::FindFirstBB(script, "Kapitel Aktion");
  physics_bb.push_back(ScriptHelper::FindFirstBB(ScriptHelper::FindFirstBB(kapitel_aktion,
    "Tut continue/exit"), "Set Physics Globals"));
  physics_bb.push_back(ScriptHelper::FindFirstBB(ScriptHelper::FindFirstBB(kapitel_aktion,
    "MoveKeys"), "Set Physics Globals"));

  script = m_bml->GetScriptByName("Event_handler");
  physics_bb.push_back(ScriptHelper::FindFirstBB(
    ScriptHelper::FindFirstBB(script, "Unpause Level"), "Set Physics Globals"));
  physics_bb.push_back(ScriptHelper::FindFirstBB(ScriptHelper::FindFirstBB(script,
    "reset Level"), "Set Physics Globals"));

  set_physics();

  init = true;
}

void ReducedGravity::OnStartLevel() {
  if (disabled) return;
  set_physics();
}

void ReducedGravity::OnCamNavActive() {
  if (disabled) return;
  set_physics();
  m_bml->AddTimer(CKDWORD(1), [this]() { set_physics(); });
}

void ReducedGravity::OnBallNavActive() {
  if (disabled) return;
  set_physics();
  m_bml->AddTimer(CKDWORD(1), [this]() { set_physics(); });
}

void ReducedGravity::OnProcess() {
  if (disabled) return;
  status->Process();
}

void ReducedGravity::OnLoadScript(iCKSTRING filename, CKBehavior* script) {
  if (disabled)
    return;
  const auto script_name = script->GetName();
  if (std::strcmp(script_name, "P_Modul03_MF Script") == 0) {
    modul03_script = script;
    set_modul03_spring_force();
  }
}

void ReducedGravity::OnPostStartMenu() {
  if (init || disabled)
    return;
  const int count = m_bml->GetModCount();
  for (int i = 0; i < count; i++) {
    auto* mod = m_bml->GetMod(i);
    if (std::strcmp(mod->GetID(), "PhysicsEditor") == 0) {
      disabled = true;
      m_bml->SendIngameMessage("[ReducedGravity] Physics Editor Mod detected, disabling to prevent conflicts...");
      break;
    }
  }
}
