#pragma once

#include "../bml_includes.hpp"

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class Unhider : public IMod {
  bool init = false;
  IProperty* prop_enabled{}, * prop_shadows{}, * prop_everything{},
    * prop_depth_cube_color{}, * prop_depth_cube_opacity{},
    * prop_other_color{}, * prop_other_opacity{};
  CKMaterial* depth_cube_material{}, * other_material{};
  CKDWORD depth_cube_color{}, other_color{};
  XObjectArray* level_object_array{};

  void load_config() {
    depth_cube_color = parse_prop_color(prop_depth_cube_color, 0x407A41);
    other_color = parse_prop_color(prop_other_color, 0x345289);

    VxColor depth_cube_color_struct(depth_cube_color), other_color_struct(other_color);
    depth_cube_material->SetEmissive(depth_cube_color_struct);
    other_material->SetEmissive(other_color_struct);
    VxColor diffuse = depth_cube_material->GetDiffuse();
    diffuse.a = prop_depth_cube_opacity->GetFloat();
    depth_cube_material->SetDiffuse(diffuse);
    diffuse.a = prop_other_opacity->GetFloat();
    other_material->SetDiffuse(diffuse);
  }

  CK3dEntity* CK3dEntity_cast(CKObject* obj) {
    if (obj->GetClassID() == CKCID_3DENTITY || obj->GetClassID() == CKCID_3DOBJECT)
      return static_cast<CK3dEntity*>(obj);
    return nullptr;
  }

  // color: fallback color
  CKDWORD parse_prop_color(IProperty* prop, CKDWORD color) {
    char color_text[8]{};
    try {
      color = (CKDWORD)std::stoul(prop->GetString(), nullptr, 16);
    }
    catch (const std::exception& e) {
      GetLogger()->Warn("Error parsing the color code: %s. Resetting to %06X.", e.what(), color & 0x00FFFFFF);
    }
    snprintf(color_text, sizeof(color_text), "%06X", color & 0x00FFFFFF);
    prop->SetString(color_text);
    return color | 0xFF000000;
  }

  CKMaterial* create_transparent_material(CKSTRING name) {
    auto* mat = static_cast<CKMaterial*>(m_bml->GetCKContext()->CreateObject(CKCID_MATERIAL, name));
    mat->EnableAlphaBlend();
    mat->SetSourceBlend(VXBLEND_SRCALPHA);
    mat->SetDestBlend(VXBLEND_INVSRCALPHA);
    return mat;
  }

  inline void apply_material(CK3dEntity* entity, CKMaterial* mat) {
    auto mesh_length = entity->GetMeshCount();
    for (int j = 0; j < mesh_length; j++) {
      entity->GetMesh(j)->ApplyGlobalMaterial(mat);
    }
  }

  void apply_unhiding() {
    if (m_bml->GetGroupByName("Shadow") == NULL)
      m_bml->GetCKContext()->CreateObject(CKCID_GROUP, "Shadow");
    auto* shadows = m_bml->GetGroupByName("Shadow");
    auto* depth_cubes = m_bml->GetGroupByName("DepthTestCubes");
    const auto apply_shadow = [&, this](CK3dEntity* entity) {
      if (prop_shadows->GetBoolean()) {
        entity->ModifyMoveableFlags(8, 0);
        shadows->AddObject(entity);
      }
    };

    int length = depth_cubes->GetObjectCount();
    for (int i = 0; i < length; i++) {
      auto* depth_cube = CK3dEntity_cast(depth_cubes->GetObject(i));
      if (!depth_cube) continue;
      apply_material(depth_cube, depth_cube_material);
      apply_shadow(depth_cube);
      depth_cube->Show();
    }
    GetLogger()->Info("Applied material to %d DepthTestCube(s).", length);

    if (!level_object_array || !prop_everything->GetBoolean())
      return;
    length = level_object_array->Size();
    auto* ctx = m_bml->GetCKContext();
    for (int i = 0; i < length; i++) {
      auto* entity = CK3dEntity_cast(ctx->GetObject(level_object_array[0][i]));
      if (!entity || entity->IsInGroup(depth_cubes) || entity->IsVisible() || _stricmp(entity->GetName(), "SkyLayer") == 0)
        continue;
      apply_material(entity, other_material);
      apply_shadow(entity);
      entity->Show();
    }
    GetLogger()->Info("Applied material to %d additional hidden object(s).", length);
  }

public:
  Unhider(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "Unhider"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "Unhider"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "Unhides hidden objects in your game."; }
  DECLARE_BML_VERSION;

  void OnLoad() override {
    auto config = GetConfig();
    prop_enabled = config->GetProperty("Main", "Enabled");
    prop_enabled->SetComment("Whether to enable this mod. Requires a game restart.");
    prop_enabled->SetDefaultBoolean(true);
    prop_shadows = config->GetProperty("Main", "AddShadows");
    prop_shadows->SetComment("Whether to add shadows to hidden objects.");
    prop_shadows->SetDefaultBoolean(true);
    prop_everything = config->GetProperty("Main", "UnhideEverything");
    prop_everything->SetComment("Unhide everything, not only depth test cubes.");
    prop_everything->SetDefaultBoolean(false);
    prop_depth_cube_color = config->GetProperty("Style", "DepthCubeColor");
    prop_depth_cube_color->SetComment("Color for depth test cubes, in HEX format.");
    prop_depth_cube_color->SetDefaultString("Placeholder");
    prop_depth_cube_opacity = config->GetProperty("Style", "DepthCubeOpacity");
    prop_depth_cube_opacity->SetComment("Opacity for depth test cubes. 0 ~ 1.");
    prop_depth_cube_opacity->SetDefaultFloat(0.25f);
    prop_other_color = config->GetProperty("Style", "OtherColor");
    prop_other_color->SetComment("Color for unhided objects other than depth test cubes, in HEX format.");
    prop_other_color->SetDefaultString("Placeholder");
    prop_other_opacity = config->GetProperty("Style", "OtherOpacity");
    prop_other_opacity->SetComment("Opacity for unhided objects other than depth test cubes. 0 ~ 1.");
    prop_other_opacity->SetDefaultFloat(0.55f);
  }

  void OnLoadScript(iCKSTRING filename, CKBehavior* script) override {
    if (!prop_enabled->GetBoolean() || !prop_shadows->GetBoolean()) return;
    if (std::strcmp(script->GetName(), "Ball_Shadow") != 0) return;
    auto shadow_height = ScriptHelper::FindFirstBB(script, "TT Simple Shadow")->GetInputParameter(2);
    float height = 120.0f;
    shadow_height->GetDirectSource()->SetValue(&height, sizeof(height));
  }

  void OnPostStartMenu() override {
    if (init) return;

    GetLogger()->Info("Creating materials...");
    depth_cube_material = create_transparent_material("Unhider_Depth_Cube_Material");
    other_material = create_transparent_material("Unhider_Other_Material");
    depth_cube_material->SetTwoSided(true);
    other_material->SetTwoSided(true);
    load_config();

    init = true;
  }

  virtual void OnLoadObject(iCKSTRING filename, CKBOOL isMap, iCKSTRING masterName,
                            CK_CLASSID filterClass, CKBOOL addtoscene, CKBOOL reuseMeshes, CKBOOL reuseMaterials,
                            CKBOOL dynamic, XObjectArray* objArray, CKObject* masterObj) override {
    level_object_array = objArray;
    if (!isMap || !prop_enabled->GetBoolean()) return;
    apply_unhiding();
  }

  void OnModifyConfig(CKSTRING category, CKSTRING key, IProperty* prop) {
    load_config();
    /*if (prop_enabled->GetBoolean() && m_bml->IsIngame())
      apply_unhiding();*/
  }
};

IMod* BMLEntry(IBML* bml) {
  return new Unhider(bml);
}
