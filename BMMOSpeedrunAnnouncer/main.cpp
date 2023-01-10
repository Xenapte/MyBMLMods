#pragma once

#include <BMLPlus/BMLAll.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "exported_client.h"
#ifdef min
#undef min
#endif // min

typedef const char* C_CKSTRING;

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

constexpr const char* lock_path = "..\\ModLoader\\Config\\BMMOSpeedrun.lock";

class DummySpeedrunAnnouncer : public IMod {
protected:
  bool init = false;
  const time_t max_time;
  std::string format_time(time_t timepoint) {
    std::string time_string(32, '\0');
    time_string.resize(strftime(time_string.data(), time_string.size(), "%F %X", localtime(&timepoint)));
    return time_string;
  }

public:
  DummySpeedrunAnnouncer(IBML* bml, time_t max_time) : IMod(bml), max_time(max_time) {}

  virtual C_CKSTRING GetID() override { return "BMMOSpeedrunAnnouncer"; }
  virtual C_CKSTRING GetVersion() override { return "0.0.2"; }
  virtual C_CKSTRING GetName() override { return "BMMO Speedrun Announcer"; }
  virtual C_CKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual C_CKSTRING GetDescription() override {
    return "Announces your valid speedrun records while you're online with BMMO.";
  }
  DECLARE_BML_VERSION;

  virtual void OnPostStartMenu() override {
    if (init)
      return;
    char warning[160];
    snprintf(warning, sizeof(warning), "[WARNING]: Lock (%s) expired at %s.",
             lock_path, format_time(max_time).c_str());
    m_bml->SendIngameMessage(warning);
    m_bml->SendIngameMessage("    SpeedrunAnnouncer will be disabled until the lock is removed.");
    init = true;
  }
};

class BMMOSpeedrunAnnouncer : public DummySpeedrunAnnouncer {
private:
  bool counting = false, valid = false, level_finished = false;
  float speedrun_time = 0;
  bmmo::exported::client *mmo_client{};
  CKTimeManager *time_manager{};
  CKDataArray *array_energy{}, *array_all_level{};

  inline void update_valid_status() {
    if (valid && mmo_client)
      valid = mmo_client->connected() && !m_bml->IsCheatEnabled();
  }

public:
  using DummySpeedrunAnnouncer::DummySpeedrunAnnouncer;

  void OnPostStartMenu() override {
    if (init)
      return;

    const int mod_count = m_bml->GetModCount();
    for (int i = 0; i < mod_count; ++i) {
      if (mmo_client = dynamic_cast<decltype(mmo_client)>(m_bml->GetMod(i))) {
        GetLogger()->Info("Presence of BMMO client detected, got pointer at %#010x", mmo_client);
        GetLogger()->Info("Lock valid. Expiration time: %s (+%ds).",
                          format_time(max_time).c_str(),
                          max_time - std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
        time_manager = m_bml->GetTimeManager();
        break;
      }
    }

    array_energy = m_bml->GetArrayByName("Energy");
    array_all_level = m_bml->GetArrayByName("AllLevel");

    init = true;
  }

  void OnProcess() override {
    if (!(counting && valid))
      return;
    speedrun_time += time_manager->GetLastDeltaTime();
  }

  void OnCounterActive() override {
    update_valid_status();
    if (!valid)
      return;
    counting = true;
  }

  void OnCounterInactive() override {
    update_valid_status();
    counting = false;
    if (!(level_finished && valid && mmo_client))
      return;
    level_finished = false;
 
    const auto current_map = mmo_client->get_current_map();
    int points, lives, life_bonus, level_bonus;
    array_energy->GetElementValue(0, 0, &points);
    array_energy->GetElementValue(0, 1, &lives);
    array_energy->GetElementValue(0, 5, &life_bonus);
    array_all_level->GetElementValue(current_map.level - 1, 6, &level_bonus);

    if (current_map.level * 100 != level_bonus || life_bonus != 200)
      return;

    int counter = int(speedrun_time);
    int ms = counter % 1000;
    counter /= 1000;
    int s = counter % 60;
    counter /= 60;
    int m = counter % 60;
    counter /= 60;
    int h = counter % 100;
    static char time_string[16];
    sprintf(time_string, "%02d:%02d:%02d.%03d", h, m, s, ms);
    int score = level_bonus + points + lives * life_bonus;

    bmmo::public_notification_msg msg{};
    msg.type = bmmo::public_notification_type::Info;
    msg.text_content.resize(192);
    msg.text_content.resize(snprintf(msg.text_content.data(), msg.text_content.size(),
      "Valid speedrun: %s | %s | %s | %d",
      mmo_client->get_client_name().c_str(), current_map.get_display_name().c_str(),
      time_string, score));
    msg.serialize();
    mmo_client->send(msg.raw.str().data(), msg.size(), k_nSteamNetworkingSend_Reliable);
  }

  void OnPauseLevel() override {
    update_valid_status();
    counting = false;
  }

  void OnUnpauseLevel() override {
    update_valid_status();
    counting = true;
  }

  void OnStartLevel() override {
    speedrun_time = 0;
    valid = true;
  }

  void OnLevelFinish() override {
    level_finished = true;
  }

  void OnCheatEnabled(bool enable) override {
    valid = false;
  }
};

IMod* BMLEntry(IBML* bml) {
  using namespace std::chrono;
  constexpr const auto max_time_length = 3h;
  time_t max_time{};
  if (std::filesystem::exists(lock_path)) {
    std::ifstream f(lock_path);
    f.read(reinterpret_cast<char*>(&max_time), sizeof(max_time));
    if (max_time <= system_clock::to_time_t(system_clock::now()))
      return new DummySpeedrunAnnouncer(bml, max_time);
  }
  else {
    std::ofstream f(lock_path);
    max_time = system_clock::to_time_t(system_clock::now() + max_time_length);
    f.write(reinterpret_cast<const char*>(&max_time), sizeof(max_time));
  }
  return new BMMOSpeedrunAnnouncer(bml, max_time);
}
