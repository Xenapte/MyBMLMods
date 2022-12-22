#include <BML/BMLAll.h>
// #include <numbers>

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class FreeViewRotation : public IMod {
private:
  bool init = false;
  ////// CK_ID cam{};
  CKBehavior *rotate_view_script{}, *interpolation_script{};
  constexpr static int ROTATE_PARAM_INDEX = 0;
  ////// int counter = 0, c2{};
  float default_rotation_degree[3]{};
  float default_interpolation_duration{};

  bool enabled = false, cursor_enabled = true;
  float sensitivity = 3;
  IProperty *prop_enabled{}, *prop_cursor_enabled{}, *prop_sensitivity{};

  bool camera_need_reset = false;

  void show_cursor() {
    if (enabled && cursor_enabled)
      m_bml->GetInputManager()->ShowCursor(true);
  }

  inline void set_interpolation_duration(float v) {
    interpolation_script->GetInputParameter(0)->GetDirectSource()->SetValue(&v);
  }

  inline void set_rotation_degree(float v[3]) {
    rotate_view_script->GetInputParameter(ROTATE_PARAM_INDEX)->GetDirectSource()->SetValue(v, 3 * sizeof(float));
    // why (sizeof(v) == 4) here ?????
  }

  void load_config_values() {
    enabled = prop_enabled->GetBoolean();
    cursor_enabled = prop_cursor_enabled->GetBoolean();
    sensitivity = prop_sensitivity->GetFloat();

    if (interpolation_script) {
      if (enabled) {
        set_interpolation_duration(0);
      }
      else {
        set_interpolation_duration(default_interpolation_duration);
      }
    }
    if (rotate_view_script && !enabled) {
      set_rotation_degree(default_rotation_degree);
      camera_need_reset = true;
    }
  }

public:
  FreeViewRotation(IBML* bml) : IMod(bml) {}

  virtual CKSTRING GetID() override { return "FreeViewRotation"; }
  virtual CKSTRING GetVersion() override { return "0.0.1"; }
  virtual CKSTRING GetName() override { return "Free View Rotation"; }
  virtual CKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual CKSTRING GetDescription() override { return "Tired of our fixed view rotation? Let's control it by using the mouse!"; }
  DECLARE_BML_VERSION;

  void OnLoad() override {
    auto *config = GetConfig();
    config->SetCategoryComment("Main", "Main Settings");
    prop_enabled = config->GetProperty("Main", "Enabled");
    prop_enabled->SetDefaultBoolean(true);
    prop_enabled->SetComment(
      "We're no strangers to settings / "
      "You know the rules and so do I / "
      "A full commitment's what I'm thinking of / "
      "You wouldn't get this from any other option"
    );
    prop_cursor_enabled = config->GetProperty("Main", "ShowCursor");
    prop_cursor_enabled->SetDefaultBoolean(true);
    prop_cursor_enabled->SetComment(
      "I just wanna tell you how the cursor's feeling / "
      "Gotta make you understand"
    );
    prop_sensitivity = config->GetProperty("Main", "Sensitivity");
    prop_sensitivity->SetDefaultFloat(4.0f);
    prop_sensitivity->SetComment("Default: 4. Can be negative. "
      "We've known each other for so long / "
      "Your hand's been aching, but you're too shy to say it / "
      "Inside, we both know what's been going on / "
      "We know the game, and we're gonna play it"
    );

    load_config_values();
  }

  void OnModifyConfig(CKSTRING category, CKSTRING key, IProperty* prop) {
    load_config_values();
  }

  // initialize object pointers
  void OnPostLoadLevel() override {
    if (init)
      return;

    ////// cam = CKOBJID(m_bml->Get3dEntityByName("Cam_MF"));

    // get our script pointers
    auto *nav_script = ScriptHelper::FindFirstBB(m_bml->GetScriptByName("Gameplay_Ingame"),
      "Cam Navigation");
    interpolation_script = ScriptHelper::FindFirstBB(nav_script, "Bezier Progression");
    rotate_view_script = ScriptHelper::FindPreviousBB(nav_script, interpolation_script->GetInput(0));

    rotate_view_script->GetInputParameter(ROTATE_PARAM_INDEX)->GetDirectSource()->GetValue(default_rotation_degree);

    // rotation will not complete before the animation finishes
    interpolation_script->GetInputParameter(0)->GetDirectSource()->GetValue(&default_interpolation_duration);
    if (enabled)
      set_interpolation_duration(0);
    /*  float t[6];
        rotate_view_script->GetInputParameter(0)->GetDirectSource()->GetValue(t);
        for (const float i: t) m_bml->SendIngameMessage(std::to_string(i).c_str());
        rotate_view_script->GetInputParameter(1)->GetDirectSource()->GetValue(t);
        for (const float i: t) m_bml->SendIngameMessage(std::to_string(i).c_str());
        m_bml->SendIngameMessage(interpolation_script->GetInputParameter(0)->GetDirectSource()->GetName());
        float interpolation_duration;
        interpolation_script->GetInputParameter(0)->GetDirectSource()->GetValue(&interpolation_duration);
        char t[64];
        snprintf(t, 64, "%f %d", interpolation_duration, interpolation_script->GetInputParameter(0)->GetDirectSource()->GetDataSize());
        m_bml->SendIngameMessage(t); */
  }

  void OnStartLevel() override { show_cursor(); }
  void OnUnpauseLevel() override {
    m_bml->AddTimer(2u, [this] {
      show_cursor();

      if (!camera_need_reset)
        return;
      camera_need_reset = false;
      auto* cam = m_bml->Get3dEntityByName("Cam_MF");
      m_bml->RestoreIC(cam, true);
      VxVector position;
      // player ball
      static_cast<CK3dObject*>(m_bml->GetArrayByName("CurrentLevel")->GetElementObject(0, 1))
        ->GetPosition(&position);
      cam->SetPosition(position);
    });
  }

  void OnProcess() override {
    if (!m_bml->IsPlaying() || !enabled)
      return;

    /*  if (m_bml->GetInputManager()->IsKeyPressed(CKKEY_5)) {
          rotate_view_script->ActivateInput(0);
          rotate_view_script->Activate();
        }
        if (m_bml->GetInputManager()->IsKeyPressed(CKKEY_6)) {
          rotate_view_script->ActivateInput(1);
          rotate_view_script->Activate();
        } */
    /*  ++counter;
        ++c2;
        if (counter != c2 / 420 + 1) {
          return;
        }
        float interpolation_duration = c2 / 420;
        interpolation_script->GetInputParameter(0)->GetDirectSource()->SetValue(&interpolation_duration);
    */ // 180 / std::numbers::pi_v<float>
    VxVector mouse_pos_delta;
    m_bml->GetInputManager()->GetMouseRelativePosition(mouse_pos_delta);
    float rotation[3] = { 0, mouse_pos_delta.x * sensitivity / m_bml->GetRenderContext()->GetWidth(), 0 };
    set_rotation_degree(rotation);
    rotate_view_script->ActivateInput(ROTATE_PARAM_INDEX);
    rotate_view_script->Activate();
    ////// counter = 0;
  }
};

IMod* BMLEntry(IBML* bml) {
  return new FreeViewRotation(bml);
}
