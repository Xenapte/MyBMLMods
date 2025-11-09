#include "main.h"

IMod* BMLEntry(IBML* bml) {
  return new ReducedGravity(bml);
}

void ReducedGravity::set_physics() {
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

void ReducedGravity::OnLoad() {
  status = std::make_unique<BGui::Label>("ReducedGravityStatus");
  status->SetAlignment(ALIGN_BOTTOM);
  status->SetPosition({ 0.5f, 0.998f });
  status->SetText("Reduced Gravity Enabled");
  status->SetFont(ExecuteBB::GAMEFONT_03);
  status->SetZOrder(24);
  status->SetVisible(true);
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
    // make elevators functional by adjusting the upward force
    auto* spring_bb = ScriptHelper::FindFirstBB(script, "Set Physics Spring");
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
