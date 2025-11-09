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

void ReducedGravity::OnLoadObject(iCKSTRING filename, CKBOOL isMap, iCKSTRING masterName,
    CK_CLASSID filterClass, CKBOOL addtoscene, CKBOOL reuseMeshes, CKBOOL reuseMaterials,
    CKBOOL dynamic, XObjectArray* objArray, CKObject* masterObj) {
  if (!isMap || disabled)
    return;
  auto* modul03_group = m_bml->GetGroupByName("P_Modul_03");
  if (!modul03_group)
    return;

  auto* ctx = m_bml->GetCKContext();
  auto get_or_create_group = [this, ctx](const char* name) -> CKGroup* {
    auto* group = m_bml->GetGroupByName(name);
    if (group)
      return group;
    group = static_cast<CKGroup*>(ctx->CreateObject(CKCID_GROUP, const_cast<char*>(name))); // stupid ckstring shenanigans
    if (group)
      return group;
    GetLogger()->Error("Failed to create or get %s group!", name);
    return nullptr;
  };

  auto* stoneball_group = get_or_create_group("P_Ball_Stone");
  auto* box_group = get_or_create_group("P_Box");
  if (!stoneball_group || !box_group)
    return;

  CKDependencies dep;
  dep.Resize(40); dep.Fill(0);
  dep.m_Flags = CK_DEPENDENCIES_CUSTOM;
  dep[CKCID_OBJECT] = CK_DEPENDENCIES_COPY_OBJECT_NAME | CK_DEPENDENCIES_COPY_OBJECT_UNIQUENAME;
  dep[CKCID_BEOBJECT] = CK_DEPENDENCIES_COPY_BEOBJECT_ATTRIBUTES | CK_DEPENDENCIES_COPY_BEOBJECT_GROUPS | CK_DEPENDENCIES_COPY_BEOBJECT_SCRIPTS;

  auto copy_and_set_props = [this, ctx, modul03_group, &dep](CKBeObject* obj, CKGroup* new_group, float y_shift) {
    auto weight = static_cast<CK3dObject*>(ctx->CopyObject(obj, &dep, "._Weight", CK_OBJECTCREATION_RENAME));
    VxVector pos;
    weight->GetPosition(&pos);
    pos.y += y_shift;
    weight->SetPosition(VT21_REF(pos));
    // remove from original group and add to box group
    modul03_group->RemoveObject(weight);
    new_group->AddObject(weight);
  };

  const int length = modul03_group->GetObjectCount();
  for (int i = 0; i < length; i++) {
    auto* obj = modul03_group->GetObject(i);
    copy_and_set_props(obj, box_group, -5.55f);
    copy_and_set_props(obj, stoneball_group, -1.0f);
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
