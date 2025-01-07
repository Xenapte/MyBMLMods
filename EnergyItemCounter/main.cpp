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
  CK_ID ingame_parameter_array{}, ph_array{};
  CK_ID life_group_id{}, point_group_id{};
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
    update_sprite();
  }

  void OnPreExitLevel() override {
    if (sprite)
      hide_sprite();
  }

  // build energy item data here
  void OnPostLoadLevel() override {
    auto life_group = m_bml->GetGroupByName("All_P_Extra_Life");
    auto point_group = m_bml->GetGroupByName("All_P_Extra_Point");
    life_group_id = CKOBJID(life_group);
    point_group_id = CKOBJID(point_group);
    auto* ph = m_bml->GetArrayByName("PH");
    ph_array = CKOBJID(ph);
    // no need to test for ph, it's always there if the game works
    int length = ph->GetRowCount();
    level_data = decltype(level_data){{0}}; // sector 0 to make indexing easier
    for (int row = 0; row < length; row++) {
      int sector;
      ph->GetElementValue(row, 0, &sector);
      if ((int) level_data.size() <= sector) level_data.resize(sector + 1);
      auto* obj = ph->GetElementObject(row, 3);
      if (obj->GetClassID() != CKCID_3DENTITY) continue;
      if (life_group && static_cast<CKBeObject*>(obj)->IsInGroup(life_group))
        level_data[sector].extra_lives++;
      else if (point_group && static_cast<CKBeObject*>(obj)->IsInGroup(point_group))
        level_data[sector].extra_points++;
    }
  }

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
  void OnGameOver() override { update_sprite(); }
  void OnLevelFinish() override { update_sprite(); }
  void OnPreCheckpointReached() override { update_sprite(); }
  void OnPostLifeUp() override { update_sprite(); }
  void OnPreLifeUp() override { update_sprite(); }
  void OnPostSubLife() override { update_sprite(); }
  void OnPreSubLife() override { update_sprite(); }

  void OnExtraPoint() override { delayed_update(); }
  void OnCamNavActive() override { delayed_update(); }
  void OnPostCheckpointReached() override { delayed_update(); }
  void OnPauseLevel() override { delayed_update(); }
  void OnUnpauseLevel() override { delayed_update(); }

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
    auto sector = get_current_sector();
    if (sector >= (int) level_data.size() || sector < 0) sector = 0;
    const auto energy = get_remaining_energy_item_count(sector);
    const auto& data = level_data[sector];
    char text[128];
    snprintf(text, sizeof(text), "%d/%d extra %s remaining\n%d/%d extra %s remaining",
      energy.extra_lives, data.extra_lives, (data.extra_lives == 1) ? "life" : "lives",
      energy.extra_points, data.extra_points, (data.extra_points == 1) ? "point" : "points");
    sprite->SetText(text);
  }

  void delayed_update() {
    m_bml->AddTimer(CKDWORD(1), [this] { update_sprite(); });
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

  sector_data get_remaining_energy_item_count(int sector) {
    int life_count = 0, point_count = 0;
    auto* life_group = life_group_id ? static_cast<CKGroup*>(m_bml->GetCKContext()->GetObject(life_group_id)) : nullptr;
    auto* point_group = point_group_id ? static_cast<CKGroup*>(m_bml->GetCKContext()->GetObject(point_group_id)) : nullptr;
    auto* ph = static_cast<CKDataArray*>(m_bml->GetCKContext()->GetObject(ph_array));
    int ph_length = ph->GetRowCount();
    for (int row = 0; row < ph_length; row++) {
      int obj_sector;
      ph->GetElementValue(row, 0, &obj_sector);
      if (obj_sector != sector) continue;
      auto* obj = ph->GetElementObject(row, 3);
      if (obj->GetClassID() != CKCID_3DENTITY) continue;
      auto obj3d = static_cast<CK3dEntity*>(obj);
      if (life_group && obj3d->IsInGroup(life_group)) {
        int script_length = obj3d->GetScriptCount();
        for (int i = 0; i < script_length; i++) {
          auto* script = obj3d->GetScript(i);
          if (std::strcmp(script->GetName(), "P_Extra_Life_MF Script") == 0 && script->IsActive()) {
            life_count++;
            break;
          }
        }
      }
      else if (point_group && static_cast<CKBeObject*>(obj)->IsInGroup(point_group)) {
        // can't check script activation here -
        // we want the extra point to not count if it's already activated
        // but the script remains active all the way until everything is collected
        // we have to check the "Set?" column in PH
        int set;
        ph->GetElementValue(row, 4, &set);
        if (set) point_count++;
      };
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
