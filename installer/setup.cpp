// MaxPad 安装 / 卸载程序
// 界面：自定义安装路径 + 快捷方式勾选
// 命令行：/silent  /uninstall  /dir=<路径>
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <string>
#include <cstdio>
#include <cstdarg>

#define IDR_APP 2
#define APP_NAME L"MaxPad"
#define APP_VER  L"2.1.0"

#define IDC_PATH      1001
#define IDC_BROWSE    1002
#define IDC_DESKTOP   1003
#define IDC_STARTMENU 1004

static bool g_silent   = false;
static bool g_fromTemp = false;

// ---------------- 小工具 ----------------
static void Log(const wchar_t* fmt, ...)
{
    if (GetEnvironmentVariableW(L"MAXPAD_SETUP_LOG", nullptr, 0) == 0) return;
    wchar_t tmp[MAX_PATH] = L"";
    GetTempPathW(MAX_PATH, tmp);
    std::wstring path = std::wstring(tmp) + L"maxpad_setup.log";
    FILE* f = _wfopen(path.c_str(), L"a, ccs=UTF-8");
    if (!f) return;
    va_list ap; va_start(ap, fmt);
    vfwprintf(f, fmt, ap);
    va_end(ap);
    fwprintf(f, L"\n");
    fclose(f);
}

static bool MakeShortcut(const std::wstring& lnk, const std::wstring& target, const std::wstring& work)
{
    IShellLinkW* sl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&sl))))
        return false;
    sl->SetPath(target.c_str());
    sl->SetWorkingDirectory(work.c_str());
    sl->SetDescription(L"MaxPad - Windows 上的 macOS 风格启动台");
    sl->SetIconLocation(target.c_str(), 0);

    IPersistFile* pf = nullptr;
    bool ok = false;
    if (SUCCEEDED(sl->QueryInterface(IID_PPV_ARGS(&pf)))) {
        ok = SUCCEEDED(pf->Save(lnk.c_str(), TRUE));
        pf->Release();
    }
    sl->Release();
    return ok;
}

static std::wstring DefaultDir()
{
    wchar_t buf[MAX_PATH] = L"";
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, buf);
    return std::wstring(buf) + L"\\Programs\\" + APP_NAME;
}

static std::wstring DesktopLnk()
{
    wchar_t buf[MAX_PATH] = L"";
    SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, buf);
    return std::wstring(buf) + L"\\" + APP_NAME + L".lnk";
}
static std::wstring StartLnk()
{
    wchar_t buf[MAX_PATH] = L"";
    SHGetFolderPathW(nullptr, CSIDL_PROGRAMS, nullptr, 0, buf);
    return std::wstring(buf) + L"\\" + APP_NAME + L".lnk";
}

static bool GetInstallLocationFromReg(std::wstring& out)
{
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MaxPad",
            0, KEY_READ, &k) != ERROR_SUCCESS) return false;
    wchar_t buf[MAX_PATH] = L"";
    DWORD cb = sizeof(buf), type = 0;
    bool ok = (RegQueryValueExW(k, L"InstallLocation", nullptr, &type,
                                (BYTE*)buf, &cb) == ERROR_SUCCESS) && buf[0];
    RegCloseKey(k);
    if (ok) out = buf;
    return ok;
}

static bool ExtractApp(const std::wstring& dest)
{
    HRSRC hr = FindResourceW(nullptr, MAKEINTRESOURCEW(IDR_APP), RT_RCDATA);
    if (!hr) return false;
    HGLOBAL hg = LoadResource(nullptr, hr);
    if (!hg) return false;
    void*  p    = LockResource(hg);
    DWORD  size = SizeofResource(nullptr, hr);
    if (!p || !size) return false;

    HANDLE f = CreateFileW(dest.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    BOOL ok = WriteFile(f, p, size, &written, nullptr);
    CloseHandle(f);
    return ok && written == size;
}

static void WriteUninstallKeys(const std::wstring& dir)
{
    HKEY k = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MaxPad",
            0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS) return;

    std::wstring exe   = dir + L"\\MaxPad.exe";
    std::wstring setup = dir + L"\\MaxPadSetup.exe";
    std::wstring un    = L"\"" + setup + L"\" /uninstall";

    auto set = [&](const wchar_t* name, const std::wstring& v) {
        RegSetValueExW(k, name, 0, REG_SZ, (const BYTE*)v.c_str(),
                       (DWORD)((v.size() + 1) * sizeof(wchar_t)));
    };
    set(L"DisplayName", L"MaxPad");
    set(L"DisplayVersion", APP_VER);
    set(L"Publisher", L"MaxPad");
    set(L"DisplayIcon", exe);
    set(L"InstallLocation", dir);
    set(L"UninstallString", un);
    set(L"QuietUninstallString", un);
    set(L"InstallSource", dir);
    DWORD one = 1;
    RegSetValueExW(k, L"NoModify", 0, REG_DWORD, (const BYTE*)&one, sizeof(one));
    RegSetValueExW(k, L"NoRepair", 0, REG_DWORD, (const BYTE*)&one, sizeof(one));
    RegCloseKey(k);
}

static void RemoveUninstallKeys()
{
    RegDeleteKeyW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MaxPad");
}

static void KillRunning()
{
    ShellExecuteW(nullptr, L"open", L"cmd.exe",
        L"/c taskkill /im MaxPad.exe /f >nul 2>&1", nullptr, SW_HIDE);
    Sleep(600);
}

// ---------------- 安装 ----------------
static bool InstallTo(const std::wstring& dir, bool desk, bool start, HWND owner)
{
    if (dir.empty()) return false;

    KillRunning();

    SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
    if (!CreateDirectoryW(dir.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        if (!PathIsDirectoryW(dir.c_str())) {
            MessageBoxW(owner, L"无法创建安装目录。", L"安装失败", MB_ICONERROR);
            return false;
        }
    }

    std::wstring exe = dir + L"\\MaxPad.exe";
    if (!ExtractApp(exe)) {
        MessageBoxW(owner, L"无法写入程序文件，请换一个目录或检查权限。",
                    L"安装失败", MB_ICONERROR);
        return false;
    }

    wchar_t self[MAX_PATH] = L"";
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring destSetup = dir + L"\\MaxPadSetup.exe";
    if (_wcsicmp(self, destSetup.c_str()) != 0) CopyFileW(self, destSetup.c_str(), FALSE);

    if (desk)  MakeShortcut(DesktopLnk(), exe, dir);
    if (start) MakeShortcut(StartLnk(),   exe, dir);
    WriteUninstallKeys(dir);

    Log(L"install dir=%ls desk=%d start=%d", dir.c_str(), (int)desk, (int)start);
    return true;
}

// ---------------- 对话框 ----------------
static INT_PTR CALLBACK InstallDlg(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    switch (m) {
    case WM_INITDIALOG: {
        SetDlgItemTextW(h, IDC_PATH, DefaultDir().c_str());
        CheckDlgButton(h, IDC_DESKTOP,   BST_CHECKED);
        CheckDlgButton(h, IDC_STARTMENU, BST_CHECKED);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BROWSE: {
            BROWSEINFOW bi = {};
            bi.hwndOwner = h;
            bi.lpszTitle = L"选择安装位置（会自动加上 MaxPad 子目录）";
            bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
            LPITEMIDLIST id = SHBrowseForFolderW(&bi);
            if (id) {
                wchar_t sel[MAX_PATH] = L"";
                if (SHGetPathFromIDListW(id, sel)) {
                    std::wstring p(sel);
                    bool endsWith = p.size() > 7 &&
                        _wcsicmp(p.c_str() + p.size() - 7, L"\\MaxPad") == 0;
                    if (!endsWith) p += L"\\MaxPad";
                    SetDlgItemTextW(h, IDC_PATH, p.c_str());
                }
                CoTaskMemFree(id);
            }
            return TRUE;
        }
        case IDOK: {
            wchar_t buf[MAX_PATH] = L"";
            GetDlgItemTextW(h, IDC_PATH, buf, MAX_PATH);
            std::wstring dir(buf);
            while (!dir.empty() && (dir.back() == L'\\' || dir.back() == L'/')) dir.pop_back();
            if (dir.empty()) {
                MessageBoxW(h, L"请填写安装位置。", L"MaxPad", MB_ICONWARNING);
                return TRUE;
            }
            bool desk  = IsDlgButtonChecked(h, IDC_DESKTOP)   == BST_CHECKED;
            bool start = IsDlgButtonChecked(h, IDC_STARTMENU) == BST_CHECKED;

            if (!InstallTo(dir, desk, start, h)) return TRUE;

            std::wstring sc = desk && start ? L"桌面 + 开始菜单"
                            : desk          ? L"桌面"
                            : start         ? L"开始菜单" : L"未创建";
            std::wstring msg = L"安装完成。\n\n位置：" + dir +
                L"\n快捷方式：" + sc + L"\n\n现在启动 MaxPad 吗？";
            if (MessageBoxW(h, msg.c_str(), L"安装成功",
                            MB_ICONINFORMATION | MB_YESNO) == IDYES) {
                std::wstring exe = dir + L"\\MaxPad.exe";
                ShellExecuteW(nullptr, L"open", exe.c_str(), nullptr, dir.c_str(), SW_SHOWNORMAL);
            }
            EndDialog(h, 1);
            return TRUE;
        }
        case IDC_DESKTOP:
        case IDC_STARTMENU:
            // windres 把 CHECKBOX 编成 BS_CHECKBOX（手动模式），不会自己切换，
            // 这里手动翻转一次，效果等同 BS_AUTOCHECKBOX
            if (HIWORD(wp) == BN_CLICKED) {
                HWND b = (HWND)lp;
                UINT st = (UINT)SendMessageW(b, BM_GETCHECK, 0, 0);
                SendMessageW(b, BM_SETCHECK, st == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED, 0);
            }
            return TRUE;

        case IDCANCEL:
            EndDialog(h, 0);
            return TRUE;
        }
    }
    return FALSE;
}

// ---------------- 卸载 ----------------
static bool DoUninstall()
{
    std::wstring dir;
    if (!GetInstallLocationFromReg(dir)) {
        wchar_t self[MAX_PATH] = L"";
        GetModuleFileNameW(nullptr, self, MAX_PATH);
        std::wstring s(self);
        size_t sl = s.find_last_of(L'\\');
        dir = (sl == std::wstring::npos) ? L"" : s.substr(0, sl);
    }

    if (!g_fromTemp) {
        if (!g_silent) {
            std::wstring msg = L"确定要卸载 MaxPad 吗？\n\n将从这里删除：\n  " + dir +
                L"\n\n并移除快捷方式和卸载信息。";
            if (MessageBoxW(nullptr, msg.c_str(), L"卸载 MaxPad",
                            MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) != IDYES)
                return false;
        }
        wchar_t tmp[MAX_PATH] = L"";
        GetTempPathW(MAX_PATH, tmp);
        std::wstring tmpExe = std::wstring(tmp) + L"MaxPadUninst.exe";
        wchar_t self[MAX_PATH] = L"";
        GetModuleFileNameW(nullptr, self, MAX_PATH);
        Log(L"--- uninstall first pass, copy to %ls", tmpExe.c_str());
        if (CopyFileW(self, tmpExe.c_str(), FALSE)) {
            std::wstring args = g_silent ? L"/uninstall /silent /fromtemp"
                                         : L"/uninstall /fromtemp";
            ShellExecuteW(nullptr, L"open", tmpExe.c_str(), args.c_str(), tmp, SW_SHOWNORMAL);
            return true;
        }
    }

    Log(L"--- uninstall fromTemp=%d dir=%ls", (int)g_fromTemp, dir.c_str());
    KillRunning();
    DeleteFileW(DesktopLnk().c_str());
    DeleteFileW(StartLnk().c_str());
    RemoveUninstallKeys();

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
            std::wstring f = dir + L"\\" + fd.cFileName;
            SetFileAttributesW(f.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(f.c_str());
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }

    wchar_t buf[MAX_PATH * 3];
    swprintf(buf, MAX_PATH * 3, L"/c rmdir /s /q \"%ls\"", dir.c_str());
    wchar_t tmp2[MAX_PATH] = L"";
    GetTempPathW(MAX_PATH, tmp2);
    ShellExecuteW(nullptr, L"open", L"cmd.exe", buf, tmp2, SW_HIDE);
    Log(L"rmdir %ls", buf);

    if (!g_silent)
        MessageBoxW(nullptr, L"卸载完成。\n\n文件和快捷方式已删除。", L"MaxPad",
                    MB_ICONINFORMATION | MB_OK);

    wchar_t self2[MAX_PATH] = L"";
    GetModuleFileNameW(nullptr, self2, MAX_PATH);
    MoveFileExW(self2, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
    return true;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR cmdLine, int)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    _wsetlocale(LC_ALL, L"");
    SetProcessDpiAwarenessContext((DPI_AWARENESS_CONTEXT)-4);

    if (cmdLine && wcsstr(cmdLine, L"/silent"))   g_silent   = true;
    if (cmdLine && wcsstr(cmdLine, L"/fromtemp")) g_fromTemp = true;

    if (cmdLine && wcsstr(cmdLine, L"/uninstall")) {
        DoUninstall();
    } else if (g_silent) {
        std::wstring dir = DefaultDir();
        if (cmdLine) {
            const wchar_t* d = wcsstr(cmdLine, L"/dir=");
            if (d) {
                std::wstring p(d + 5);
                if (p.size() >= 2 && p.front() == L'"' && p.back() == L'"')
                    p = p.substr(1, p.size() - 2);
                if (!p.empty()) dir = p;
            }
        }
        bool ok = InstallTo(dir, true, true, nullptr);
        Log(L"silent install ok=%d dir=%ls", (int)ok, dir.c_str());
    } else {
        DialogBoxParamW(hInst, MAKEINTRESOURCEW(100), nullptr, InstallDlg, 0);
    }

    CoUninitialize();
    return 0;
}
