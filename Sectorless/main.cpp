#pragma once

#include "../bml_includes.hpp"
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#endif // !WIN32_MEAN_AND_LEAN
#include <numeric>
#include <unordered_map>

extern "C" {
    __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class Sectorless : public IMod {
    IProperty* prop_enabled{}, * prop_remove_checkpoints{}, * prop_dupe_modules{};
    bool enabled{}, remove_checkpoints{}, dupe_modules{};

    void load_config() {
        enabled = prop_enabled->GetBoolean();
        remove_checkpoints = prop_remove_checkpoints->GetBoolean();
        dupe_modules = false; // prop_dupe_modules->GetBoolean();
        // disabled temporarily - pending fix
    }

    void output_abort_message(const char* msg) {
        char text[96];
        std::snprintf(text, sizeof(text), "[%s] %s, aborting", GetID(), msg);
        m_bml->SendIngameMessage(msg);
    }

    CKGroup* get_sector_group(int sector) {
        char sector_name[12];
        std::snprintf(sector_name, sizeof(sector_name), "Sector_%02d", sector);
        auto* group = m_bml->GetGroupByName(sector_name);
        if (!group && sector == 9)
            return m_bml->GetGroupByName("Sector_9");
        return group;
    }

    bool remove_all_other_sectors() {
        auto group_01 = m_bml->GetGroupByName("Sector_01");
        if (!group_01)
            return output_abort_message("Group `Sector_01` not found"), false;
        for (int sector = 2; sector < 1000; sector++) {
            auto* group = get_sector_group(sector);
            if (!group)
                break;
            auto length = group->GetObjectCount();
            for (int i = 0; i < length; i++) {
                group_01->AddObject(group->GetObject(i));
            }
            m_bml->GetCKContext()->DestroyObject(group);
        }
        return true;
    }

    // mostly the same code at remove_all_other_sectors()
    // this is to try to make the code less convoluted
    void dupe_to_other_sectors() {
        //std::vector<CKGroup*> groups;
        std::unordered_map<CKGroup*, std::vector<CKBeObject*>> group_items;
        for (int sector = 1; sector < 1000; sector++) {
            auto* group = get_sector_group(sector);
            if (!group)
                break;
            decltype(group_items)::mapped_type items;
            auto length = group->GetObjectCount();
            items.resize(length);
            for (int i = 0; i < length; i++)
                items[i] = group->GetObject(i);
            group_items.try_emplace(group, items);
        }
        CKDependencies dep;
        dep.Resize(40); dep.Fill(0);
        dep.m_Flags = CK_DEPENDENCIES_CUSTOM;
        dep[CKCID_OBJECT] = CK_DEPENDENCIES_COPY_OBJECT_NAME | CK_DEPENDENCIES_COPY_OBJECT_UNIQUENAME;
        dep[CKCID_BEOBJECT] = CK_DEPENDENCIES_COPY_BEOBJECT_ATTRIBUTES | CK_DEPENDENCIES_COPY_BEOBJECT_GROUPS | CK_DEPENDENCIES_COPY_BEOBJECT_SCRIPTS;
        dep[CKCID_3DOBJECT] = CK_DEPENDENCIES_COPY_3DENTITY_MESH;
        auto* ctx = m_bml->GetCKContext();
        for (auto& [group, items] : group_items) {
            GetLogger()->Info("%s", group->GetName());
            for (auto& [other_group, _] : group_items) {
                if (other_group == group)
                    continue;
                for (auto* obj : items) {
                    auto new_obj = static_cast<CKBeObject*>(ctx->CopyObject(obj, &dep, ".Sectorless", CK_OBJECTCREATION_RENAME));
                    GetLogger()->Info("%s", obj->GetName());
                    other_group->AddObject(new_obj);
                    group->RemoveObject(new_obj);
                }
            }
        }
        m_bml->RestoreIC(m_bml->GetArrayByName("PH_Groups"));
        for (auto& [group, _] : group_items) {
            GetLogger()->Info("----------------- %s", group->GetName());
            for (int i = 0; i < group->GetObjectCount(); i++) {
                GetLogger()->Info("%s", group->GetObject(i)->GetName());
            }
        }
    }

public:
    Sectorless(IBML* bml) : IMod(bml) {}

    virtual iCKSTRING GetID() override { return "Sectorless"; }
    virtual iCKSTRING GetVersion() override { return "0.0.1"; }
    virtual iCKSTRING GetName() override { return GetID(); }
    virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
    virtual iCKSTRING GetDescription() override {
        return
            "A mod for making every module in the map show up at the same time"
            " regardless of their sectors. Useful for previewing your maps.";
    }
    DECLARE_BML_VERSION;

    void OnLoad() override {
        auto config = GetConfig();
        prop_enabled = config->GetProperty("Main", "Enabled");
        prop_enabled->SetDefaultBoolean(true);
        prop_remove_checkpoints = config->GetProperty("Main", "RemoveCheckpoints");
        prop_remove_checkpoints->SetDefaultBoolean(false);
        prop_remove_checkpoints->SetComment("Remove all checkpoints so the map actually only contains 1 single sector.");
        prop_dupe_modules = config->GetProperty("Main", "Experimental_DuplicateModules");
        prop_dupe_modules->SetDefaultBoolean(false);
        prop_dupe_modules->SetComment("Duplicate modules so they show up even after switching the sector. Experimental. May make your game unstable.");
        load_config();
    }

    void OnModifyConfig(iCKSTRING category, iCKSTRING key, IProperty* prop) {
        load_config();
    }

    virtual void OnLoadObject(iCKSTRING filename, CKBOOL isMap, iCKSTRING masterName,
            CK_CLASSID filterClass, CKBOOL addtoscene, CKBOOL reuseMeshes, CKBOOL reuseMaterials,
            CKBOOL dynamic, XObjectArray* objArray, CKObject* masterObj) override {
        if (!enabled || !isMap)
            return;
        if (!dupe_modules) {
            if (!remove_all_other_sectors())
                return;
        }
        else if (!remove_checkpoints) {
            dupe_to_other_sectors();
        }
        if (remove_checkpoints) {
            auto* group_pc = m_bml->GetGroupByName("PC_Checkpoints");
            if (!group_pc)
                return output_abort_message("Group `PC_Checkpoints` not found");
            // constexpr const char* first_pc_name = "PC_TwoFlames_01";
            const VxVector pos{0, std::numeric_limits<float>::infinity(), 0};
            for (int i = 0; i < group_pc->GetObjectCount(); i++) {
                auto* obj = static_cast<CK3dEntity*>(group_pc->GetObject(i));
                obj->SetPosition(VT21_REF(pos));
                /* if (stricmp(obj->GetName(), first_pc_name)) {
                    // m_bml->GetCKContext()->DestroyObject(obj);
                }; */
                // would mess up indices
            }
        }
    }
};

IMod* BMLEntry(IBML* bml) {
    return new Sectorless(bml);
}
