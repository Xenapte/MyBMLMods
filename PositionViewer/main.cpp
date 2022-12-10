#pragma once

#include <BML/BMLAll.h>
#include <thread>
#include <memory>


extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class PositionViewer : public IMod {
  std::unique_ptr<BGui::Text> sprite;
  bool init = false;
  std::thread pos_thread;
  CKDataArray *current_level_array{};

public:
  PositionViewer(IBML* bml) : IMod(bml) {}

  virtual CKSTRING GetID() override { return "PositionViewer"; }
  virtual CKSTRING GetVersion() override { return "0.0.1"; }
  virtual CKSTRING GetName() override { return "Position Viewer"; }
  virtual CKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual CKSTRING GetDescription() override { return "Displays the coordinates of your player ball."; }
  DECLARE_BML_VERSION;

  void OnProcess() override {
    if (!(m_bml->IsPlaying() && sprite)) 
      return;

    auto* player_ball = static_cast<CK3dObject*>(current_level_array->GetElementObject(0, 1));
    VxVector pos; player_ball->GetPosition(&pos);
    char pos_text[128];
    snprintf(pos_text, sizeof(pos_text), "X: %.3f, Y: %.3f, Z: %.3f", pos.x, pos.y, pos.z);
    sprite->SetText(pos_text);
  }

  void OnPostStartMenu() override {
    if (init)
      return;

    current_level_array = m_bml->GetArrayByName("CurrentLevel");
    show_ball_pos();
    init = true;
  }

  void OnStartLevel() {
    if (!sprite)
      show_ball_pos();
  }

  void OnPreExitLevel() {
    if (sprite)
      hide_ball_pos();
  }

private:
  void show_ball_pos() {
    if (sprite)
      return;
    sprite = std::make_unique<decltype(sprite)::element_type>("PositionDisplay");
    sprite->SetSize({ 0.6f, 0.05f });
    sprite->SetPosition({ 0.2f, 0.9f });
    sprite->SetAlignment(CKSPRITETEXT_CENTER);
    sprite->SetTextColor(0xffffffff);
    sprite->SetZOrder(128);
    sprite->SetFont("Arial", 12, 400, false, false);
  }

  void hide_ball_pos() { sprite.reset(); }
};

IMod* BMLEntry(IBML* bml) {
  return new PositionViewer(bml);
}
