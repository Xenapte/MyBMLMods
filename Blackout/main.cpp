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
  CKLight *light{}, *light_bottom{};
  IProperty *prop_enabled{}, *prop_light{}, *prop_full_dark{};
  inline static const std::unordered_set<std::string> exempt_list {
    "tower_floor_top_flat_illu", "laterne_verlauf", "laterne_schatten", "laterne_glas",
    "ball_lightningsphere", "animtrafo_flashfield", "pe_ufo_flash"
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
    mat->SetEmissive(get_darkened_color(mat->GetEmissive(), 0.125f));
  }

  void load_config() {
    light->Active(prop_light->GetBoolean());
    //light_bottom->Active(prop_light->GetBoolean());
  }

public:
  Blackout(IBML *bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "Blackout"; }
  virtual iCKSTRING GetVersion() override { return "0.0.3"; }
  virtual iCKSTRING GetName() override { return "Blackout"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "Darkens everything in your game."; }
  DECLARE_BML_VERSION;

  void OnLoadScript(iCKSTRING filename, CKBehavior *script) override {
    if (!prop_full_dark->GetBoolean() || !prop_enabled->GetBoolean()) return;
    if (std::strcmp(script->GetName(), "Gameplay_Sky") != 0) return;
    auto sky_color = ScriptHelper::FindFirstBB(script, "TT Sky")->GetInputParameter(1);
    VxColor color {80, 80, 80, 128};
    sky_color->GetDirectSource()->SetValue(&color, sizeof(color));
  }

  void OnLoad() override {
    auto config = GetConfig();
    prop_enabled = config->GetProperty("Main", "Enabled");
    prop_enabled->SetComment("Whether to enable this mod. Requires a game restart.");
    prop_enabled->SetDefaultBoolean(true);
    prop_light = config->GetProperty("Main", "LightUpSurroundings");
    prop_light->SetComment("Lights up the immediate surrounding environment by a faint light source slightly above the ball.");
    prop_light->SetDefaultBoolean(true);
    prop_full_dark = config->GetProperty("Main", "FullDarkMode");
    prop_full_dark->SetComment("Fully darkens everything. Requires a game restart.");
    prop_full_dark->SetDefaultBoolean(true);
  }

  void OnPostStartMenu() override {
    if (init || !prop_enabled->GetBoolean()) return;

    current_level_array = m_bml->GetArrayByName("CurrentLevel");
    all_level_array = m_bml->GetArrayByName("AllLevel");

    VxColor black {0, 0, 0, 255};
    int row_count = all_level_array->GetRowCount();
    for (int i = 0; i < row_count; i++) {
      all_level_array->SetElementValue(i, 4, &black, sizeof(black));
    }

    light = static_cast<decltype(light)>(m_bml->GetCKContext()->CreateObject(CKCID_LIGHT, "Blackout_Light"));
    light->SetType(VX_LIGHTPOINT);
    light->SetColor({ 224, 224, 224, 224 });
    light->SetRange(28.0f);
    light->SetConstantAttenuation(0);
    light->SetLinearAttenuation(0.022f);
    light->SetQuadraticAttenuation(0.44f);
    light->SetLightPower(1.41f);
    /*light_bottom = static_cast<decltype(light)>(m_bml->GetCKContext()->CreateObject(CKCID_LIGHT, "Blackout_Light_Bottom"));
    VxQuaternion rotation;
    rotation.FromEulerAngles(-90.0f, 0, 0);
    light_bottom->SetQuaternion(rotation);
    light_bottom->SetType(VX_LIGHTSPOT);
    light_bottom->SetColor({192, 192, 192, 64});
    light_bottom->SetRange(10.0f);
    light_bottom->SetConstantAttenuation(0);
    light_bottom->SetLinearAttenuation(0);
    light_bottom->SetQuadraticAttenuation(0.2f);
    light_bottom->SetHotSpot(60.0f);
    light_bottom->SetFallOff(60.0f);*/
    m_bml->GetCKContext()->GetCurrentScene()->AddObjectToScene(light);
    //m_bml->GetCKContext()->GetCurrentScene()->AddObjectToScene(light_bottom);
    load_config();

    init = true;
  }

  /*void OnStartLevel() override {
    m_bml->AddTimer(CKDWORD(10), [this] { });
  }*/

  void OnProcess() override {
    if (!(m_bml->IsIngame() && light && prop_enabled->GetBoolean() && prop_light->GetBoolean())) return;
    auto current_ball = get_current_ball();
    VxVector pos;
    current_ball->GetPosition(&pos);
    pos.y += 3.6f;
    light->SetPosition(pos);
    //pos.y -= 12.0f;
    //light_bottom->SetPosition(pos);
  }

  virtual void OnLoadObject(iCKSTRING filename, BOOL isMap, iCKSTRING masterName,
                            CK_CLASSID filterClass, BOOL addtoscene, BOOL reuseMeshes, BOOL reuseMaterials,
                            BOOL dynamic, XObjectArray* objArray, CKObject* masterObj) override {
    if (!(prop_full_dark && prop_full_dark->GetBoolean() && prop_enabled->GetBoolean())) return;
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
