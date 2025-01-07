#include "bml_includes.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <algorithm> 
#include <cctype>
#include <locale>


class utils {
  IBML* bml_;

public:
  utils(IBML *bml): bml_(bml) {};

  // Windows 7 does not have GetDpiForSystem
  typedef UINT(WINAPI* GetDpiForSystemPtr) (void);
  GetDpiForSystemPtr const get_system_dpi = [] {
    auto hMod = GetModuleHandleW(L"user32.dll");
    if (hMod) {
      return (GetDpiForSystemPtr)GetProcAddress(hMod, "GetDpiForSystem");
    }
    return (GetDpiForSystemPtr)nullptr;
  }();

  int get_bgui_font_size(float size) {
    return (int)std::round(bml_->GetRenderContext()->GetHeight() / (768.0f / 119) * size / ((get_system_dpi == nullptr) ? 96 : get_system_dpi()));
  }

  CKGroup* get_sector_group(int sector) {
    char sector_name[12];
    std::snprintf(sector_name, sizeof(sector_name), "Sector_%02d", sector);
    auto* group = bml_->GetGroupByName(sector_name);
    if (!group && sector == 9)
      return bml_->GetGroupByName("Sector_9");
    return group;
  }

  // trim from start (in place)
  static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
      return !std::isspace(ch);
    }));
  }

  // trim from end (in place)
  static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
      return !std::isspace(ch);
    }).base(), s.end());
  }

  // trim from both ends (in place)
  static inline void trim(std::string& s) {
    rtrim(s);
    ltrim(s);
  }

  static inline std::string join_strings(const char** strings, size_t length, const char* delim = " ") {
    std::string s;
    for (size_t i = 0; i < length; i++)
      s.append(strings[i]).append(delim);
    s.erase(s.length() - std::strlen(delim));
    return s;
  }

  static inline std::string wide_to_ansi(const std::wstring& wstr) {
    int count = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
    std::string str(count, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
    return str;
  }

  static inline std::wstring utf8_to_wide(const std::string& str) {
    int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstr[0], count);
    return wstr;
  }

  static inline std::string utf8_to_ansi(const std::string& str) {
    return wide_to_ansi(utf8_to_wide(str));
  }

  static inline std::string to_lower(std::string data) {
    std::transform(data.begin(), data.end(), data.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return data;
  }
};
