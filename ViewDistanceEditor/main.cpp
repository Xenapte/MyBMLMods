#pragma once

#include "../bml_includes.hpp"

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class ViewDistanceEditor : public IMod {
  bool init = false, notify = true;
  float default_view_distance = 1200, view_distance{};
  IProperty *prop{};
  CKCamera *ingame_cam{};

  auto get_ingame_cam() { return m_bml->GetTargetCameraByName("InGameCam"); }

  void set_view_distance(float new_view_distance) {
    if (ingame_cam->GetBackPlane() == new_view_distance)
      return;
    ingame_cam->SetBackPlane(new_view_distance);
    if (!notify)
      return;
    char msg[48];
    std::snprintf(msg, sizeof(msg), "Set the view distance to %.5g", new_view_distance);
    m_bml->SendIngameMessage(msg);
    notify = false;
  }

public:
  ViewDistanceEditor(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "ViewDistanceEditor"; }
  virtual iCKSTRING GetVersion() override { return "0.0.1"; }
  virtual iCKSTRING GetName() override { return "View Distance Editor"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "View Distance Editor."; }
  DECLARE_BML_VERSION;

  void OnLoad() override {
    GetConfig()->SetCategoryComment("Main", "Main settings.");
    prop = GetConfig()->GetProperty("Main", "ViewDistance");
    prop->SetDefaultFloat(default_view_distance);
    char comment[64];
    std::snprintf(comment, sizeof(comment), "View distance. Default: %.5g", default_view_distance);
    prop->SetComment(comment);
  }

  void OnPostStartMenu() override {
    if (init) return;

    ingame_cam = get_ingame_cam();
    default_view_distance = ingame_cam->GetBackPlane();

    view_distance = prop->GetFloat();

    init = true;
  }
  
  void OnCamNavActive() override {
    set_view_distance(view_distance);
  }

  void OnModifyConfig(iCKSTRING category, iCKSTRING key, IProperty* prop) override {
    notify = true;
    view_distance = prop->GetFloat();
    set_view_distance(view_distance);
  };

  void OnStartLevel() override {
    notify = true;
  };
};

IMod* BMLEntry(IBML* bml) {
  return new ViewDistanceEditor(bml);
}
