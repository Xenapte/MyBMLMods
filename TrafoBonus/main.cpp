#pragma once

#include <set>
#include <map>
#include "../bml_includes.hpp"

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class TrafoBonus : public IMod {
private:
  static constexpr int bonus_per_trafo = 1000;
  int bonus_counter_ = 0;
  std::map<int, std::set<CKSTRING>> trafo_record_; // sector -> set of ball - not trafo - IDs
  CKDataArray* ingame_parameter_array_{}, * current_level_array_{}, * energy_array_{};
  CK3dObject* player_ball_{};
  bool init_ = false, level_start_ = false;

  int get_current_sector() {
    int sector = 0;
    ingame_parameter_array_->GetElementValue(0, 1, &sector);
    return sector;
  }

  CK3dObject* get_current_ball() {
    return static_cast<CK3dObject*>(current_level_array_->GetElementObject(0, 1));
  }

  void add_extra_point(int points) {
    int current_score = 0;
    energy_array_->GetElementValue(0, 0, &current_score);
    current_score += points;
    energy_array_->SetElementValue(0, 0, &current_score);
  }

  void give_bonus() {
    bonus_counter_++;
    add_extra_point(bonus_per_trafo);
    GetLogger()->Info("Granted %d bonus points for transformation #%d in sector %d",
      bonus_per_trafo, bonus_counter_, get_current_sector());
  }

  void check_on_trafo() {
    auto* ball = get_current_ball();
    if (!player_ball_ || !ball)
      return;
    if (strcmp(ball->GetName(), player_ball_->GetName()) != 0) {
      if (trafo_record_[get_current_sector()].insert(ball->GetName()).second == true) {
        give_bonus();
      };
      // Update current player ball
      player_ball_ = ball;
    }
  }

public:
  TrafoBonus(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "TrafoBonus"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "Transformation Bonus"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override {
    return "Give you extra bonus points for each different ball transformation (in every sector).";
  }
  DECLARE_BML_VERSION;

  void OnPostStartMenu() override {
    if (init_) return;

    ingame_parameter_array_ = m_bml->GetArrayByName("IngameParameter");
    current_level_array_ = m_bml->GetArrayByName("CurrentLevel");
    energy_array_ = m_bml->GetArrayByName("Energy");

    init_ = true;
  };

  void OnStartLevel() override {
    trafo_record_.clear();
    player_ball_ = get_current_ball();
    bonus_counter_ = 0;
    level_start_ = true;
  }

  void OnLevelFinish() override {
    m_bml->AddTimer(200.0f, [this] {
      char buf[128];
      std::snprintf(buf, sizeof(buf),
        "You received a total of %d bonus points!", bonus_counter_ * bonus_per_trafo);
      m_bml->SendIngameMessage(buf);
      add_extra_point(-bonus_counter_ * bonus_per_trafo);
    });
  }

  void OnCamNavActive() override {
    if (!level_start_)
      return;
    level_start_ = false;
    player_ball_ = get_current_ball();
  }

  void OnBallNavInactive() override {
    if (!m_bml->IsIngame())
      return;
    check_on_trafo();
  }

  void OnBallNavActive() override {
    OnBallNavInactive();
  }
};

IMod* BMLEntry(IBML* bml) {
  return new TrafoBonus(bml);
}
