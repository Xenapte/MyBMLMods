#pragma once

#include <numeric>
#include "../bml_includes.hpp"
#ifdef USING_BML_PLUS
#define BML_BUILD_VER BML_PATCH_VER
#endif // USING_BML_PLUS

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class StickyMaps : public IMod {
private:
  bool enabled = true, init = false;
  IProperty* prop_enabled{};

  bool is_sticky_ball_mod_installed() {
    auto count = m_bml->GetModCount();
    for (int i = 0; i < count; ++i) {
      auto mod = m_bml->GetMod(i);
      if (std::strcmp(mod->GetID(), "BallSticky") == 0)
        return true;
    }
    return false;
  }

  CKTexture* CKTexture_cast(CKObject* obj) {
    if (obj->GetClassID() == CKCID_TEXTURE)
      return static_cast<CKTexture*>(obj);
    return nullptr;
  }
  
  void str_replace_all(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
  }

public:
  StickyMaps(IBML* bml) : IMod(bml) {
    enabled = is_sticky_ball_mod_installed();
    if (!enabled)
      GetLogger()->Warn("Sticky Ball mod not detected, StickyMaps will be disabled!");
  }

  virtual iCKSTRING GetID() override { return "StickyMaps"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "Sticky Maps"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override {
    return "A mod that replaces all stone and wood trafos to sticky trafos.";
  }
  // returning a version higher than the current BML version will cause the mod to be disabled
  virtual BMLVersion GetBMLVersion() override {
    // normally BML loads mods alphabetically, so this mod should be loaded after BallSticky
    // so we can safely disable it here if BallSticky is not installed
    if (!enabled)
      return { (std::numeric_limits<int>::max)(), 0, 0 };
    return { BML_MAJOR_VER, BML_MINOR_VER, BML_BUILD_VER };
  };

  virtual void OnLoad() override {
    prop_enabled = GetConfig()->GetProperty("Main", "Enabled");
    prop_enabled->SetComment("Requires a full restart.");
    prop_enabled->SetDefaultBoolean(true);
  }

  virtual void OnPostStartMenu() override {
    if (init || !prop_enabled->GetBoolean())
      return;
    
    auto* all_level = m_bml->GetArrayByName("AllLevel");
    assert(all_level);
    const int row_count = all_level->GetRowCount();
    static constexpr const int STARTBALL_COL = 1;
    for (int i = 0; i < row_count; ++i) {
      all_level->SetElementStringValue(i, STARTBALL_COL, "Ball_Sticky");
    }
  }

  virtual void OnLoadObject(iCKSTRING filename, CKBOOL isMap, iCKSTRING masterName,
      CK_CLASSID filterClass, CKBOOL addtoscene, CKBOOL reuseMeshes, CKBOOL reuseMaterials,
      CKBOOL dynamic, XObjectArray* objArray, CKObject* masterObj) override {
    if (!isMap || !prop_enabled->GetBoolean()) return;

    CKGroup* trafo_sticky_group = m_bml->GetGroupByName("Trafo_Sticky");
    if (!trafo_sticky_group)
      trafo_sticky_group = static_cast<CKGroup*>(m_bml->GetCKContext()->CreateObject(CKCID_GROUP, "P_Trafo_Sticky"));
    assert(trafo_sticky_group);
    for (const auto* group_name : { "P_Trafo_Stone", "P_Trafo_Wood" }) {
      auto* group = m_bml->GetGroupByName(group_name);
      if (!group) continue;
      const int obj_count = group->GetObjectCount();
      for (int i = 0; i < obj_count; ++i)
        trafo_sticky_group->AddObject(group->GetObject(i));
      m_bml->GetCKContext()->DestroyObject(group);
    }

    const int obj_count = objArray->Size();
    auto* ctx = m_bml->GetCKContext();
    for (int i = 0; i < obj_count; ++i) {
      auto* obj = CKTexture_cast(ctx->GetObject(objArray[0][i]));
      if (!obj) continue;
      std::string texture = obj->GetSlotFileName(obj->GetCurrentSlot());
      str_replace_all(texture, "Ball_Wood.bmp", "Column_beige.bmp"); // ballsticky texture
      str_replace_all(texture, "Ball_Stone.bmp", "Column_beige.bmp");
      obj->SetSlotFileName(obj->GetCurrentSlot(), texture.data());
    }
  }
};

IMod* BMLEntry(IBML* bml) {
  return new StickyMaps(bml);
}
