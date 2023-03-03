#pragma once

#include <BML/BMLAll.h>

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class RotationIndicator : public IMod {
  bool init = false;
  CKDataArray *current_level_array{};
  CK3dObject *indicator{};

  CK3dObject* get_current_ball() {
    return static_cast<CK3dObject*>(current_level_array->GetElementObject(0, 1));
  }

public:
  RotationIndicator(IBML* bml) : IMod(bml) {}

  virtual CKSTRING GetID() override { return "RotationIndicator"; }
  virtual CKSTRING GetVersion() override { return "0.0.1"; }
  virtual CKSTRING GetName() override { return "Rotation Indicator"; }
  virtual CKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual CKSTRING GetDescription() override { return "Indicates your ball's rotation."; }
  DECLARE_BML_VERSION;

  void OnPostStartMenu() override {
    if (init) return;

    current_level_array = m_bml->GetArrayByName("CurrentLevel");

    auto pm = m_bml->GetPathManager();
    /*for (int i = 0; i < pm->GetCategoryCount(); ++i) {
      XString name;
      pm->GetCategoryName(i, name);
      m_bml->SendIngameMessage(name.CStr());
    }*/
    int data_path_index = pm->GetCategoryIndex("Data Paths");

    /*for (int i = 0; i < pm->GetPathCount(data_path_index); ++i) {
      XString name;
      pm->GetPathName(data_path_index, i, name);
      m_bml->SendIngameMessage(name.CStr());
    }*/

    XString indicator_file("Rotation_Indicator.nmo");
    pm->ResolveFileName(indicator_file, data_path_index);
    //m_bml->SendIngameMessage(indicator_file.CStr());
    ExecuteBB::ObjectLoad(indicator_file.CStr(), false);

    indicator = static_cast<CK3dObject*>(m_bml->GetCKContext()->GetObjectByName("___Rotation_Indicator"));

    init = true;
  }

  void OnProcess() override {
    if (!m_bml->IsPlaying()) return;
    auto current_ball = get_current_ball();
    VxVector pos; VxQuaternion rot;
    current_ball->GetPosition(&pos);
    current_ball->GetQuaternion(&rot);
    indicator->SetPosition(pos);
    indicator->SetQuaternion(rot);
  }
};

IMod* BMLEntry(IBML* bml) {
  return new RotationIndicator(bml);
}
