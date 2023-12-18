#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#include <Windows.h>

enum TextName {
  ResolutionNotice, CurrentResolution,
  ConfigIssueDetected,
};

class translator {
private:
  __declspec(noinline) static const char* zhCN(TextName key) {
    switch (key) {
      case ResolutionNotice:
        return "你只能以 4:3 的分辨率比例进入服务器。";
      case CurrentResolution:
        return "当前屏幕分辨率: %dx%d (%d:%d)。";
      case ConfigIssueDetected:
        return "BMMO 反作弊 - 检测到配置问题";
      default:
        return "";
    };
  };

  __declspec(noinline) static const char* en(TextName key) {
    switch (key) {
      case ResolutionNotice:
        return "You can only join the server with a resolution ratio of 4:3.";
      case CurrentResolution:
        return "Current screen resolution: %dx%d (%d:%d).";
      case ConfigIssueDetected:
        return "BMMO Anti-Cheat: Configuration Issue Detected";
      default:
        return "";
    };
  };

public:
  static std::string get(TextName key, bool translation = true) {
    switch (translation ? GetUserDefaultUILanguage() : 0) {
      // https://learn.microsoft.com/en-us/windows/win32/intl/language-identifiers
      case 0x0804: // zh-CN.
        return bmmo::string_utils::ConvertWideToANSI(bmmo::string_utils::ConvertUtf8ToWide(zhCN(key)));
      default:
        return en(key);
    };
  };
};
