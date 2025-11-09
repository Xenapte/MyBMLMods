#pragma once

#include "../bml_includes.hpp"
#include <memory>

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class ReducedGravity : public IMod {
public:
  ReducedGravity(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "ReducedGravity"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "Reduced Gravity Mode"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "Play with only 60% of the original gravity. Note: all elevators now feature an additional stabilizing weight to maintain functionality."; }
  DECLARE_BML_VERSION;

  virtual void OnLoad() override;
  virtual void OnPostLoadLevel() override;
  virtual void OnStartLevel() override;
  virtual void OnCamNavActive() override;
  virtual void OnBallNavActive() override;
  virtual void OnProcess() override;
  virtual void OnLoadObject(iCKSTRING filename, CKBOOL isMap, iCKSTRING masterName, CK_CLASSID filterClass, CKBOOL addtoscene, CKBOOL reuseMeshes, CKBOOL reuseMaterials, CKBOOL dynamic, XObjectArray* objArray, CKObject* masterObj) override;

  virtual void OnPostStartMenu() override;

private:
  std::vector<CKBehavior*> physics_bb;
  std::unique_ptr<BGui::Label> status;
  const VxVector gravity = { 0, -12, 0 };
  static constexpr const float time_factor = 2;
  bool init = false, disabled = false;

  void set_physics();
};
