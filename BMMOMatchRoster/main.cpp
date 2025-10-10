#pragma once

#include <algorithm>
#include <ranges>
#include <set>
#include "../bml_includes.hpp"
#include "exported_client.h"

extern "C" {
  __declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

using bmmo::Sprintf;
using bmmo::ansi;

class BMMOMatchRoster : public IMod, public bmmo::exported::listener {
private:
  bmmo::exported::client* mmo_client_{};
  bool init_ = false, ansi_color_supported_ = false;
  IProperty* prop_enrolled_players_{};

  class MatchRosterCommand: public ICommand {
  private:
    BMMOMatchRoster& mod_;

    std::string GetName() override { return "matchroster"; };
    std::string GetAlias() override { return "roster"; };
    std::string GetDescription() override { return "Checks online players."; };
    bool IsCheat() override { return false; };

    void Execute(IBML* bml, const std::vector<std::string>& args) override {
      mod_.on_command(args);
    };

    const std::vector<std::string> GetTabCompletion(IBML* bml, const std::vector<std::string>& args) override {
      return mod_.on_tab_completion(args);
    };

  public:
    MatchRosterCommand(BMMOMatchRoster& mod) : mod_(mod) {}
  };

  void ingame_msg(std::string msg, int color = bmmo::ansi::Reset) const {
    if (ansi_color_supported_ && color != bmmo::ansi::Reset)
      msg = bmmo::ansi::get_escape_code(color) + msg + bmmo::ansi::RESET;
    m_bml->SendIngameMessage(msg.c_str());
  }

  void init_config() {
    auto* config = GetConfig();
    config->SetCategoryComment("Main", "Main settings for BMMO Match Roster");
    prop_enrolled_players_ = config->GetProperty("Main", "EnrolledPlayers");
    prop_enrolled_players_->SetComment("Comma-separated list of enrolled players.");
    prop_enrolled_players_->SetDefaultString("");
    load_enrolled_players();
  }

  void load_enrolled_players() {
    const std::string enrolled_str = prop_enrolled_players_->GetString();
    enrolled_players_ = bmmo::string_utils::split_strings(enrolled_str, ',');
    std::ranges::sort(enrolled_players_, [](const auto& a, const auto& b) {
      return bmmo::string_utils::to_lower(a) < bmmo::string_utils::to_lower(b);
    });
  }

  void save_enrolled_players() {
    prop_enrolled_players_->SetString(bmmo::string_utils::join_strings(enrolled_players_, 0, ",").c_str());
  }

  void print_player_list(const std::vector<std::string>& players) const {
    std::string line;
    for (const auto& player : players) {
      static constexpr size_t max_line_length = 70;
      line += player;
      if (line.length() > max_line_length) {
        m_bml->SendIngameMessage(line.c_str());
        line.clear();
      }
      else {
        line += ", ";
      }
    }
    if (!line.empty()) {
      if (line.ends_with(", "))
        line.erase(line.length() - 2);
      m_bml->SendIngameMessage(line.c_str());
    }
  }

  std::vector<std::string> enrolled_players_{};

  // remember that the player could be online but in spectator mode
  bool verify_enrolled_players(bool quiet = false) const {
    if (!mmo_client_) return true;
    auto clients = mmo_client_->get_client_list();
    std::set<std::string> online_players;
    for (const auto& [_, name] : clients) {
      if (bmmo::name_validator::is_spectator(name)) continue;
      online_players.insert(name);
    }
    if (!mmo_client_->is_spectator())
      online_players.insert(mmo_client_->get_own_id().second);
    std::vector<std::string> offline_players;
    for (const auto& player : enrolled_players_) {
      if (!online_players.contains(player)) {
        offline_players.push_back(player);
      }
    }
    if (offline_players.empty()) {
      if (!quiet)
        ingame_msg(Sprintf("All %u enrolled player(s) are online.", enrolled_players_.size()), ansi::Green);
    }
    else if (!quiet) {
      ingame_msg(Sprintf("The following %u enrolled player(s) are NOT online:", offline_players.size()), ansi::Red);
      print_player_list(offline_players);
    }
    for (const auto& player : enrolled_players_) {
      online_players.erase(player);
    }
    if (!online_players.empty()) {
      if (!quiet) {
        ingame_msg(Sprintf("The following %u player(s) are online but not enrolled:", online_players.size()), ansi::Magenta);
        print_player_list(std::vector<std::string>(online_players.begin(), online_players.end()));
      }
      return false;
    }
    return offline_players.empty();
  }

public:
  BMMOMatchRoster(IBML* bml) : IMod(bml) {}

  virtual iCKSTRING GetID() override { return "BMMOMatchRoster"; }
  virtual iCKSTRING GetVersion() override { return "0.1.0"; }
  virtual iCKSTRING GetName() override { return "BMMO Match Roster"; }
  virtual iCKSTRING GetAuthor() override { return "BallanceBug"; }
  virtual iCKSTRING GetDescription() override {
    return
      "Online checker for BMMO. Detects players that are not online when your match starts!"
    ;
  }
  DECLARE_BML_VERSION;

  void OnLoad() override {
    m_bml->RegisterCommand(new MatchRosterCommand(*this));
    init_config();
  }

  // initialize here since not all mods are loaded by the time of OnLoad
  void OnPreStartMenu() override {
    if (init_) return;

    const auto mod_size = m_bml->GetModCount();
    for (int i = 0; i < mod_size; ++i) {
      auto* mod = m_bml->GetMod(i);
#ifdef USING_BML_PLUS
      if (std::strcmp(mod->GetID(), "BML") == 0) {
        BMLVersion loader_version{};
        int count = std::sscanf(mod->GetVersion(), "%d.%d.%d",
          &loader_version.major, &loader_version.minor, &BMMO_BML_BUILD_VERSION(loader_version));
        if (loader_version >= BMLVersion{0, 3, 9})
          ansi_color_supported_ = true;
      }
#endif
      if (mmo_client_ = dynamic_cast<decltype(mmo_client_)>(mod)) {
        GetLogger()->Info("Presence of BMMO client detected, got pointer at %#010x", mmo_client_);
        mmo_client_->register_listener(this);
        break;
      }
    }

    init_ = true;
  }

  void OnModifyConfig(const char* category, const char* key, IProperty* prop) override {
    load_enrolled_players();
  }

  void on_countdown(const bmmo::countdown_msg* msg) override {
    if (enrolled_players_.empty())
      return;
    if (!verify_enrolled_players(true)) {
      ingame_msg("[Warning] Roster verification failed! You may need to abort the match.", ansi::BrightRed);
      ingame_msg("[Warning] Please use \"/roster verify\" to check the current status.", ansi::BrightRed);
    }
  };

  void on_command(const std::vector<std::string>& args) {
    if (!mmo_client_) {
      ingame_msg("BMMO client not found, cannot execute command.", ansi::Red);
      return;
    }
    if (!mmo_client_->connected()) {
      ingame_msg("Not connected to a BMMO server.", ansi::Red);
      return;
    }
    switch (args.size()) {
      case 1: {
        if (enrolled_players_.empty()) {
          ingame_msg("No enrolled players.", ansi::Underline);
        }
        else {
          ingame_msg(Sprintf("Enrolled players (count: %u):", enrolled_players_.size()), ansi::Underline);
          print_player_list(enrolled_players_);
        }
        break;
      }
      case 2: {
        auto lower1 = bmmo::string_utils::to_lower(args[1]);
        if (lower1 == "help" || lower1 == "?") {
          m_bml->SendIngameMessage("Usage: /matchroster [add|clear|enroll|remove|verify]");
          m_bml->SendIngameMessage("Without arguments, lists enrolled players.");
          //m_bml->SendIngameMessage("add <name> - add a player to the roster.");
          m_bml->SendIngameMessage("clear - clear the roster.");
          m_bml->SendIngameMessage("enroll - enroll ALL online non-spectator players.");
          //m_bml->SendIngameMessage("remove <name> - remove a player from the roster.");
          m_bml->SendIngameMessage("verify - verify enrolled players are online.");
          break;
        }
        else if (lower1 == "clear") {
          enrolled_players_.clear();
          save_enrolled_players();
          ingame_msg("Cleared enrolled players.", ansi::WhiteInverse);
          break;
        }
        else if (lower1 == "enroll") {
          enrolled_players_.clear();
          auto clients = mmo_client_->get_client_list();
          for (const auto& [id, name] : clients) {
            if (id != mmo_client_->get_own_id().first && !bmmo::name_validator::is_spectator(name))
              enrolled_players_.push_back(name);
          }
          if (!mmo_client_->is_spectator())
            enrolled_players_.push_back(mmo_client_->get_own_id().second);
          std::ranges::sort(enrolled_players_, [](const std::string& a, const std::string& b) {
            return bmmo::string_utils::to_lower(a) < bmmo::string_utils::to_lower(b);
          });
          save_enrolled_players();
          ingame_msg(Sprintf("Enrolled %u online players.", enrolled_players_.size()), ansi::WhiteInverse);
          break;
        }
        else if (lower1 == "verify") {
          if (enrolled_players_.empty()) {
            ingame_msg("No enrolled players to verify.", ansi::Cyan);
            break;
          }
          verify_enrolled_players();
        }
        break;
      }
    }
  };

  const std::vector<std::string> on_tab_completion(const std::vector<std::string>& args) {
    switch (args.size()) {
      case 2:
        return { "clear", "enroll", "help", "verify" };
      default:
        return {};
    }
  };
};

IMod* BMLEntry(IBML* bml) {
  return new BMMOMatchRoster(bml);
}
