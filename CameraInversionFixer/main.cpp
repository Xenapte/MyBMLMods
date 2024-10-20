#pragma once

#include "../bml_includes.hpp"
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#endif // !WIN32_MEAN_AND_LEAN

extern "C" {
    __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class CameraInversionFixer : public IMod {
private:
    std::function<void()> process_func = [] {};
    bool init = false, debug_toggle = false;
    CKKEYBOARD fix_key{};
    CK_ID ingame_cam{};
    IProperty* prop_enabled{}, * prop_ensure_fix{}, * prop_fix_key{}, * prop_toggle{};

    void fix_inversion() {
        VxVector dir, up;
        auto* cam = static_cast<CKCamera*>(m_bml->GetCKContext()->GetObject(ingame_cam));
        cam->GetOrientation(&dir, &up);
        if (up.y < 0 || debug_toggle) {
            VxVector pos; cam->GetPosition(&pos);
            m_bml->RestoreIC(cam);
            up.y = -up.y;
            cam->SetOrientation(VT21_REF(dir), VT21_REF(up));
            m_bml->AddTimer(CKDWORD(0), [=] {
                cam->SetPosition(VT21_REF(pos));
            });
        }
    }

    void load_config() {
        if (!prop_enabled->GetBoolean()) {
            process_func = [] {};
            return;
        }
        debug_toggle = prop_toggle->GetBoolean();
        if (prop_ensure_fix->GetBoolean()) {
            debug_toggle = false;
            process_func = [this] {
                fix_inversion();
            };
        }
        else {
            fix_key = prop_fix_key->GetKey();
            process_func = [this] {
                if (m_bml->GetInputManager()->IsKeyPressed(fix_key)) {
                    fix_inversion();
                }
            };
        }
    }

public:
    CameraInversionFixer(IBML* bml) : IMod(bml) {}

    virtual iCKSTRING GetID() override { return "CameraInversionFixer"; }
    virtual iCKSTRING GetVersion() override { return "0.0.1"; }
    virtual iCKSTRING GetName() override { return "Camera Inversion Fixer"; }
    virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
    virtual iCKSTRING GetDescription() override {
        return "A mod for fixing camera inversions.";
    }
    DECLARE_BML_VERSION;
    
    void OnLoad() override {
        GetConfig()->SetCategoryComment("Main", "Main settings.");
        prop_enabled = GetConfig()->GetProperty("Main", "Enabled");
        prop_enabled->SetDefaultBoolean(true);
        prop_enabled->SetComment("Whether to enable the mod.");
        prop_ensure_fix = GetConfig()->GetProperty("Main", "EnsureFix");
        prop_ensure_fix->SetDefaultBoolean(false);
        prop_ensure_fix->SetComment("Check for camera every frame to ensure it is never inverted. Could have visible framerate impacts. If not enabled, use the fix key to manually flip the camera back.");
        prop_fix_key = GetConfig()->GetProperty("Main", "FixKey");
        prop_fix_key->SetDefaultKey(CKKEY_B);
        prop_fix_key->SetComment("Key to fix the camera inversion.");
        prop_toggle = GetConfig()->GetProperty("Main", "Debug_ToggleInversion");
        prop_toggle->SetDefaultBoolean(false);
        prop_toggle->SetComment("[Debug] Make the camera fix key toggle the inversion instead.");
    }

    void OnPreStartMenu() override {
        if (init) return;

        ingame_cam = CKOBJID(m_bml->GetTargetCameraByName("InGameCam"));
        load_config();

        init = true;
    }

    void OnProcess() override {
        if (!m_bml->IsPlaying()) return;
        process_func();
    }

    void OnModifyConfig(iCKSTRING category, iCKSTRING key, IProperty* prop) override {
        load_config();
    }
};

IMod* BMLEntry(IBML* bml) {
    return new CameraInversionFixer(bml);
}
