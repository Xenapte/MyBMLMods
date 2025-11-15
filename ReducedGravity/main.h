#pragma once

#include "../bml_includes.hpp"
#include <memory>

#define FIXED_GRAVITY_RATE 0.525
#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class ReducedGravity : public IMod {
public:
  ReducedGravity(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "ReducedGravity"; }
  virtual iCKSTRING GetVersion() override {
    return "0.2.1"
#ifdef ALLOW_CUSTOM_GRAVITY
      "-adjustable";
#else
      "-fixed-" STRINGIFY(FIXED_GRAVITY_RATE);
#endif
  }
  virtual iCKSTRING GetName() override { return "Reduced Gravity Mode"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "Play with a reduced gravity."; }
  DECLARE_BML_VERSION;

  virtual void OnLoad() override;
  virtual void OnPostLoadLevel() override;
  virtual void OnStartLevel() override;
  virtual void OnCamNavActive() override;
  virtual void OnBallNavActive() override;
  virtual void OnProcess() override;
  virtual void OnLoadScript(iCKSTRING filename, CKBehavior* script) override;

  virtual void OnPostStartMenu() override;

private:
  std::vector<CKBehavior*> physics_bb;
  std::unique_ptr<BGui::Label> status;
  float gravity_factor = float{ FIXED_GRAVITY_RATE }; // hardcoded to prevent cheating
#ifdef ALLOW_CUSTOM_GRAVITY
  const VxVector default_gravity = { 0, -20, 0 };
#else
  const VxVector gravity = { 0, -20 * gravity_factor, 0 };
#endif
  static constexpr const float time_factor = 2.0f;
  CKBehavior* modul03_script = nullptr;
  bool init = false, disabled = false;

  void set_physics();
  void set_modul03_spring_force();
  void update_status_text();

#ifdef ALLOW_CUSTOM_GRAVITY
  class GravityCommand : public ICommand {
    ReducedGravity* mod;
  public:
    GravityCommand(ReducedGravity* m) : mod(m) {}
    virtual std::string GetName() override { return "gravity"; };
    virtual std::string GetAlias() override { return "setgravity"; };
    virtual std::string GetDescription() override { return "Set gravity rate (default 0.6)."; };
    virtual bool IsCheat() override { return false; };

    virtual void Execute(IBML* bml, const std::vector<std::string>& args) override {
      if (args.size() < 2) {
        bml->SendIngameMessage("Usage: gravity <rate>");
        return;
      }
      float rate = ICommand::ParseFloat(args[1], -100, 100);
      mod->gravity_factor = rate;
      if (bml->IsPlaying()) mod->set_physics();
      mod->set_modul03_spring_force();
      mod->update_status_text();
      char msg[64];
      std::snprintf(msg, sizeof msg, "Gravity rate set to %g%%", rate * 100);
      bml->SendIngameMessage(msg);
    }

    virtual const std::vector<std::string> GetTabCompletion(IBML* bml, const std::vector<std::string>& args) override {
      return args.size() == 2 ? std::vector<std::string>{ "0.6", "1" } : std::vector<std::string>{};
    };
  };
#endif
};
