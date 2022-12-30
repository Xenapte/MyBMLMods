#pragma once

#include <BMLPlus/BMLAll.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fstream>
#include <chrono>
#include "client.h"

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

typedef const char* C_CKSTRING;

class ImproperResetDetector : public IMod {
private:
  static constexpr const char *config_directory = "..\\ModLoader\\Config",
    *config_path = "..\\ModLoader\\Config\\BML.cfg";
  client *mmo_client{};
  bool init = false;
  HANDLE hChangeEvent;
  HANDLE hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  FILETIME last_config_write_time;
  std::thread listener_thread;
  CKKEYBOARD reset_hotkey = CKKEY_Q;

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
      if (word != "Suicide")
        continue;
      config >> word;
      if (word == "K")
        continue;
      break;
    }
    try {
      int hotkey_int = std::stoi(word);
      reset_hotkey = static_cast<decltype(reset_hotkey)>(hotkey_int);
      GetLogger()->Info("Reset hotkey: %d", hotkey_int);
    } catch (...) {}
  }

public:
  ImproperResetDetector(IBML* bml) : IMod(bml) {}

  virtual C_CKSTRING GetID() override { return "ImproperResetDetector"; }
  virtual C_CKSTRING GetVersion() override { return "3.4.4"; }
  virtual C_CKSTRING GetName() override { return "Improper Reset Detector"; }
  virtual C_CKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual C_CKSTRING GetDescription() override { return "Detects improper resets (caused by pressing the Reset/Suicide hotkey) in BMMO matches"; }
  DECLARE_BML_VERSION;

  // initialize here since not all mods are loaded by the time of OnLoad
  void OnPreStartMenu() override {
    if (init) return;

    const auto mod_size = m_bml->GetModCount();
    for (int i = 0; i < mod_size; ++i) {
      if (mmo_client = dynamic_cast<client*>(m_bml->GetMod(i))) {
        GetLogger()->Info("Presence of BMMO client detected, got pointer at %#010x", mmo_client);
        load_hotkey();
        init_config_listener();
        break;
      }
    }

    init = true;
  }

  void OnProcess() override {
    if (!m_bml->IsPlaying() || !mmo_client)
      return;
    
    if (mmo_client->connected() && m_bml->GetInputManager()->IsKeyPressed(reset_hotkey)) {
      bmmo::public_warning_msg msg{};
      msg.text_content = mmo_client->get_own_name() + " just pressed the Reset hotkey!";
      msg.serialize();
      mmo_client->send(msg.raw.str().data(), msg.size(), k_nSteamNetworkingSend_Reliable);
    }
  }

  void OnUnload() override {
    SetEvent(hStopEvent);
    listener_thread.join();
    FindCloseChangeNotification(hChangeEvent);
  }
};

IMod* BMLEntry(IBML* bml) {
  return new ImproperResetDetector(bml);
}
