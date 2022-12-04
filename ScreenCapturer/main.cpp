#include <chrono>

#define ASIO_NO_NOMINMAX
#include <asio/post.hpp>
#include <asio/thread_pool.hpp>

#include <BMLPlus/BMLAll.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>

typedef const char* C_CKSTRING;

extern "C" {
	__declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

inline std::wstring ConvertAnsiToWide(const std::string& str) {
  int count = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), NULL, 0);
  std::wstring wstr(count, 0);
  MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), wstr.data(), count);
  return wstr;
}

BOOL FileExists(LPCTSTR szPath) {
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
          !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

class ScreenCapturer : public IMod {
private:
	Gdiplus::GdiplusStartupInput gdiplus_startup_input;
	ULONG_PTR gdiplus_token;
  asio::thread_pool pool = asio::thread_pool(4);
  CLSID png_clsid, jpeg_clsid;
  IProperty *prop_key{}, *prop_notify{}, *prop_jpeg{}, *prop_copy{}, *prop_copy_only{};
  CKKEYBOARD screenshot_key = CKKEY_F8;
  bool notify = true, save_as_jpeg = false, copy_to_clipboard = false, copy_only = false;
  std::string save_extension = ".png";

  int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
      return -1;  // Failure

    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
      return -1;  // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
      if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
        *pClsid = pImageCodecInfo[j].Clsid;
        free(pImageCodecInfo);
        return j;  // Success
      }
    }

    free(pImageCodecInfo);
    return -1;  // Failure
  }

  void load_config_values() {
    screenshot_key = prop_key->GetKey();
    notify = prop_notify->GetBoolean();
    save_as_jpeg = prop_jpeg->GetBoolean();
    save_extension = save_as_jpeg ? ".jpg" : ".png";
    copy_to_clipboard = prop_copy->GetBoolean();
    copy_only = prop_copy_only->GetBoolean();
  }


public:
	ScreenCapturer(IBML* bml) : IMod(bml) {}

	virtual C_CKSTRING GetID() override { return "ScreenCapturer"; }
	virtual C_CKSTRING GetVersion() override { return "0.0.1"; }
	virtual C_CKSTRING GetName() override { return "Screen Capturer"; }
	virtual C_CKSTRING GetAuthor() override { return "BallanceBug"; }
	virtual C_CKSTRING GetDescription() override { return "Take and save screenshots in-game."; }
	DECLARE_BML_VERSION;

	void OnLoad() override {
    CreateDirectory("..\\Screenshots", NULL);
		Gdiplus::GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, nullptr);
    GetEncoderClsid(L"image/png", &png_clsid);
    GetEncoderClsid(L"image/jpeg", &jpeg_clsid);

    auto *config = GetConfig();
    config->SetCategoryComment("Action", "Settings related to triggering the Screen Capturer.");
    prop_key = config->GetProperty("Action", "KeyBinding");
    prop_key->SetDefaultKey(CKKEY_F8);
    prop_key->SetComment("Which key to trigger screenshotting?");
    prop_notify = config->GetProperty("Action", "Notify");
    prop_notify->SetDefaultBoolean(true);
    prop_notify->SetComment("Whether to notify the action of screenshotting using in-game messages.");
    config->SetCategoryComment("Saving", "Settings related to saving screenshot files.");
    prop_jpeg = config->GetProperty("Saving", "SaveAsJPEG");
    prop_jpeg->SetDefaultBoolean(false);
    prop_jpeg->SetComment("Whether to save screenshots in the default PNG (.png) format or the JPEG (.jpg) format.");
    prop_copy = config->GetProperty("Saving", "CopyToClipboard");
    prop_copy->SetDefaultBoolean(false);
    prop_copy->SetComment("Copy the screenshots to your clipboard in addition to saving them in the file system.");
    prop_copy_only = config->GetProperty("Saving", "ClipboardOnly");
    prop_copy_only->SetDefaultBoolean(false);
    prop_copy_only->SetComment("Only copy screenshots to the clipboard without saving them on the disk. Requires CopyToClipboard to be true;");

    load_config_values();
	}

	void OnUnload() override {
    pool.join();
		Gdiplus::GdiplusShutdown(gdiplus_token);
	}

  void OnModifyConfig(C_CKSTRING category, C_CKSTRING key, IProperty* prop) override {
    load_config_values();
  }

	void OnProcess() override {
		if (m_bml->GetInputManager()->IsKeyPressed(screenshot_key)) {
			auto handle = static_cast<HWND>(m_bml->GetCKContext()->GetMainWindow());
      VxRect rect; m_bml->GetRenderContext()->GetWindowRect(rect);
      int width = rect.GetWidth();
      int height = rect.GetHeight();
      RECT window_rect; GetClientRect(handle, &window_rect);
      if (window_rect.right - window_rect.left < width || window_rect.bottom - window_rect.top < height) {
        window_rect.right = window_rect.left + width;
        window_rect.bottom = window_rect.top + height;
        AdjustWindowRectEx(&window_rect, GetWindowLongA(handle, GWL_STYLE), false, GetWindowLongA(handle, GWL_EXSTYLE));
        SetWindowPos(handle, handle, 0, 0, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, SWP_NOMOVE | SWP_NOZORDER);
      }

      asio::post(pool, [=, this] {
        HDC hdcScreen = GetDC(handle);
        HDC hdc = CreateCompatibleDC(hdcScreen);
        HBITMAP hbmp = CreateCompatibleBitmap(hdcScreen, width, height);
        HGDIOBJ old_obj = SelectObject(hdc, hbmp);

        BitBlt(hdc, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);

        if (copy_to_clipboard) {
          OpenClipboard(NULL);
          EmptyClipboard();
          SetClipboardData(CF_BITMAP, hbmp);
          CloseClipboard();
        }

        std::string filename(64, 0);
        if (!(copy_to_clipboard && copy_only)) {
          Gdiplus::Bitmap bitmap(hbmp, NULL);
          auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
          filename.resize(std::strftime(filename.data(), filename.size(), "..\\Screenshots\\%Y-%m-%d_%H-%M-%S", std::localtime(&current_time)));
          while (FileExists((filename + save_extension).c_str()))
            filename.append("_");
          bitmap.Save(ConvertAnsiToWide(filename + save_extension).c_str(),
                      save_as_jpeg ? &jpeg_clsid : &png_clsid);
        }

        std::string notify_msg = "Saved screenshot as ";
        if (copy_to_clipboard && copy_only) {
          notify_msg = "Copied screenshot to the clipboard";
        }
        else {
          filename.erase(0, sizeof("..\\Screenshots\\") - 1);
          notify_msg.append(filename + save_extension);
          if (copy_to_clipboard) notify_msg.append(" and copied to the clipboard");
        }

        if (notify) {
          m_bml->SendIngameMessage(notify_msg.c_str());
        }
        else {
          GetLogger()->Info("%s", notify_msg.c_str());
        }

        SelectObject(hdc, old_obj);
        DeleteDC(hdc);
        DeleteObject(hbmp);
        ReleaseDC(handle, hdcScreen);
      });
		}
	}
};

IMod* BMLEntry(IBML* bml) {
	return new ScreenCapturer(bml);
}
