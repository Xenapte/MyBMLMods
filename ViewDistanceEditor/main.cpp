#pragma once

#include "../bml_includes.hpp"

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class ViewDistanceEditor : public IMod {
  bool init = false, notify = true;
  float default_view_distance = 1200, view_distance{};
  float default_life_distance = 60, life_distance{},
      default_point_distance = 80, point_distance{};
  IProperty* prop_view_distance{}, * prop_life_distance{}, * prop_point_distance{};
  CKCamera *ingame_cam{};

  auto get_ingame_cam() { return m_bml->GetTargetCameraByName("InGameCam"); }

  void set_view_distance(float new_view_distance) {
    if (ingame_cam->GetBackPlane() == new_view_distance)
      return;
    ingame_cam->SetBackPlane(new_view_distance);
    if (!notify)
      return;
    char msg[48];
    std::snprintf(msg, sizeof(msg), "View distance set to %.5g", new_view_distance);
    m_bml->SendIngameMessage(msg);
    notify = false;
  }

public:
  ViewDistanceEditor(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "ViewDistanceEditor"; }
  virtual iCKSTRING GetVersion() override { return "0.0.2"; }
  virtual iCKSTRING GetName() override { return "View Distance Editor"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override { return "View Distance Editor."; }
  DECLARE_BML_VERSION;

  void OnLoad() override {
    GetConfig()->SetCategoryComment("Main", "Main settings.");
    prop_view_distance = GetConfig()->GetProperty("Main", "ViewDistance");
    prop_view_distance->SetDefaultFloat(default_view_distance);
    char comment[64];
    std::snprintf(comment, sizeof(comment), "View distance. Default: %.5g", default_view_distance);
    prop_view_distance->SetComment(comment);
    prop_life_distance = GetConfig()->GetProperty("Main", "ExtraLifeDistance");
    prop_life_distance->SetDefaultFloat(default_life_distance);
    std::snprintf(comment, sizeof(comment), "View distance of extra lives. Default: %.5g", default_life_distance);
    prop_life_distance->SetComment(comment);
    prop_point_distance = GetConfig()->GetProperty("Main", "ExtraPointDistance");
    prop_point_distance->SetDefaultFloat(default_point_distance);
    std::snprintf(comment, sizeof(comment), "View distance of extra points. Default: %.5g", default_point_distance);
    prop_point_distance->SetComment(comment);
  }

  void OnPostStartMenu() override {
    if (init) return;

    ingame_cam = get_ingame_cam();
    default_view_distance = ingame_cam->GetBackPlane();

    view_distance = prop_view_distance->GetFloat();
    life_distance = prop_life_distance->GetFloat();
    point_distance = prop_point_distance->GetFloat();

    init = true;
  }

  virtual void OnLoadObject(iCKSTRING filename, CKBOOL isMap, iCKSTRING masterName,
      CK_CLASSID filterClass, CKBOOL addtoscene, CKBOOL reuseMeshes, CKBOOL reuseMaterials,
      CKBOOL dynamic, XObjectArray* objArray, CKObject* masterObj) override {
    if (_strcmpi(filename, "3D Entities\\PH\\P_Extra_Life.nmo") == 0) {
      auto* bb = ScriptHelper::FindFirstBB(m_bml->GetScriptByName("P_Extra_Life_MF Script"),
                                           "TT Scaleable Proximity");
      bb->GetInputParameter(0)->GetDirectSource()->GetValue(&default_life_distance);
      if (default_life_distance != life_distance) {
          auto temp = life_distance;
          bb->GetInputParameter(0)->GetDirectSource()->SetValue(&temp);
          bb->GetInputParameter(4)->GetDirectSource()->SetValue(&temp);
          temp += 10;
          bb->GetInputParameter(5)->GetDirectSource()->SetValue(&temp);
      }
    }
    else if (_strcmpi(filename, "3D Entities\\PH\\P_Extra_Point.nmo") == 0) {
        auto* bb = ScriptHelper::FindFirstBB(m_bml->GetScriptByName("P_Extra_Point_MF Script"),
                                             "TT Scaleable Proximity");
        bb->GetInputParameter(0)->GetDirectSource()->GetValue(&default_point_distance);
        if (default_point_distance != point_distance) {
            auto temp = point_distance;
            bb->GetInputParameter(0)->GetDirectSource()->SetValue(&temp);
            temp += 5;
            bb->GetInputParameter(4)->GetDirectSource()->SetValue(&temp);
            temp += 15;
            bb->GetInputParameter(5)->GetDirectSource()->SetValue(&temp);
        }
    }
  }
  
  void OnCamNavActive() override {
    set_view_distance(view_distance);
  }

  void OnModifyConfig(iCKSTRING category, iCKSTRING key, IProperty* prop) override {
    notify = true;
    view_distance = prop_view_distance->GetFloat();
    set_view_distance(view_distance);
    life_distance = prop_life_distance->GetFloat();
    point_distance = prop_point_distance->GetFloat();
  };

  void OnStartLevel() override {
    notify = true;
  };
};

IMod* BMLEntry(IBML* bml) {
  return new ViewDistanceEditor(bml);
}
