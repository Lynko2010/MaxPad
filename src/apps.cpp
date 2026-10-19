#include "apps.h"
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <algorithm>
#include <vector>

#pragma comment(lib, "shell32.lib")

// 这些名字通常不是「应用」，是卸载入口 / 帮助文档
static bool IsJunk(const std::wstring& n)
{
    static const wchar_t* bad[] = {
        L"卸载", L"uninstall", L"readme", L"帮助", L"help", L"license",
        L"许可", L"更新", L"update", L"说明", L"documentation"
    };
    std::wstring low = n;
    for (auto& c : low) c = (wchar_t)towlower(c);
    for (auto b : bad) if (low.find(b) != std::wstring::npos) return true;
    return false;
}

static void ScanDir(const std::wstring& dir, std::vector<AppEntry>& out, std::vector<std::wstring>& seen)
{
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L".."))
                ScanDir(dir + L"\\" + fd.cFileName, out, seen);
            continue;
        }
        std::wstring file = fd.cFileName;
        if (file.size() < 5) continue;
        if (_wcsicmp(file.c_str() + file.size() - 4, L".lnk") != 0) continue;

        std::wstring name = file.substr(0, file.size() - 4);
        if (name.empty() || IsJunk(name)) continue;

        std::wstring low = name;
        for (auto& c : low) c = (wchar_t)towlower(c);
        if (std::find(seen.begin(), seen.end(), low) != seen.end()) continue;
        seen.push_back(low);

        AppEntry e;
        e.name = name;
        e.lnk  = dir + L"\\" + file;
        out.push_back(e);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

std::vector<AppEntry> ScanApps()
{
    std::vector<AppEntry> out;
    std::vector<std::wstring> seen;

    wchar_t buf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_COMMON_PROGRAMS, nullptr, 0, buf)))
        ScanDir(buf, out, seen);
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAMS, nullptr, 0, buf)))
        ScanDir(buf, out, seen);

    std::sort(out.begin(), out.end(), [](const AppEntry& a, const AppEntry& b) {
        return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
    });
    return out;
}

ID2D1Bitmap* HBmpToD2D(HBITMAP hbmp, ID2D1RenderTarget* rt)
{
    BITMAP bm = {};
    if (!GetObjectW(hbmp, sizeof(bm), &bm)) return nullptr;
    int w = bm.bmWidth, h = bm.bmHeight;
    if (w <= 0 || h <= 0) return nullptr;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = w;
    bi.bmiHeader.biHeight      = -h;      // 自上而下
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    std::vector<BYTE> pix((size_t)w * h * 4, 0);
    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    // 注意：GetDIBits 要求位图未被选入 DC
    GetDIBits(mem, hbmp, 0, (UINT)h, pix.data(), &bi, DIB_RGB_COLORS);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);

    // BGRA → 预乘 alpha
    for (size_t i = 0; i < (size_t)w * h; i++) {
        BYTE b = pix[i * 4 + 0], g = pix[i * 4 + 1], r = pix[i * 4 + 2], a = pix[i * 4 + 3];
        pix[i * 4 + 0] = (BYTE)(b * a / 255);
        pix[i * 4 + 1] = (BYTE)(g * a / 255);
        pix[i * 4 + 2] = (BYTE)(r * a / 255);
        pix[i * 4 + 3] = a;
    }

    D2D1_BITMAP_PROPERTIES bp = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    ID2D1Bitmap* out = nullptr;
    if (FAILED(rt->CreateBitmap(D2D1::SizeU(w, h), pix.data(), w * 4, bp, &out)))
        return nullptr;
    return out;
}

HBITMAP ExtractIconBitmap(const std::wstring& lnkPath)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    HBITMAP out = nullptr;
    IShellItemImageFactory* f = nullptr;
    if (SUCCEEDED(SHCreateItemFromParsingName(lnkPath.c_str(), nullptr,
            __uuidof(IShellItemImageFactory), (void**)&f))) {
        SIZE s = { 96, 96 };
        f->GetImage(s, SIIGBF_ICONONLY, &out);
        f->Release();
    }
    return out;
}

void LoadIcons(std::vector<AppEntry>& apps, ID2D1RenderTarget* rt)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    for (auto& a : apps) {
        IShellItemImageFactory* f = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(a.lnk.c_str(), nullptr,
                __uuidof(IShellItemImageFactory), (void**)&f))) {
            SIZE s = { 96, 96 };
            HBITMAP hb = nullptr;
            if (SUCCEEDED(f->GetImage(s, SIIGBF_ICONONLY, &hb)) && hb) {
                a.icon = HBmpToD2D(hb, rt);
                DeleteObject(hb);
            }
            f->Release();
        }
    }
}

bool LaunchApp(const std::wstring& path)
{
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
    sei.lpVerb       = L"open";
    sei.lpFile       = path.c_str();
    sei.nShow        = SW_SHOWNORMAL;
    return ShellExecuteExW(&sei) != FALSE;
}
