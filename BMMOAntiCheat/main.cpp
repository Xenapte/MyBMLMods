#pragma once

#include "../bml_includes.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fstream>
#include <chrono>
#include <numeric>
#include "exported_client.h"
#include "translations.hpp"

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class BMMOAntiCheat : public IMod, public bmmo::exported::listener {
private:
  static constexpr const char *config_directory = "..\\ModLoader\\Config",
    *config_path = "..\\ModLoader\\Config\\BML.cfg";
  bmmo::exported::client *mmo_client{};
  const std::string version = []() -> decltype(version) {
    char version_str[32];
    std::snprintf(version_str, sizeof(version_str), "0.2.1_bmmo-%u.%u.%u",
                  bmmo::current_version.major, bmmo::current_version.minor, bmmo::current_version.subminor);
    return version_str;
  }();
  bool init = false;
  HANDLE hChangeEvent{};
  HANDLE hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  FILETIME last_config_write_time{};
  std::thread listener_thread;
  CKKEYBOARD reset_hotkey = CKKEY_R;
  bool reset_enabled = true;

  void init_config_listener() {
    listener_thread = std::thread([this] {
      hChangeEvent = FindFirstChangeNotification(config_directory, false, FILE_NOTIFY_CHANGE_LAST_WRITE);
      if (hChangeEvent == INVALID_HANDLE_VALUE || hChangeEvent == NULL) {
        GetLogger()->Error("FindFirstChangeNotification function failed.");
        return;
      }
      HANDLE hDirectory = CreateFile(config_directory, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

      HANDLE hEvents[] = { hChangeEvent, hStopEvent };

      while (true) {
        auto dwWaitStatus = WaitForMultipleObjects(sizeof(hEvents) / sizeof(HANDLE), hEvents, false, INFINITE);
        switch (dwWaitStatus) {
          case WAIT_OBJECT_0:
            load_hotkey();
            if (FindNextChangeNotification(hEvents[0]) == FALSE) {
              GetLogger()->Error("FindNextChangeNotification function failed.");
              return;
            }
            break;
          case WAIT_OBJECT_0 + 1:
            return;
          default:
            GetLogger()->Error("Unhandled dwWaitStatus.");
            break;
        }
      }
    });
  }

  void load_reset_key(const std::string& word) {
    try {
      int hotkey_int = std::stoi(word);
      reset_hotkey = static_cast<decltype(reset_hotkey)>(hotkey_int);
      GetLogger()->Info("Reset hotkey: %d", hotkey_int);
    }
    catch (...) {}
  }

  void load_hotkey() {
    using namespace std::chrono;
    static int64_t last_load_time = 0;
    auto current_time = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    if (current_time - last_load_time < 1000)
      return;
    last_load_time = current_time;

    std::this_thread::sleep_for(250ms);
    HANDLE hConfig = CreateFile(config_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    FILETIME config_write_time;
    GetFileTime(hConfig, NULL, NULL, &config_write_time);
    CloseHandle(hConfig);
    if (CompareFileTime(&config_write_time, &last_config_write_time) == 0)
      return;
    last_config_write_time = config_write_time;

    std::ifstream config(config_path);
    std::string word;
    while (config >> word) {
      if (word == "Suicide") {
        config >> word;
        if (word == "K" || word == "Hotkey")
          continue;
        load_reset_key(word);
      }
      else if (word.starts_with("EnableSuicide")) {
        config >> reset_enabled;
      }
    }
  }

public:
  BMMOAntiCheat(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "BMMOAntiCheat"; }
  virtual iCKSTRING GetVersion() override { return version.c_str(); }
  virtual iCKSTRING GetName() override { return "BMMO Anti-Cheat"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override {
    return
      "Anti-Cheat for BMMO.\n\n"
      "Currently it:\n"
      "- Detects improper resets (caused by pressing the Reset/Suicide hotkey) in BMMO matches.\n"
      "- Detects incorrect screen resolution."
    ;
  }
  DECLARE_BML_VERSION;

  // initialize here since not all mods are loaded by the time of OnLoad
  void OnPreStartMenu() override {
    if (init) return;

    const auto mod_size = m_bml->GetModCount();
    for (int i = 0; i < mod_size; ++i) {
      if (mmo_client = dynamic_cast<decltype(mmo_client)>(m_bml->GetMod(i))) {
        GetLogger()->Info("Presence of BMMO client detected, got pointer at %#010x", mmo_client);
        load_hotkey();
        init_config_listener();
        mmo_client->register_listener(this);
        break;
      }
    }

    init = true;
  }

  void OnProcess() override {
    if (!m_bml->IsPlaying() || !mmo_client || mmo_client->is_spectator() || !mmo_client->connected())
      return;
    
    if (reset_enabled && m_bml->GetInputManager()->IsKeyPressed(reset_hotkey)) {
      bmmo::public_notification_msg msg{};
      msg.type = bmmo::public_notification_type::SeriousWarning;
      msg.text_content = mmo_client->get_own_id().second + " just pressed the Reset hotkey at "
        + mmo_client->get_current_map().get_display_name() + "!";
      msg.serialize();
      mmo_client->send(msg.raw.str().data(), msg.size(), k_nSteamNetworkingSend_Reliable);
    }
  }

  void OnUnload() override {
    SetEvent(hStopEvent);
    listener_thread.join();
    FindCloseChangeNotification(hChangeEvent);
  }

  bool on_pre_login(const char* address, const char* name) override {
    VxRect screen_rect;
    m_bml->GetRenderContext()->GetViewRect(screen_rect);
    auto height = (int) std::roundf(screen_rect.GetHeight()),
      width = (int) std::roundf(screen_rect.GetWidth()),
      x = width, y = height;
    auto gcd = std::gcd(x, y);
    x /= gcd; y /= gcd;
    if (x == 4 && y == 3)
      return true;

    m_bml->SendIngameMessage(translator::get(ResolutionNotice, false).c_str());
    char t[128];
    std::snprintf(t, sizeof t, translator::get(CurrentResolution, false).c_str(),
                  width, height, x, y);
    m_bml->SendIngameMessage(t);
    std::snprintf(t, sizeof t, translator::get(CurrentResolution).c_str(),
                  width, height, x, y);
    std::ignore = MessageBox(NULL,
                             (translator::get(ResolutionNotice) + "\n" + t).c_str(),
                             translator::get(ConfigIssueDetected).c_str(),
                             MB_OK | MB_ICONWARNING);
    return false;
  }
};

IMod* BMLEntry(IBML* bml) {
  return new BMMOAntiCheat(bml);
}
