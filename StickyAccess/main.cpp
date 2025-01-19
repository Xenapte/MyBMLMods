#pragma once

#include <numeric>
#include "../bml_includes.hpp"
#ifdef USING_BML_PLUS
#define BML_BUILD_VER BML_PATCH_VER
#endif // USING_BML_PLUS

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class StickyAccess : public IMod {
private:
  bool enabled = true, init = false;
  IProperty* prop_key{};
  CKKEYBOARD key{};
  int ball_change_cd = 0;
  CK_ID current_level_array{};
  CKBehavior* set_new_ball{};
  CKParameter* current_trafo{};

  bool is_sticky_ball_mod_installed() {
    auto count = m_bml->GetModCount();
    for (int i = 0; i < count; ++i) {
      auto mod = m_bml->GetMod(i);
      if (std::strcmp(mod->GetID(), "BallSticky") == 0)
        return true;
    }
    return false;
  }

  void load_config() {
    key = prop_key->GetKey();
  }

public:
  StickyAccess(IBML* bml) : IMod(bml) {
    enabled = is_sticky_ball_mod_installed();
    if (!enabled)
      GetLogger()->Warn("Sticky Ball mod not detected, StickyAccess will be disabled!");
  }

  virtual iCKSTRING GetID() override { return "StickyAccess"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "Sticky Ball Access"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override {
    return "A mod that lets you switch to the Sticky Ball added by BML, under Cheat Mode.";
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
    prop_key = GetConfig()->GetProperty("Main", "StickyKey");
    prop_key->SetComment("Key to switch to the Sticky Ball.");
    prop_key->SetDefaultKey(CKKEY_SEMICOLON);
    load_config();
  }

  virtual void OnModifyConfig(CKSTRING category, CKSTRING key, IProperty* prop) {
    load_config();
  };

  virtual void OnPostStartMenu() override {
    if (init)
      return;
    current_level_array = CKOBJID(m_bml->GetArrayByName("CurrentLevel"));

    CKBehavior* trafo_manager = ScriptHelper::FindFirstBB(m_bml->GetScriptByName("Gameplay_Ingame"), "Trafo Manager");
    set_new_ball = ScriptHelper::FindFirstBB(trafo_manager, "set new Ball");
    current_trafo = ScriptHelper::FindFirstBB(set_new_ball, "Switch On Parameter")->GetInputParameter(0)->GetDirectSource();
  }

  virtual void OnProcess() override {
    if (!m_bml->IsPlaying() || !enabled)
      return;

    if (ball_change_cd == 0) {
      if (m_bml->IsCheatEnabled() && m_bml->GetInputManager()->IsKeyPressed(key)) {
        CKMessageManager* mm = m_bml->GetMessageManager();
        CKMessageType ball_deact = mm->AddMessageType("BallNav deactivate");

        mm->SendMessageSingle(ball_deact, m_bml->GetGroupByName("All_Gameplay"));
        mm->SendMessageSingle(ball_deact, m_bml->GetGroupByName("All_Sound"));
        ball_change_cd = 2;

        m_bml->AddTimer(CKDWORD(2), [this]() {
          CK3dEntity* current_ball = static_cast<CK3dEntity*>(static_cast<CKDataArray*>(m_bml->GetCKContext()->GetObject(current_level_array))->GetElementObject(0, 1));
          ExecuteBB::Unphysicalize(current_ball);

          ScriptHelper::SetParamString(current_trafo, "sticky");
          set_new_ball->ActivateInput(0);
          set_new_ball->Activate();

          GetLogger()->Info("Set to Sticky Ball");
        });
      }
    }
    else if (ball_change_cd > 0)
      --ball_change_cd;
  }
};

IMod* BMLEntry(IBML* bml) {
  return new StickyAccess(bml);
}
