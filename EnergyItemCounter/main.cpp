#pragma once

#include <memory>
#include <vector>
#include "../bml_includes.hpp"
#include "../utils.hpp"


extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class EnergyItemCounter : public IMod {
  std::unique_ptr<BGui::Text> sprite;
  bool init = false;
  CK_ID ingame_parameter_array{};
  CK_ID life_group{}, point_group{};
  float y_pos = 0.9f;
  int font_size = 12;
  IProperty* prop_y{}, * prop_font_size{};
  utils utils;
  struct sector_data { int extra_lives = 0, extra_points = 0; };
  std::vector<sector_data> level_data;

public:
  EnergyItemCounter(IBML* bml) : IMod(bml), utils(bml) {}

  virtual iCKSTRING GetID() override { return "EnergyItemCounter"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "Energy Item Counter"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "Displays counts of remaining energy items (extra lives and points) in player's current sector."; }
  DECLARE_BML_VERSION;

  void OnPostStartMenu() override {
    if (init)
      return;

    ingame_parameter_array = CKOBJID(m_bml->GetArrayByName("IngameParameter"));
    show_sprite();
    init = true;
  }

  void OnStartLevel() override {
    if (!sprite)
      show_sprite();
    life_group = CKOBJID(m_bml->GetGroupByName("All_P_Extra_Life"));
    point_group = CKOBJID(m_bml->GetGroupByName("All_P_Extra_Point"));
    update_sprite();
  }

  void OnPreExitLevel() override {
    if (sprite)
      hide_sprite();
  }

  // build energy item data here
  virtual void OnLoadObject(CKSTRING filename, BOOL isMap, CKSTRING masterName,
      CK_CLASSID filterClass, BOOL addtoscene, BOOL reuseMeshes, BOOL reuseMaterials,
      BOOL dynamic, XObjectArray* objArray, CKObject* masterObj) {
    if (!isMap) return;
    std::vector<CKGroup*> sectors;
    for (int sector = 1; sector < 1000; sector++) {
      const auto sector_group = utils.get_sector_group(sector);
      if (sector_group) sectors.push_back(sector_group);
      else break;
    }
    level_data = decltype(level_data){{0}}; // sector 0 to make indexing easier
    level_data.resize(sectors.size() + 1);
    auto* group = m_bml->GetGroupByName("P_Extra_Life");
    if (group) {
      const auto length = group->GetObjectCount();
      for (int i = 0; i < length; i++) {
        auto* obj = group->GetObject(i);
        for (unsigned int sector = 0; sector < sectors.size(); sector++) {
          if (!obj->IsInGroup(sectors[sector])) continue;
          level_data[sector+1].extra_lives++;
          break;
        }
      }
    }
    group = m_bml->GetGroupByName("P_Extra_Point");
    if (group) {
      for (int i = 0; i < group->GetObjectCount(); i++) {
        auto* obj = group->GetObject(i);
        for (unsigned int sector = 0; sector < sectors.size(); sector++) {
          if (!obj->IsInGroup(sectors[sector])) continue;
          level_data[sector+1].extra_points++;
          break;
        }
      }
    }
  };

  void OnLoad() override {
    GetConfig()->SetCategoryComment("Main", "Main settings.");
    prop_y = GetConfig()->GetProperty("Main", "Y_Position");
    prop_y->SetComment("Y position of the top of sprite text.");
    prop_y->SetDefaultFloat(0.8f);
    prop_font_size = GetConfig()->GetProperty("Main", "Font_Size");
    prop_font_size->SetComment("Font size of sprite text.");
    prop_font_size->SetDefaultFloat(10);
    load_config();
  }

  void OnModifyConfig(iCKSTRING category, iCKSTRING key, IProperty* prop) override {
    load_config();
    if (sprite) {
      hide_sprite();
      show_sprite();
    }
  }

  void OnBallOff() override { update_sprite(); }
  void OnCamNavInactive() override { update_sprite(); }
  void OnCounterActive() override { update_sprite(); }
  void OnCounterInactive() override { update_sprite(); }
  void OnDead() override { update_sprite(); }
  void OnExtraPoint() override { update_sprite(); }
  void OnGameOver() override { update_sprite(); }
  void OnLevelFinish() override { update_sprite(); }
  void OnPreCheckpointReached() override { update_sprite(); }
  void OnPostLifeUp() override { update_sprite(); }
  void OnPreLifeUp() override { update_sprite(); }
  void OnPostSubLife() override { update_sprite(); }
  void OnPreSubLife() override { update_sprite(); }

  void OnCamNavActive() override { ensure_update(); }
  void OnPostCheckpointReached() override { ensure_update(); }

private:
  void show_sprite() {
    if (sprite)
      return;
    sprite = std::make_unique<decltype(sprite)::element_type>("EnergyItemCountDisplay");
    sprite->SetSize({ 0.6f, 0.12f });
    sprite->SetPosition({ 0.2f, y_pos });
    sprite->SetAlignment(CKSPRITETEXT_CENTER);
    sprite->SetTextColor(0xffffffff);
    sprite->SetZOrder(128);
    sprite->SetFont("Arial", font_size, 400, false, false);
  }

  void hide_sprite() { sprite.reset(); }

  void update_sprite() {
    if (!sprite)
      return;
    const auto sector = std::clamp(get_current_sector(), 0, (int)level_data.size() - 1);
    const auto energy = get_remaining_energy_item_count();
    const auto& data = level_data[sector];
    char text[128];
    snprintf(text, sizeof(text), "%d/%d extra %s remaining\n%d/%d extra %s remaining",
      energy.extra_lives, data.extra_lives, (data.extra_lives == 1) ? "life" : "lives",
      energy.extra_points, data.extra_points, (data.extra_points == 1) ? "point" : "points");
    sprite->SetText(text);
  }

  // just to make sure it's updating
  void ensure_update() {
    m_bml->AddTimer(CKDWORD(1), [this] { update_sprite(); });
    m_bml->AddTimer(CKDWORD(5), [this] { update_sprite(); });
    m_bml->AddTimer(CKDWORD(10), [this] { update_sprite(); });
  }

  int get_current_sector() {
    int sector = 0;
    static_cast<CKDataArray*>(m_bml->GetCKContext()->GetObject(ingame_parameter_array))->GetElementValue(0, 1, &sector);
    return sector;
  }

  sector_data get_remaining_energy_item_count() {
    int life_count = 0, point_count = 0;
    if (life_group) {
      auto* group = static_cast<CKGroup*>(m_bml->GetCKContext()->GetObject(life_group));
      int length = group->GetObjectCount();
      std::putchar('\n');
      for (int i = 0; i < length; i++) {
        auto* obj = group->GetObject(i);
        // P_Extra_Life_Sphere becomes hidden when collected
        // sometimes there are duplicates, or items from the last sector
        // isn't properly removed; we need to cross-reference the data we
        // built when the level is loaded
        if (
          obj->GetClassID() == CKCID_SPRITE3D
          && std::string_view{ obj->GetName() }.starts_with("P_Extra_Life_SilverBall")
          && obj->IsVisible()
        ) {
          life_count++;
        }
      }
    }
    if (point_group) {
      auto* group = static_cast<CKGroup*>(m_bml->GetCKContext()->GetObject(point_group));
      int length = group->GetObjectCount();
      for (int i = 0; i < length; i++) {
        auto* obj = group->GetObject(i);
        // P_Extra_Point_Ball0 (central ball) becomes hidden immediately after collection
        if (
          obj->GetClassID() == CKCID_SPRITE3D
          && std::string_view{ obj->GetName() }.starts_with("P_Extra_Point_Ball0")
          && obj->IsVisible()
        ) {
          point_count++;
        }
      }
    }
    return { life_count, point_count };
  }

  void load_config() {
    y_pos = prop_y->GetFloat();
    font_size = utils.get_bgui_font_size(prop_font_size->GetFloat());
  }
};

IMod* BMLEntry(IBML* bml) {
  return new EnergyItemCounter(bml);
}
