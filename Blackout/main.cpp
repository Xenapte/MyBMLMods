#pragma once

#include <unordered_set>
#include "../bml_includes.hpp"
#include "../utils.hpp"

extern "C" {
  __declspec(dllexport) IMod *BMLEntry(IBML *bml);
}

class Blackout : public IMod {
  bool init = false;
  CKDataArray *current_level_array{}, *all_level_array{};
  CKLight *light{};
  IProperty *prop_light{}, *prop_full_dark{};
  inline static const std::unordered_set<std::string> exempt_list {
    "tower_floor_top_flat_illu", "laterne_verlauf", "laterne_schatten", "laterne_glas",
    "ball_lightningsphere", "animtrafo_flashfield"
  };

  CK3dObject *get_current_ball() {
    return static_cast<CK3dObject*>(current_level_array->GetElementObject(0, 1));
  }

  VxColor get_darkened_color(VxColor color, float max) {
    float max_v = 0;
    for (int i = 0; i < 3; i++) {
      if (color.col[i] < max_v)
        continue;
      max_v = color.col[i];
    }
    if (max_v <= max) return color;
    for (int i = 0; i < 3; i++) {
      color.col[i] = std::round(color.col[i] / max_v * max);
    }
    return color;
  }

  void darken_material(CKMaterial *mat) {
    if (exempt_list.contains(utils::to_lower(mat->GetName()))) return;
    mat->SetEmissive(get_darkened_color(mat->GetEmissive(), 0.15f));
  }

  void load_config() {
    light->Active(prop_light->GetBoolean());
  }

public:
  Blackout(IBML *bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "Blackout"; }
  virtual iCKSTRING GetVersion() override { return "0.0.1"; }
  virtual iCKSTRING GetName() override { return "Blackout"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "Darkens everything in your game."; }
  DECLARE_BML_VERSION;

  void OnLoadScript(iCKSTRING filename, CKBehavior *script) override {
    if (!prop_full_dark->GetBoolean()) return;
    if (std::strcmp(script->GetName(), "Gameplay_Sky") != 0) return;
    auto sky_color = ScriptHelper::FindFirstBB(script, "TT Sky")->GetInputParameter(1);
    VxColor color {80, 80, 80, 128};
    sky_color->GetDirectSource()->SetValue(&color, sizeof(color));
  }

  void OnLoad() override {
    auto config = GetConfig();
    prop_light = config->GetProperty("Main", "LightUpSurroundings");
    prop_light->SetComment("Lights up the immediate surrounding environment by a faint light source slightly above the ball.");
    prop_light->SetDefaultBoolean(true);
    prop_full_dark = config->GetProperty("Main", "FullDarkMode");
    prop_full_dark->SetComment("Fully darkens everything. Requires a game restart.");
    prop_full_dark->SetDefaultBoolean(true);
  }

  void OnPostStartMenu() override {
    if (init) return;

    current_level_array = m_bml->GetArrayByName("CurrentLevel");
    all_level_array = m_bml->GetArrayByName("AllLevel");

    VxColor black {0, 0, 0, 255};
    int row_count = all_level_array->GetRowCount();
    for (int i = 0; i < row_count; i++) {
      all_level_array->SetElementValue(i, 4, &black, sizeof(black));
    }

    light = static_cast<decltype(light)>(m_bml->GetCKContext()->CreateObject(CKCID_LIGHT, "Blackout_Light"));
    light->SetType(VX_LIGHTPOINT);
    light->SetColor({ 192, 192, 192, 160 });
    light->SetRange(20.0f);
    light->SetConstantAttenuation(0);
    light->SetLinearAttenuation(0.022f);
    light->SetQuadraticAttenuation(0.55f);
    /*VxQuaternion rotation;
    rotation.FromEulerAngles(90.0f, 0, 0);
    light->SetQuaternion(rotation);
    light->SetType(VX_LIGHTSPOT);
    light->SetColor({192, 192, 192, 64});
    light->SetRange(100.0f);
    light->SetConstantAttenuation(0);
    light->SetLinearAttenuation(0);
    light->SetQuadraticAttenuation(0.2f);
    light->SetHotSpot(10.0f);
    light->SetFallOff(15.0f);*/
    m_bml->GetCKContext()->GetCurrentScene()->AddObjectToScene(light);
    load_config();

    init = true;
  }

  /*void OnStartLevel() override {
    m_bml->AddTimer(CKDWORD(10), [this] { });
  }*/

  void OnProcess() override {
    if (!m_bml->IsIngame() || !light) return;
    auto current_ball = get_current_ball();
    VxVector pos;
    current_ball->GetPosition(&pos);
    pos.y += 3.6f;
    light->SetPosition(pos);
  }

  virtual void OnLoadObject(iCKSTRING filename, BOOL isMap, iCKSTRING masterName,
                            CK_CLASSID filterClass, BOOL addtoscene, BOOL reuseMeshes, BOOL reuseMaterials,
                            BOOL dynamic, XObjectArray* objArray, CKObject* masterObj) override {
    if (!prop_full_dark || !prop_full_dark->GetBoolean()) return;
    auto& list = m_bml->GetCKContext()->GetObjectListByType(CKCID_MATERIAL, false);
    for (int i = 0; i < list.Size(); i++) {
      darken_material(static_cast<CKMaterial*>(list[i]));
    }
  }

  void OnModifyConfig(CKSTRING category, CKSTRING key, IProperty* prop) {
    load_config();
  }
};

IMod *BMLEntry(IBML *bml) {
  return new Blackout(bml);
}
