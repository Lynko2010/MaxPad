// MaxPad —— Windows 上的 macOS 风格启动台
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include "apps.h"
#include "config.h"

using namespace D2D1;

static const int   COLS = 7, ROWS = 5, PER_PAGE = COLS * ROWS;
static const float CELL_W = 180, CELL_H = 186, GRID_TOP = 196;
static const float SEARCH_W = 440, SEARCH_H = 44, SEARCH_Y = 86, GEAR_D = 44;
static const float CARD_W = 900, CARD_H = 620, NAV_W = 190, ROW_H = 46;

#define WCA_ACCENT_POLICY 19
#define ACCENT_ENABLE_BLURBEHIND        3
#define ACCENT_ENABLE_ACRYLICBLURBEHIND 4
#define WM_TRAY (WM_APP + 2)

struct ACCENTPOLICY { int nAccentState; DWORD nFlags; DWORD nColor; DWORD nAnimationId; };
struct WINCOMPATTRDATA { int nAttribute; PVOID pData; ULONG ulDataSize; };

struct Theme { D2D1_COLOR_F base, card, cardLine, text, textDim, hover, sel, accent; DWORD accentColor; };
static Theme g_dark  = { {0.06f,0.06f,0.08f,1}, {0.11f,0.11f,0.13f,1}, {1,1,1,0.10f}, {1,1,1,0.94f}, {1,1,1,0.50f},
                         {1,1,1,0.09f}, {1,1,1,0.16f}, {0.30f,0.55f,1.0f,1.0f}, 0x99241C1C };
static Theme g_light = { {0.96f,0.96f,0.97f,1}, {0.98f,0.98f,0.99f,1}, {0,0,0,0.10f}, {0.09f,0.09f,0.11f,0.95f}, {0.09f,0.09f,0.11f,0.55f},
                         {0,0,0,0.05f}, {0,0,0,0.09f}, {0.00f,0.42f,0.95f,1.0f}, 0xC8F2F1F6 };
static Theme* g_theme = &g_dark;

struct Tile { std::wstring name, path; ID2D1Bitmap* icon; IDWriteTextLayout* layout = nullptr; float layoutW = 0; };
struct Row  { std::wstring name, path; bool custom; bool added; };
enum HitKind { HK_GEAR=1, HK_TAB, HK_THEME, HK_STYLE, HK_LANG, HK_ROW, HK_ADDPATH, HK_CLOSE, HK_SLIDER, HK_AUTOSTART, HK_ANIM, HK_LANG_BTN, HK_MORE, HK_MENU_REMOVE, HK_MENU_RENAME, HK_SEARCH, HK_RENAME_BOX };
struct Hot { D2D1_RECT_F r; int kind; int id; bool added; };

static HWND g_hwnd = nullptr;
static ID2D1Factory* g_fac = nullptr;
static ID2D1HwndRenderTarget* g_rt = nullptr;
static IDWriteFactory* g_dw = nullptr;
static IDWriteTextFormat *g_tfName=nullptr,*g_tfCap=nullptr,*g_tfSearch=nullptr,*g_tfTitle=nullptr,*g_tfBody=nullptr;

static std::vector<AppEntry> g_apps;
static std::vector<Tile> g_tiles;
static std::vector<Row>  g_rows;
static std::vector<int>  g_view;
static std::vector<Hot>  g_hot;
static std::vector<std::pair<std::wstring, ID2D1Bitmap*>> g_iconCache;

static std::wstring g_query, g_dir;
static int   g_page = 0, g_hover = -1, g_tab = 0, g_dragSlider = -1;
static float g_scale = 1.0f, g_scroll = 0;
static bool  g_settingsOpen = false, g_trayAdded = false;
static Settings g_set;
static HICON g_trayIcon = nullptr;
static ULONGLONG g_animStart = 0;
static bool g_animActive = false;
static int  g_animDir = 0;            // 1=进场 2=退场
static ULONGLONG g_setAnimStart = 0;
static bool g_setAnimActive = false;
static int  g_setAnimDir = 0;         // 1=打开 2=关闭
static ID2D1Layer* g_layer = nullptr;
static ID2D1Layer* g_layer2 = nullptr;
static bool g_langOpen = false;   // 语言下拉是否展开
static bool g_icoDirty = true;     // 图标需要后台重扫
static int  g_menuRow = -1;         // 打开三点菜单的行
static int  g_menuDir = 0;          // 1=开 2=关
static ULONGLONG g_menuT0 = 0;
static float g_menuAnim = 0;
static D2D1_RECT_F g_menuAnchor = {};
static int  g_renameRow = -1;       // 正在重命名的行
static std::wstring g_renameBuf;
static D2D1_RECT_F g_sbTrack = {}, g_sbThumb = {};
static float g_sbListTop = 0, g_sbListH = 0, g_sbMax = 0, g_sbThumbH = 0;
static bool  g_sbDrag = false;
static float g_sbGrab = 0;
static float g_langA = 0;
static int   g_langDir = 0;
static ULONGLONG g_langT0 = 0;
static int  g_slideFrom = 0, g_slideDir = 0;
static ULONGLONG g_slideStart = 0;
static bool g_slideActive = false;

// 前向声明（HandleHotClick / ShowPad 用到，定义在后面）
static void ShowPad();
static void HidePad();
static void HidePadNow();
static void StartPadAnim(int dir);
static void StartSetAnim(int dir);
static void OpenSettings();
static void CloseSettings();

static float S(float v){ return v * g_scale; }
static float FS(){ return g_set.fontSize / 100.0f; }            // 字号倍率
static float ICON(){ return S((float)g_set.iconSize); }         // 图标边长
static float ALPHA(){ return g_set.opacity / 100.0f; }          // 控件不透明度
static float BGA()  { float v = g_set.bgOpacity / 100.0f; return powf(v, 1.4f); }   // 背景不透明度（低端更透）
static float g_styleFrom = 1.0f, g_styleTo = 1.0f;
static ULONGLONG g_styleT0 = 0;
static bool g_styleRun = false;
// 0=经典 1=液态，切换时插值（受过渡动画开关控制）
static float StyleT()
{
    if (!g_styleRun) return (g_set.style == 1) ? 1.0f : 0.0f;
    float t = (float)(GetTickCount64() - g_styleT0) / 200.0f;
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    float e = 1.0f - powf(1.0f - t, 3.0f);
    return g_styleFrom + (g_styleTo - g_styleFrom) * e;
}
static bool  Liquid(){ return g_set.style == 1; }

static ID2D1SolidColorBrush* Br(D2D1_COLOR_F c){ ID2D1SolidColorBrush* b=nullptr; g_rt->CreateSolidColorBrush(c,&b); return b; }
// 主题感知的前景色（alpha 可调）
static D2D1_COLOR_F Fg(float a){ D2D1_COLOR_F c = g_theme->text; c.a = a; return c; }

static void Text(const std::wstring& s, IDWriteTextFormat* f, const D2D1_RECT_F& box,
                 ID2D1Brush* br, DWRITE_TEXT_ALIGNMENT a = DWRITE_TEXT_ALIGNMENT_CENTER)
{
    if (!f) return;
    f->SetTextAlignment(a);
    IDWriteTextLayout* lay = nullptr;
    g_dw->CreateTextLayout(s.c_str(), (UINT32)s.size(), f, box.right-box.left, box.bottom-box.top, &lay);
    if (!lay) return;
    DWRITE_TRIMMING tr = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
    IDWriteInlineObject* el = nullptr;
    g_dw->CreateEllipsisTrimmingSign(f, &el);
    lay->SetTrimming(&tr, el);
    g_rt->DrawTextLayout(Point2F(box.left, box.top), lay, br);
    if (el) el->Release();
    lay->Release();
}
static void TextC(const wchar_t* s, IDWriteTextFormat* f, const D2D1_RECT_F& box, ID2D1Brush* br,
                  DWRITE_TEXT_ALIGNMENT a = DWRITE_TEXT_ALIGNMENT_CENTER)
{ Text(std::wstring(s), f, box, br, a); }

// 量文本宽度（用于定位光标）
static float TextWidth(const std::wstring& s, IDWriteTextFormat* f)
{
    if (s.empty() || !f) return 0.0f;
    IDWriteTextLayout* lay = nullptr;
    g_dw->CreateTextLayout(s.c_str(), (UINT32)s.size(), f, 4000.0f, 100.0f, &lay);
    if (!lay) return 0.0f;
    DWRITE_TEXT_METRICS m = {};
    lay->GetMetrics(&m);
    float w = m.widthIncludingTrailingWhitespace;
    lay->Release();
    return w;
}

// 光标此刻是否该显示（按系统闪烁周期）
static bool CaretOn()
{
    DWORD bt = GetCaretBlinkTime();
    if (bt == 0 || bt == INFINITE) return true;
    return (((GetTickCount64() / bt) & 1) == 0);
}

static void MakeFormats()
{
    if (g_tfName)   { g_tfName->Release();   g_tfName=nullptr; }
    if (g_tfCap)    { g_tfCap->Release();    g_tfCap=nullptr; }
    if (g_tfSearch) { g_tfSearch->Release(); g_tfSearch=nullptr; }
    if (g_tfTitle)  { g_tfTitle->Release();  g_tfTitle=nullptr; }
    if (g_tfBody)   { g_tfBody->Release();   g_tfBody=nullptr; }
    for (auto& t : g_tiles) { if (t.layout) { t.layout->Release(); t.layout = nullptr; t.layoutW = 0; } }
    const wchar_t* fam = FontForLang(g_set.lang);
    float k = FS();
    g_dw->CreateTextFormat(fam,nullptr,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,15.0f*k,L"",&g_tfName);
    g_dw->CreateTextFormat(fam,nullptr,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,12.0f*k,L"",&g_tfCap);
    g_dw->CreateTextFormat(fam,nullptr,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,19.0f*k,L"",&g_tfSearch);
    g_dw->CreateTextFormat(fam,nullptr,DWRITE_FONT_WEIGHT_BOLD,  DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,22.0f*k,L"",&g_tfTitle);
    g_dw->CreateTextFormat(fam,nullptr,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,15.0f*k,L"",&g_tfBody);
}

static void ApplyTheme()
{
    bool light = (g_set.theme == THEME_LIGHT) || (g_set.theme == THEME_SYSTEM && SystemUsesLightTheme());
    g_theme = light ? &g_light : &g_dark;
    typedef BOOL(WINAPI* pSWCA)(HWND, WINCOMPATTRDATA*);
    static pSWCA swca = (pSWCA)GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute");
    if (!swca) return;
    // 经典模式：完全不透明，不需要模糊
    DWORD col = Liquid() ? g_theme->accentColor : ((g_set.theme == THEME_LIGHT || (g_set.theme==THEME_SYSTEM && SystemUsesLightTheme())) ? 0xFFF5F4F8 : 0xFF111114);
    if (Liquid()) {
        DWORD a0 = (col >> 24) & 0xFF;
        DWORD a1 = (DWORD)((float)a0 * BGA());
        col = (a1 << 24) | (col & 0x00FFFFFF);
    }
    ACCENTPOLICY ap = { ACCENT_ENABLE_ACRYLICBLURBEHIND, 0, col, 0 };
    WINCOMPATTRDATA d = { WCA_ACCENT_POLICY, &ap, sizeof(ap) };

    if (!swca(g_hwnd, &d)) { ap.nAccentState = ACCENT_ENABLE_BLURBEHIND; swca(g_hwnd, &d); }
}

static bool IsHidden(const std::wstring& n)
{
    for (auto& h : g_set.hidden) if (_wcsicmp(h.c_str(), n.c_str()) == 0) return true;
    return false;
}

static std::wstring NameOverride(const std::wstring& path)
{
    for (auto& r : g_set.renamed) if (r.first == path) return r.second;
    return L"";
}
static std::wstring OriginalName(const std::wstring& path)
{
    for (auto& a : g_apps) if (a.lnk == path) return a.name;
    for (auto& c : g_set.custom) if (c.path == path) return c.name;
    return L"";
}
static bool IsRemovedName(const std::wstring& orig)
{
    for (auto& r : g_set.removed) if (_wcsicmp(r.c_str(), orig.c_str()) == 0) return true;
    return false;
}

static void RebuildRows()
{
    g_rows.clear();
    for (auto& a : g_apps) {
        if (IsRemovedName(a.name)) continue;
        Row r; r.path = a.lnk;
        std::wstring ov = NameOverride(a.lnk);
        r.name = ov.empty() ? a.name : ov;
        r.custom = false; r.added = !IsHidden(a.name);
        g_rows.push_back(r);
    }
    for (auto& c : g_set.custom) {
        Row r; r.path = c.path;
        std::wstring ov = NameOverride(c.path);
        r.name = ov.empty() ? c.name : ov;
        r.custom = true; r.added = c.enabled;
        g_rows.push_back(r);
    }
}

static ID2D1Bitmap* CacheGet(const std::wstring& p)
{
    for (auto& kv : g_iconCache) if (kv.first == p) return kv.second;
    return nullptr;
}

static void RebuildTiles()
{
    for (auto& t : g_tiles) { if (t.layout) { t.layout->Release(); t.layout = nullptr; t.layoutW = 0; } }
    g_tiles.clear();
    g_icoDirty = true;
    for (auto& r : g_rows) {
        if (!r.added) continue;
        Tile t; t.name = r.name; t.path = r.path; t.icon = CacheGet(r.path);
        g_tiles.push_back(t);
    }
}

// ---------------- 后台图标预加载 ----------------
struct ReadyIcon { std::wstring path; HBITMAP hb; };
static CRITICAL_SECTION g_icoCS;
static std::vector<ReadyIcon> g_icoReady;
static volatile LONG g_icoRunning = 0;

static DWORD WINAPI IconWorker(LPVOID p)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    auto* jobs = (std::vector<std::wstring>*)p;
    for (auto& path : *jobs) {
        HBITMAP hb = ExtractIconBitmap(path);
        if (hb) {
            EnterCriticalSection(&g_icoCS);
            g_icoReady.push_back({ path, hb });
            LeaveCriticalSection(&g_icoCS);
            PostMessageW(g_hwnd, WM_APP + 1, 0, 0);
        }
    }
    delete jobs;
    InterlockedExchange(&g_icoRunning, 0);
    CoUninitialize();
    return 0;
}

static void StartIconWorker()
{
    if (g_icoRunning) return;
    auto* jobs = new std::vector<std::wstring>();
    for (auto& t : g_tiles) if (!t.icon) jobs->push_back(t.path);
    if (jobs->empty()) { delete jobs; return; }
    InterlockedExchange(&g_icoRunning, 1);
    HANDLE h = CreateThread(nullptr, 0, IconWorker, jobs, 0, nullptr);
    if (h) CloseHandle(h);
    else { InterlockedExchange(&g_icoRunning, 0); delete jobs; }
}

static void DrainIcons()
{
    std::vector<ReadyIcon> got;
    EnterCriticalSection(&g_icoCS);
    got.swap(g_icoReady);
    LeaveCriticalSection(&g_icoCS);
    if (got.empty()) return;
    for (auto& r : got) {
        ID2D1Bitmap* bmp = HBmpToD2D(r.hb, g_rt);
        DeleteObject(r.hb);
        if (!bmp) continue;
        for (auto& t : g_tiles) if (!t.icon && t.path == r.path) t.icon = bmp;
        g_iconCache.push_back({ r.path, bmp });
    }
    if (g_hwnd) InvalidateRect(g_hwnd, nullptr, FALSE);
}

// 图标交给后台线程补齐，这里只负责启动一次
static void EnsureIconsForPage(int)
{
    if (g_icoDirty) { g_icoDirty = false; StartIconWorker(); }
}

static int TotalPages()
{
    int n = (int)g_view.size();
    return n <= 0 ? 1 : (n + PER_PAGE - 1) / PER_PAGE;
}

static void RebuildView()
{
    g_view.clear();
    std::wstring q = g_query;
    for (auto& c : q) c = (wchar_t)towlower(c);
    for (int i = 0; i < (int)g_tiles.size(); i++) {
        if (q.empty()) { g_view.push_back(i); continue; }
        std::wstring n = g_tiles[i].name;
        for (auto& c : n) c = (wchar_t)towlower(c);
        if (n.find(q) != std::wstring::npos) g_view.push_back(i);
    }
    if (g_page >= TotalPages()) g_page = TotalPages() - 1;
    if (g_page < 0) g_page = 0;
    g_hover = -1;
}

// ---------------- 开机自启动（启动文件夹快捷方式） ----------------
static std::wstring AutostartLnk()
{
    wchar_t buf[MAX_PATH] = L"";
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, 0, buf))) return L"";
    return std::wstring(buf) + L"\\MaxPad.lnk";
}
static bool IsAutostart()
{
    std::wstring p = AutostartLnk();
    return !p.empty() && GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES;
}
static void SetAutostart(bool on)
{
    std::wstring lnk = AutostartLnk();
    if (lnk.empty()) return;
    if (!on) { DeleteFileW(lnk.c_str()); return; }
    IShellLinkW* sl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&sl)))) return;
    sl->SetPath((g_dir + L"\\MaxPad.exe").c_str());
    sl->SetWorkingDirectory(g_dir.c_str());
    sl->SetDescription(L"MaxPad launcher");
    IPersistFile* pf = nullptr;
    if (SUCCEEDED(sl->QueryInterface(IID_PPV_ARGS(&pf)))) { pf->Save(lnk.c_str(), TRUE); pf->Release(); }
    sl->Release();
}

static void Commit()
{
    SaveSettings(g_set, g_dir);
    SetCurrentLang(g_set.lang);
    ApplyTheme();
    MakeFormats();
    RebuildRows();
    RebuildTiles();
    RebuildView();
    if (g_hwnd) InvalidateRect(g_hwnd, nullptr, FALSE);
}

// 格子尺寸随窗口自适应，保证 5 行永远放得下
static float CellW(float w){ return std::min(S(CELL_W), (w - S(32)) / COLS); }
static float CellH(float h){ return std::min(S(CELL_H), (h - S(GRID_TOP) - S(56)) / ROWS); }

static D2D1_POINT_2F CellOrigin(int slot, float w, float h)
{
    int col = slot % COLS, row = slot / COLS;
    float cw = CellW(w), chh = CellH(h);
    float x0 = (w - COLS * cw) * 0.5f;
    return Point2F(x0 + col * cw, S(GRID_TOP) + row * chh);
}

static int HitCell(float mx, float my, float w, float h)
{
    if (g_slideActive) return -1;
    int base = g_page * PER_PAGE;
    for (int s = 0; s < PER_PAGE && base + s < (int)g_view.size(); s++) {
        D2D1_POINT_2F o = CellOrigin(s, w, h);
        if (mx>=o.x && mx<=o.x+S(CELL_W) && my>=o.y && my<=o.y+S(CELL_H)) return s;
    }
    return -1;
}

static void DrawGear(float cx, float cy, float r, ID2D1Brush* br)
{
    for (int i = 0; i < 8; i++) {
        float a = (float)i * 3.14159265f / 4.0f;
        g_rt->DrawLine(Point2F(cx+cosf(a)*r*0.60f, cy+sinf(a)*r*0.60f),
                       Point2F(cx+cosf(a)*r,        cy+sinf(a)*r),        br, S(2.4f));
    }
    g_rt->DrawEllipse(Ellipse(Point2F(cx,cy), r*0.48f, r*0.48f), br, S(2.2f));
    g_rt->FillEllipse(Ellipse(Point2F(cx,cy), r*0.16f, r*0.16f), br);
}

// ---------------- 玻璃卡片 ----------------
static D2D1_RECT_F g_sliderRect[4];

static void DrawBackdrop(float W, float H)
{
    float bk = BGA();                        // 背景不透明度（独立于控件）
    float st2 = StyleT();
    bk = 1.0f - st2 * (1.0f - bk);           // 经典=1（纯色），液态=bk，切换时平滑过渡
    auto adj = [](float v, float d) { float r = v + d; return r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r); };
    D2D1_COLOR_F cTop = ColorF(adj(g_theme->base.r, 0.03f), adj(g_theme->base.g, 0.03f), adj(g_theme->base.b, 0.03f), bk);
    D2D1_COLOR_F cBot = ColorF(adj(g_theme->base.r, -0.03f), adj(g_theme->base.g, -0.03f), adj(g_theme->base.b, -0.03f), bk);
    {
        D2D1_GRADIENT_STOP gs[2] = { {0.0f, cTop}, {1.0f, cBot} };
        ID2D1GradientStopCollection* col = nullptr;
        g_rt->CreateGradientStopCollection(gs, 2, &col);
        ID2D1LinearGradientBrush* lg = nullptr;
        g_rt->CreateLinearGradientBrush(LinearGradientBrushProperties(Point2F(0,0), Point2F(0,H)), col, &lg);
        g_rt->FillRectangle(RectF(0,0,W,H), lg);
        lg->Release(); col->Release();
    }
}

static void DrawGlassCard(const D2D1_RECT_F& r, float rad)
{
    D2D1_ROUNDED_RECT rr = RoundedRect(r, rad, rad);
    if (Liquid()) {
        D2D1_COLOR_F c = g_theme->card; c.a = 0.94f * ALPHA();
        ID2D1SolidColorBrush* b = Br(c);
        g_rt->FillRoundedRectangle(rr, b); b->Release();
        D2D1_GRADIENT_STOP gs[3] = {
            {0.00f, Fg(0.18f)},
            {0.35f, Fg(0.03f)},
            {1.00f, Fg(0.00f)},
        };
        ID2D1GradientStopCollection* col = nullptr;
        g_rt->CreateGradientStopCollection(gs, 3, &col);
        ID2D1LinearGradientBrush* lg = nullptr;
        g_rt->CreateLinearGradientBrush(LinearGradientBrushProperties(Point2F(0,r.top), Point2F(0,r.bottom)), col, &lg);
        g_rt->FillRoundedRectangle(rr, lg);
        lg->Release(); col->Release();
        ID2D1SolidColorBrush* l = Br(Fg(0.22f));
        g_rt->DrawRoundedRectangle(rr, l, S(1)); l->Release();
    } else {
        ID2D1SolidColorBrush* b = Br(g_theme->card);
        g_rt->FillRoundedRectangle(rr, b); b->Release();
        ID2D1SolidColorBrush* l = Br(g_theme->cardLine);
        g_rt->DrawRoundedRectangle(rr, l, S(1)); l->Release();
    }
}

// 顶部 UI（搜索栏 / 齿轮 / 页码）的淡入淡出系数
// 进场：最后浮现；退场：最先消失
static float LauncherUiAlpha()
{
    if (!g_set.anim || !g_animActive) return 1.0f;
    float el = (float)(GetTickCount64() - g_animStart);
    float delay = (g_animDir == 1) ? (float)ROWS * 40.0f : 0.0f;
    float dur   = (g_animDir == 1) ? 420.0f : 260.0f;
    float tt = (el - delay) / dur;
    tt = tt < 0.0f ? 0.0f : (tt > 1.0f ? 1.0f : tt);
    float e = 1.0f - powf(1.0f - tt, 3.0f);
    return (g_animDir == 1) ? e : (1.0f - e);
}

// 画某一页的应用格子（dx = 水平偏移，a = 整体透明度）
static void DrawGridPage(float W, float H, int page, float dx, float a)
{
    EnsureIconsForPage(page);
    int base = page * PER_PAGE;
    float ic = ICON();
    float el = g_animActive ? (float)(GetTickCount64() - g_animStart) : 99999.0f;
    // 只有进出场动画需要逐格透明度；平时共用一组画笔，省掉每秒几百次 COM 调用
    bool perCell = (g_set.anim && g_animActive);
    ID2D1SolidColorBrush* bTile  = perCell ? nullptr : Br(Fg(0.07f * a));
    ID2D1SolidColorBrush* bTileH = perCell ? nullptr : Br(Fg(0.16f * a));
    ID2D1SolidColorBrush* bLine  = perCell ? nullptr : Br(Fg(0.10f * a));
    ID2D1SolidColorBrush* bHover = perCell ? nullptr : Br(Fg(0.13f * a));
    ID2D1SolidColorBrush* bLabel = perCell ? nullptr : Br(Fg(0.92f * a));

    for (int slot = 0; slot < PER_PAGE; slot++) {
        int idx = base + slot;
        if (idx >= (int)g_view.size()) break;
        Tile& t = g_tiles[g_view[idx]];
        D2D1_POINT_2F o = CellOrigin(slot, W, H);
        if (o.x + dx > W || o.x + dx + CellW(W) < 0.0f) continue;   // 完全在屏外，不画

        // 进场：底行先出、逐行往上；退场：顶行先走、逐行往下
        float prog = 1.0f;
        if (g_set.anim && g_animActive) {
            int rowIdx = slot / COLS;
            float delay = (g_animDir == 1) ? (float)(ROWS - 1 - rowIdx) * 40.0f
                                           : (float)rowIdx * 30.0f;
            float dur   = (g_animDir == 1) ? 420.0f : 300.0f;
            float tt = (el - delay) / dur;
            tt = tt < 0.0f ? 0.0f : (tt > 1.0f ? 1.0f : tt);
            float ee = 1.0f - powf(1.0f - tt, 3.0f);
            prog = (g_animDir == 1) ? ee : (1.0f - ee);
        }
        float dy = (1.0f - prog) * S(80);
        float fa = prog * a;

        float ix = o.x + (CellW(W)-ic)*0.5f + dx, iy = o.y + S(12) + dy;

        if (Liquid()) {   // 液态：图标底下垫一块玻璃
            bool hov = (slot == g_hover);
            ID2D1SolidColorBrush* fb = bTile ? (hov ? bTileH : bTile) : Br(Fg((hov?0.16f:0.07f) * fa));
            D2D1_ROUNDED_RECT tile = RoundedRect(RectF(ix-S(10), iy-S(10), ix+ic+S(10), iy+ic+S(10)), S(22), S(22));
            g_rt->FillRoundedRectangle(tile, fb);
            if (bLine) g_rt->DrawRoundedRectangle(tile, bLine, S(1));
            else { ID2D1SolidColorBrush* lb = Br(Fg(0.10f * fa)); g_rt->DrawRoundedRectangle(tile, lb, S(1)); lb->Release(); }
            if (!bTile) fb->Release();
        } else if (slot == g_hover) {
            D2D1_ROUNDED_RECT hl = RoundedRect(
                RectF(o.x+S(14)+dx, o.y+S(6)+dy, o.x+CellW(W)-S(14)+dx, o.y+CellH(H)-S(14)+dy), S(18), S(18));
            ID2D1SolidColorBrush* hb = bHover ? bHover : Br(Fg(0.13f * fa));
            g_rt->FillRoundedRectangle(hl, hb);
            if (!bHover) hb->Release();
        }

        // 图标带轻微放大
        float sc = 0.86f + 0.14f * prog;
        float cxx = ix + ic*0.5f, cyy = iy + ic*0.5f;
        float hf = ic * 0.5f * sc;
        if (t.icon) g_rt->DrawBitmap(t.icon, RectF(cxx-hf, cyy-hf, cxx+hf, cyy+hf),
                                     fa, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        // 标签：排版只建一次，之后每帧直接复用
        float lw = CellW(W) - S(12);
        if (!t.layout || fabsf(t.layoutW - lw) > 0.5f) {
            if (t.layout) { t.layout->Release(); t.layout = nullptr; }
            g_dw->CreateTextLayout(t.name.c_str(), (UINT32)t.name.size(), g_tfName, lw, S(46), &t.layout);
            if (t.layout) {
                t.layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                DWRITE_TRIMMING tr2 = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
                IDWriteInlineObject* el2 = nullptr;
                g_dw->CreateEllipsisTrimmingSign(g_tfName, &el2);
                t.layout->SetTrimming(&tr2, el2);
                if (el2) el2->Release();
                t.layoutW = lw;
            }
        }
        ID2D1SolidColorBrush* nb = bLabel ? bLabel : Br(Fg(0.92f * fa));
        if (t.layout) g_rt->DrawTextLayout(Point2F(o.x+S(6)+dx, iy+ic+S(10)), t.layout, nb);
        if (!bLabel) nb->Release();
    }
    if (bTile)  bTile->Release();
    if (bTileH) bTileH->Release();
    if (bLine)  bLine->Release();
    if (bHover) bHover->Release();
    if (bLabel) bLabel->Release();
}

// ---------------- 启动台 ----------------
static void DrawLauncher(float W, float H)
{
    DrawBackdrop(W, H);
    float ua = LauncherUiAlpha();

    float sw = S(SEARCH_W), sh = S(SEARCH_H);
    float sx = (W - sw) * 0.5f, sy = S(SEARCH_Y);
    D2D1_ROUNDED_RECT box = RoundedRect(RectF(sx,sy,sx+sw,sy+sh), sh*0.5f, sh*0.5f);
    ID2D1SolidColorBrush* f1 = Br(Fg( (Liquid()?0.10f:0.08f) * ua));
    ID2D1SolidColorBrush* l1 = Br(Fg(0.18f * ua));
    g_rt->FillRoundedRectangle(box, f1);
    g_rt->DrawRoundedRectangle(box, l1, S(1));
    f1->Release(); l1->Release();
    g_hot.push_back({ RectF(sx, sy, sx+sw, sy+sh), HK_SEARCH, 0, false });
    ID2D1SolidColorBrush* tb = Br(Fg( (g_query.empty()?0.45f:0.95f) * ua));
    Text(g_query.empty()? T(S_SEARCH) : g_query, g_tfSearch,
         RectF(sx+sh*0.55f, sy+sh*0.18f, sx+sw-sh*0.45f, sy+sh), tb, DWRITE_TEXT_ALIGNMENT_LEADING);
    tb->Release();

    // 闪烁光标
    if (!g_settingsOpen && CaretOn()) {
        float cwid = TextWidth(g_query, g_tfSearch);
        float cx2 = sx + sh*0.55f + cwid + S(1);
        ID2D1SolidColorBrush* cb2 = Br(Fg(0.85f * ua));
        g_rt->FillRectangle(RectF(cx2, sy + sh*0.26f, cx2 + S(1.6f), sy + sh*0.74f), cb2);
        cb2->Release();
    }

    float gx = sx + sw + S(14), gy = sy + (sh - S(GEAR_D)) * 0.5f;
    D2D1_RECT_F gr = RectF(gx, gy, gx + S(GEAR_D), gy + S(GEAR_D));
    D2D1_ROUNDED_RECT grr = RoundedRect(gr, S(GEAR_D)*0.5f, S(GEAR_D)*0.5f);
    ID2D1SolidColorBrush* gb = Br(Fg(0.10f * ua));
    g_rt->FillRoundedRectangle(grr, gb); gb->Release();
    ID2D1SolidColorBrush* ib = Br(Fg(0.75f * ua));
    DrawGear((gr.left+gr.right)*0.5f, (gr.top+gr.bottom)*0.5f, S(9), ib);
    ib->Release();
    g_hot.push_back({ gr, HK_GEAR, 0, false });

    if (g_slideActive) {
        // 左右翻页：两页对滑（无模糊，每页只画一遍，保证流畅）
        float t = (float)(GetTickCount64() - g_slideStart) / 300.0f;
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        float e = 1.0f - powf(1.0f - t, 3.0f);
        DrawGridPage(W, H, g_slideFrom, -g_slideDir * e * W, 1.0f);
        DrawGridPage(W, H, g_page,       g_slideDir * (1.0f - e) * W, 1.0f);
    } else {
        DrawGridPage(W, H, g_page, 0.0f, 1.0f);
    }

    if (g_view.empty()) {
        ID2D1SolidColorBrush* b = Br(Fg(0.5f));
        TextC(T(S_NO_MATCH), g_tfName, RectF(0, H*0.42f, W, H*0.5f), b);
        b->Release();
    }

    int pages = TotalPages();
    if (pages > 1) {
        float r = S(3.5f), gap = S(14);
        float total = pages * (r*2) + (pages-1) * (gap - r*2);
        float x = (W - total)*0.5f, yy = H - S(38);
        for (int i = 0; i < pages; i++) {
            ID2D1SolidColorBrush* b = Br(Fg( (i==g_page?0.95f:0.30f) * ua));
            g_rt->FillEllipse(Ellipse(Point2F(x+r,yy), r, r), b); b->Release();
            x += gap;
        }
    }
}

// ---------------- 拨杆开关 ----------------
// 开 = 绿色，关 = 灰色；切换时滑块位置和颜色都做 150ms 插值
struct SwState { float v = 0; float from = 0, target = 0; ULONGLONG t0 = 0; bool running = false; };
static std::vector<std::pair<int, SwState>> g_sw;
static bool g_swBusy = false;

static SwState& SwGet(int key, bool on)
{
    for (auto& kv : g_sw) if (kv.first == key) return kv.second;
    SwState s; s.v = s.target = on ? 1.0f : 0.0f;
    g_sw.push_back({ key, s });
    return g_sw.back().second;
}

static void DrawSwitch(const D2D1_RECT_F& r, bool on, int key)
{
    SwState& s = SwGet(key, on);
    float tgt = on ? 1.0f : 0.0f;
    if (!s.running && s.target != tgt) {
        s.from = s.v; s.target = tgt; s.t0 = GetTickCount64(); s.running = true;
        g_swBusy = true;
        SetTimer(g_hwnd, 1, 8, nullptr);
    }
    if (s.running) {
        float t = (float)(GetTickCount64() - s.t0) / 150.0f;
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        float e = 1.0f - powf(1.0f - t, 3.0f);
        s.v = s.from + (s.target - s.from) * e;
        if (t >= 1.0f) { s.v = s.target; s.running = false; }
    } else {
        s.v = tgt;
    }
    float v = s.v;

    float h = r.bottom - r.top, cy = (r.top + r.bottom) * 0.5f;
    D2D1_ROUNDED_RECT tr = RoundedRect(r, h*0.5f, h*0.5f);
    D2D1_COLOR_F gc = ColorF(g_theme->text.r, g_theme->text.g, g_theme->text.b, 0.18f);
    D2D1_COLOR_F gr = ColorF(0.20f, 0.78f, 0.35f, 1.0f);
    D2D1_COLOR_F tc = ColorF(gc.r + (gr.r - gc.r) * v, gc.g + (gr.g - gc.g) * v,
                             gc.b + (gr.b - gc.b) * v, gc.a + (gr.a - gc.a) * v);
    ID2D1SolidColorBrush* tb = Br(tc);
    g_rt->FillRoundedRectangle(tr, tb); tb->Release();

    float pad = h * 0.12f;
    float kr  = h * 0.5f - pad;
    float xL  = r.left + kr + pad;
    float xR  = r.right - kr - pad;
    float kx  = xL + (xR - xL) * v;
    ID2D1SolidColorBrush* kb = Br(ColorF(1, 1, 1, 0.98f));
    g_rt->FillEllipse(Ellipse(Point2F(kx, cy), kr, kr), kb); kb->Release();
}

// ---------------- 单选 / 滑块 ----------------
static void RadioRow(float x0, float x1, float y, const wchar_t* label, bool on, int kind, int id)
{
    D2D1_RECT_F r = RectF(x0, y, x1, y+S(ROW_H)-S(6));
    if (on) {
        D2D1_ROUNDED_RECT rr = RoundedRect(r, S(8), S(8));
        ID2D1SolidColorBrush* sb = Br(g_theme->sel);
        g_rt->FillRoundedRectangle(rr, sb); sb->Release();
    }
    D2D1_POINT_2F c = Point2F(r.left+S(18), (r.top+r.bottom)*0.5f);
    ID2D1SolidColorBrush* b = Br(g_theme->text);
    g_rt->DrawEllipse(Ellipse(c, S(7), S(7)), b, S(1.6f));
    if (on) g_rt->FillEllipse(Ellipse(c, S(3.6f), S(3.6f)), b);
    b->Release();
    ID2D1SolidColorBrush* tb = Br(g_theme->text);
    TextC(label, g_tfBody, RectF(r.left+S(38), r.top+S(11), r.right, r.bottom), tb, DWRITE_TEXT_ALIGNMENT_LEADING);
    tb->Release();
    g_hot.push_back({ r, kind, id, false });
}

static void SliderRow(float x0, float x1, float y, const wchar_t* label, int val, int lo, int hi, int id)
{
    ID2D1SolidColorBrush* tb = Br(g_theme->text);
    TextC(label, g_tfBody, RectF(x0, y+S(12), x0+S(110), y+S(ROW_H)), tb, DWRITE_TEXT_ALIGNMENT_LEADING);
    tb->Release();

    float tx0 = x0 + S(118), tx1 = x1 - S(64), ty = y + S(ROW_H)*0.5f;
    float t = (float)(val - lo) / (float)std::max(1, hi - lo);
    t = std::max(0.0f, std::min(1.0f, t));
    D2D1_RECT_F track = RectF(tx0, ty-S(2), tx1, ty+S(2));
    D2D1_ROUNDED_RECT tr = RoundedRect(track, S(2), S(2));
    ID2D1SolidColorBrush* bg = Br(Fg(0.14f));
    g_rt->FillRoundedRectangle(tr, bg); bg->Release();
    D2D1_ROUNDED_RECT fill = RoundedRect(RectF(tx0, ty-S(2), tx0+(tx1-tx0)*t, ty+S(2)), S(2), S(2));
    ID2D1SolidColorBrush* fb = Br(g_theme->accent);
    g_rt->FillRoundedRectangle(fill, fb); fb->Release();
    ID2D1SolidColorBrush* kb = Br(Fg(0.96f));
    g_rt->FillEllipse(Ellipse(Point2F(tx0+(tx1-tx0)*t, ty), S(7), S(7)), kb); kb->Release();

    wchar_t buf[16]; swprintf(buf, 16, L"%d", val);
    ID2D1SolidColorBrush* vb = Br(g_theme->textDim);
    TextC(buf, g_tfCap, RectF(tx1+S(12), y+S(14), x1, y+S(ROW_H)), vb, DWRITE_TEXT_ALIGNMENT_TRAILING);
    vb->Release();

    g_sliderRect[id] = RectF(tx0, y+S(8), tx1, y+S(ROW_H)-S(8));
    g_hot.push_back({ g_sliderRect[id], HK_SLIDER, id, false });
}

// ---------------- 设置面板 ----------------
static void DrawSettings(float W, float H)
{
    // 设置面板进出场动画
    float st = 1.0f;
    if (g_setAnimActive) {
        float el2 = (float)(GetTickCount64() - g_setAnimStart);
        float t2 = el2 / 220.0f;
        t2 = t2 < 0.0f ? 0.0f : (t2 > 1.0f ? 1.0f : t2);
        float e2 = 1.0f - powf(1.0f - t2, 3.0f);
        st = (g_setAnimDir == 1) ? e2 : (1.0f - e2);
    }
    ID2D1SolidColorBrush* dim = Br(ColorF(0,0,0, (g_theme==&g_dark?0.62f:0.30f) * st));
    g_rt->FillRectangle(RectF(0,0,W,H), dim); dim->Release();

    float cw = S(CARD_W), ch = S(CARD_H);
    float cx = (W-cw)*0.5f, cy = (H-ch)*0.5f;

    if (!g_layer) g_rt->CreateLayer(nullptr, &g_layer);
    D2D1_LAYER_PARAMETERS lp = LayerParameters();
    lp.contentBounds = D2D1::InfiniteRect();
    lp.opacity = st;
    lp.layerOptions = D2D1_LAYER_OPTIONS_NONE;
    float scl = 1.0f;                      // 纯淡入淡出，不缩放
    g_rt->SetTransform(Matrix3x2F::Scale(scl, scl, Point2F(cx+cw*0.5f, cy+ch*0.5f)));
    g_rt->PushLayer(lp, g_layer);

    DrawGlassCard(RectF(cx,cy,cx+cw,cy+ch), S(20));

    ID2D1SolidColorBrush* titleB = Br(g_theme->text);
    TextC(T(S_SETTINGS), g_tfTitle, RectF(cx+S(24), cy+S(18), cx+S(NAV_W), cy+S(54)), titleB, DWRITE_TEXT_ALIGNMENT_LEADING);
    titleB->Release();

    const wchar_t* tabs[4] = { T(S_TAB_APPEARANCE), T(S_TAB_APPS), T(S_TAB_GENERAL), T(S_TAB_ABOUT) };
    for (int i = 0; i < 4; i++) {
        D2D1_RECT_F r = RectF(cx+S(12), cy+S(62)+i*S(ROW_H), cx+S(NAV_W), cy+S(62)+(i+1)*S(ROW_H)-S(4));
        if (g_tab == i) {
            D2D1_ROUNDED_RECT rr = RoundedRect(r, S(9), S(9));
            ID2D1SolidColorBrush* sb = Br(Liquid()? Fg(0.14f) : g_theme->sel);
            g_rt->FillRoundedRectangle(rr, sb); sb->Release();
        }
        ID2D1SolidColorBrush* tb = Br(g_theme->text);
        TextC(tabs[i], g_tfBody, RectF(r.left+S(14), r.top+S(11), r.right, r.bottom), tb, DWRITE_TEXT_ALIGNMENT_LEADING);
        tb->Release();
        g_hot.push_back({ r, HK_TAB, i, false });
    }

    float x0 = cx + S(NAV_W) + S(6), x1 = cx + cw - S(26);
    float y  = cy + S(62);

    if (g_tab == 0) {
        ID2D1SolidColorBrush* hb = Br(g_theme->textDim);
        TextC(T(S_THEME), g_tfCap, RectF(x0,y,x1,y+S(20)), hb, DWRITE_TEXT_ALIGNMENT_LEADING);
        hb->Release(); y += S(26);
        RadioRow(x0,x1,y, T(S_THEME_SYSTEM), g_set.theme==0, HK_THEME, 0); y += S(ROW_H);
        RadioRow(x0,x1,y, T(S_THEME_LIGHT),  g_set.theme==1, HK_THEME, 1); y += S(ROW_H);
        RadioRow(x0,x1,y, T(S_THEME_DARK),   g_set.theme==2, HK_THEME, 2); y += S(ROW_H);
        ID2D1SolidColorBrush* hb2 = Br(g_theme->textDim);
        TextC(T(S_STYLE), g_tfCap, RectF(x0,y,x1,y+S(20)), hb2, DWRITE_TEXT_ALIGNMENT_LEADING);
        hb2->Release(); y += S(26);
        RadioRow(x0,x1,y, T(S_STYLE_CLASSIC), g_set.style==0, HK_STYLE, 0); y += S(ROW_H);
        RadioRow(x0,x1,y, T(S_STYLE_LIQUID),  g_set.style==1, HK_STYLE, 1); y += S(ROW_H);
        {
            D2D1_RECT_F r = RectF(x0, y, x1, y+S(ROW_H)-S(6));
            ID2D1SolidColorBrush* tb = Br(g_theme->text);
            TextC(T(S_ANIMATION), g_tfBody, RectF(r.left+S(10), r.top+S(11), r.right-S(64), r.bottom), tb, DWRITE_TEXT_ALIGNMENT_LEADING);
            tb->Release();
            float swW = S(42), swH = S(23);
            D2D1_RECT_F swr = RectF(r.right-S(8)-swW, r.top+(S(ROW_H)-S(6)-swH)*0.5f,
                                    r.right-S(8), r.top+(S(ROW_H)-S(6)-swH)*0.5f+swH);
            DrawSwitch(swr, g_set.anim != 0, 100);
            g_hot.push_back({ swr, HK_ANIM, 0, false });
            y += S(ROW_H);
        }
        SliderRow(x0,x1,y, T(S_FONT_SIZE), g_set.fontSize, 80, 160, 0); y += S(ROW_H);
        SliderRow(x0,x1,y, T(S_ICON_SIZE), g_set.iconSize, 56, 128, 1); y += S(ROW_H);
        // 不透明度 / 背景不透明度：只在液态模式出现，切换样式时淡入淡出
        {
            float st3 = StyleT();
            if (st3 > 0.02f) {
                size_t hotBefore = g_hot.size();
                if (!g_layer2) g_rt->CreateLayer(nullptr, &g_layer2);
                D2D1_LAYER_PARAMETERS lp3 = LayerParameters();
                lp3.contentBounds = D2D1::InfiniteRect();
                lp3.opacity = st3;
                lp3.layerOptions = D2D1_LAYER_OPTIONS_NONE;
                g_rt->PushLayer(lp3, g_layer2);
                float y2 = y;
                SliderRow(x0,x1,y2, T(S_OPACITY),    g_set.opacity,   40, 100, 2); y2 += S(ROW_H);
                SliderRow(x0,x1,y2, T(S_BG_OPACITY), g_set.bgOpacity, 0, 100, 3); y2 += S(ROW_H);
                g_rt->PopLayer();
                if (st3 < 0.9f) g_hot.resize(hotBefore);   // 淡入淡出过程中不接受点击
            }
        }
    }
    else if (g_tab == 1) {
        D2D1_RECT_F br = RectF(x1-S(184), cy+S(20), x1-S(34), cy+S(52));   // 右端收进来，别盖住关闭叉号
        D2D1_ROUNDED_RECT brr = RoundedRect(br, S(8), S(8));
        ID2D1SolidColorBrush* ab = Br(g_theme->accent);
        g_rt->FillRoundedRectangle(brr, ab);
        ID2D1SolidColorBrush* btb = Br(ColorF(1,1,1,1));
        TextC(T(S_ADD_PATH), g_tfCap, RectF(br.left, br.top+S(8), br.right, br.bottom), btb);
        btb->Release(); ab->Release();
        g_hot.push_back({ br, HK_ADDPATH, 0, false });

        float listTop = cy + S(66), listBot = cy + ch - S(26);
        float listH = listBot - listTop;
        int total = (int)g_rows.size();
        float contentH = total * S(ROW_H);
        float maxScr = std::max(0.0f, contentH - listH);
        if (g_scroll > maxScr) g_scroll = maxScr;
        if (g_scroll < 0) g_scroll = 0;

        // 补齐当前可见行的图标
        {
            std::vector<AppEntry> need; std::vector<size_t> needIdx;
            for (int i = 0; i < total; i++) {
                float tp = listTop + i * S(ROW_H) - g_scroll;
                if (tp + S(ROW_H) < listTop || tp > listBot) continue;
                if (CacheGet(g_rows[i].path)) continue;
                AppEntry e2; e2.name = g_rows[i].name; e2.lnk = g_rows[i].path;
                needIdx.push_back((size_t)i); need.push_back(e2);
            }
            if (!need.empty()) {
                LoadIcons(need, g_rt);
                for (size_t k = 0; k < need.size(); k++) g_iconCache.push_back({ need[k].lnk, need[k].icon });
            }
        }

        for (int i = 0; i < total; i++) {
            float top = listTop + i * S(ROW_H) - g_scroll;
            if (top + S(ROW_H) < listTop) continue;
            if (top > listBot) break;
            const Row& rw = g_rows[i];
            float isz = S(24);
            float iy2 = top + (S(ROW_H) - S(4) - isz) * 0.5f;
            ID2D1Bitmap* ico = CacheGet(rw.path);
            if (ico) g_rt->DrawBitmap(ico, RectF(x0+S(8), iy2, x0+S(8)+isz, iy2+isz),
                                      1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

            float swW = S(42), swH = S(23);
            float swX = x1 - S(8) - swW;
            D2D1_RECT_F swr = RectF(swX, top+(S(ROW_H)-S(4)-swH)*0.5f, swX+swW, top+(S(ROW_H)-S(4)-swH)*0.5f+swH);

            // 「···」更多按钮
            float moreW = S(24);
            D2D1_RECT_F mr = RectF(swX - S(8) - moreW, top + S(8), swX - S(8), top + S(ROW_H) - S(12));
            {
                ID2D1SolidColorBrush* mb = Br(Fg(g_menuRow == i ? 0.85f : 0.42f));
                float mcx = (mr.left + mr.right) * 0.5f, mcy = (mr.top + mr.bottom) * 0.5f;
                for (int d = -1; d <= 1; d++)
                    g_rt->FillEllipse(Ellipse(Point2F(mcx + d*S(6), mcy), S(1.8f), S(1.8f)), mb);
                mb->Release();
            }
            g_hot.push_back({ mr, HK_MORE, i, false });
            if (i == g_menuRow) g_menuAnchor = mr;

            ID2D1SolidColorBrush* tb = Br(g_theme->text);
            Text(rw.name, g_tfName, RectF(x0+S(8)+isz+S(12), top+S(10), mr.left-S(6), top+S(ROW_H)), tb, DWRITE_TEXT_ALIGNMENT_LEADING);
            tb->Release();

            DrawSwitch(swr, rw.added, 200 + i);
            g_hot.push_back({ swr, HK_ROW, i, rw.added });
        }
        // 「···」扩展菜单（受过渡动画开关控制）
        if (g_menuRow >= 0 && g_menuDir != 0) {
            float mt = (float)(GetTickCount64() - g_menuT0) / 140.0f;
            mt = mt < 0.0f ? 0.0f : (mt > 1.0f ? 1.0f : mt);
            float me = 1.0f - powf(1.0f - mt, 3.0f);
            float ma = g_set.anim ? ((g_menuDir == 1) ? me : (1.0f - me)) : ((g_menuDir == 1) ? 1.0f : 0.0f);
            if (ma > 0.01f) {
                float mw = S(150), mh = S(70);
                float mx = g_menuAnchor.right - mw;
                float my = g_menuAnchor.bottom + S(2) + (1.0f - ma) * S(6);
                D2D1_RECT_F mrc = RectF(mx, my, mx+mw, my+mh);
                D2D1_ROUNDED_RECT mrr = RoundedRect(mrc, S(9), S(9));
                D2D1_COLOR_F mbgc = g_theme->card; mbgc.a = 0.99f * ma;
                ID2D1SolidColorBrush* mbg = Br(mbgc);
                g_rt->FillRoundedRectangle(mrr, mbg); mbg->Release();
                ID2D1SolidColorBrush* mln = Br(Fg(0.18f * ma));
                g_rt->DrawRoundedRectangle(mrr, mln, S(1)); mln->Release();

                const wchar_t* items[2] = { T(S_REMOVE), T(S_RENAME) };
                for (int k = 0; k < 2; k++) {
                    D2D1_RECT_F ir = RectF(mx+S(4), my+S(5)+k*S(30), mx+mw-S(4), my+S(5)+(k+1)*S(30));
                    ID2D1SolidColorBrush* itb = Br(Fg(0.92f * ma));
                    TextC(items[k], g_tfBody, RectF(ir.left+S(12), ir.top+S(6), ir.right, ir.bottom), itb, DWRITE_TEXT_ALIGNMENT_LEADING);
                    itb->Release();
                    g_hot.push_back({ ir, k==0 ? HK_MENU_REMOVE : HK_MENU_RENAME, g_menuRow, false });
                }
            }
        }

        if (maxScr > 0) {
            float thumbH = std::max(S(36), listH * listH / contentH);
            float tpos = listTop + (listH - thumbH) * (g_scroll / maxScr);
            D2D1_ROUNDED_RECT tr = RoundedRect(RectF(x1+S(9), listTop, x1+S(15), listBot), S(3), S(3));
            ID2D1SolidColorBrush* sb = Br(g_theme->hover); g_rt->FillRoundedRectangle(tr, sb); sb->Release();
            D2D1_ROUNDED_RECT th = RoundedRect(RectF(x1+S(9), tpos, x1+S(15), tpos+thumbH), S(3), S(3));
            ID2D1SolidColorBrush* sb2 = Br(g_sbDrag ? g_theme->text : g_theme->textDim);
            g_rt->FillRoundedRectangle(th, sb2); sb2->Release();
            // 记录几何，供鼠标拖动
            g_sbTrack   = RectF(x1+S(2), listTop, x1+S(22), listBot);
            g_sbThumb   = RectF(x1+S(9), tpos, x1+S(15), tpos+thumbH);
            g_sbListTop = listTop; g_sbListH = listH; g_sbMax = maxScr; g_sbThumbH = thumbH;
        } else {
            g_sbMax = 0;
        }
    }
    else if (g_tab == 2) {
        // 开机自启动
        {
            D2D1_RECT_F r = RectF(x0, y, x1, y+S(ROW_H)-S(6));
            ID2D1SolidColorBrush* tb = Br(g_theme->text);
            TextC(T(S_AUTOSTART), g_tfBody, RectF(r.left+S(10), r.top+S(11), r.right-S(64), r.bottom), tb, DWRITE_TEXT_ALIGNMENT_LEADING);
            tb->Release();
            float swW = S(42), swH = S(23);
            D2D1_RECT_F swr = RectF(r.right-S(8)-swW, r.top+(S(ROW_H)-S(6)-swH)*0.5f,
                                    r.right-S(8), r.top+(S(ROW_H)-S(6)-swH)*0.5f+swH);
            DrawSwitch(swr, IsAutostart(), 101);
            g_hot.push_back({ swr, HK_AUTOSTART, 0, false });
            y += S(ROW_H) + S(12);
        }

        // 语言：单个按钮下拉展开
        {
            ID2D1SolidColorBrush* hb = Br(g_theme->textDim);
            TextC(T(S_LANGUAGE), g_tfCap, RectF(x0,y,x1,y+S(20)), hb, DWRITE_TEXT_ALIGNMENT_LEADING);
            hb->Release(); y += S(26);

            D2D1_RECT_F br = RectF(x0, y, x1, y+S(42));
            D2D1_ROUNDED_RECT rr = RoundedRect(br, S(9), S(9));
            ID2D1SolidColorBrush* fb = Br(Liquid()? Fg(0.08f) : g_theme->sel);
            g_rt->FillRoundedRectangle(rr, fb); fb->Release();
            ID2D1SolidColorBrush* lb = Br(Fg(0.16f));
            g_rt->DrawRoundedRectangle(rr, lb, S(1)); lb->Release();

            ID2D1SolidColorBrush* tb = Br(g_theme->text);
            TextC(LangName(g_set.lang), g_tfBody,
                  RectF(br.left+S(16), br.top+S(11), br.right-S(42), br.bottom), tb, DWRITE_TEXT_ALIGNMENT_LEADING);
            tb->Release();

            float ax = br.right - S(22), ay = (br.top+br.bottom)*0.5f;
            ID2D1SolidColorBrush* ab = Br(g_theme->textDim);
            if (g_langOpen) {
                g_rt->DrawLine(Point2F(ax-S(6), ay+S(3)), Point2F(ax, ay-S(4)), ab, S(1.8f));
                g_rt->DrawLine(Point2F(ax, ay-S(4)), Point2F(ax+S(6), ay+S(3)), ab, S(1.8f));
            } else {
                g_rt->DrawLine(Point2F(ax-S(6), ay-S(3)), Point2F(ax, ay+S(4)), ab, S(1.8f));
                g_rt->DrawLine(Point2F(ax, ay+S(4)), Point2F(ax+S(6), ay-S(3)), ab, S(1.8f));
            }
            ab->Release();
            g_hot.push_back({ br, HK_LANG_BTN, 0, false });

            if (g_langOpen) {
                float lA = 1.0f;
                if (g_set.anim) {
                    float lt = (float)(GetTickCount64() - g_langT0) / 160.0f;
                    lt = lt < 0.0f ? 0.0f : (lt > 1.0f ? 1.0f : lt);
                    float le = 1.0f - powf(1.0f - lt, 3.0f);
                    lA = (g_langDir == 2) ? (1.0f - le) : le;
                }
                g_langA = lA;
                float itemH = S(38);
                float ly = br.bottom + S(6);
                float fullH = LANG_COUNT*itemH + S(8);
                D2D1_RECT_F lr = RectF(x0, ly, x1, ly + fullH * lA);
                D2D1_ROUNDED_RECT lrr = RoundedRect(lr, S(10), S(10));
                D2D1_COLOR_F bgc = g_theme->card; bgc.a = 0.99f * lA;
                ID2D1SolidColorBrush* bg = Br(bgc);
                g_rt->FillRoundedRectangle(lrr, bg); bg->Release();
                ID2D1SolidColorBrush* lb2 = Br(Fg(0.20f));
                g_rt->DrawRoundedRectangle(lrr, lb2, S(1)); lb2->Release();

                float iy = ly + S(4);
                for (int i = 0; i < LANG_COUNT; i++) {
                    if (iy + itemH > ly + fullH * lA + 1.0f) break;   // 跟着高度裁掉
                    D2D1_RECT_F ir = RectF(x0+S(4), iy, x1-S(4), iy+itemH);
                    if (g_set.lang == i) {
                        D2D1_ROUNDED_RECT irr = RoundedRect(ir, S(7), S(7));
                        ID2D1SolidColorBrush* sb = Br(g_theme->accent);
                        g_rt->FillRoundedRectangle(irr, sb); sb->Release();
                    }
                    ID2D1SolidColorBrush* tb2 = Br(g_set.lang==i ? ColorF(1,1,1,1) : g_theme->text);
                    TextC(LangName(i), g_tfBody,
                          RectF(ir.left+S(16), ir.top+S(9), ir.right, ir.bottom), tb2, DWRITE_TEXT_ALIGNMENT_LEADING);
                    tb2->Release();
                    if (lA > 0.95f) g_hot.push_back({ ir, HK_LANG, i, false });
                    iy += itemH;
                }
            }
        }
    }
    else {
        ID2D1SolidColorBrush* tb = Br(g_theme->text);
        TextC(L"MaxPad", g_tfTitle, RectF(x0,y,x1,y+S(36)), tb, DWRITE_TEXT_ALIGNMENT_LEADING);
        tb->Release(); y += S(44);
        ID2D1SolidColorBrush* db = Br(g_theme->textDim);
        TextC(T(S_ABOUT_DESC), g_tfBody, RectF(x0,y,x1,y+S(26)), db, DWRITE_TEXT_ALIGNMENT_LEADING); y += S(32);
        wchar_t buf[128];
        swprintf(buf,128,L"%ls 2.1.0", T(S_VERSION));
        TextC(buf, g_tfBody, RectF(x0,y,x1,y+S(26)), db, DWRITE_TEXT_ALIGNMENT_LEADING); y += S(32);
        TextC(L"C++17 / Win32 / Direct2D", g_tfBody, RectF(x0,y,x1,y+S(26)), db, DWRITE_TEXT_ALIGNMENT_LEADING); y += S(32);
        swprintf(buf,128,T(S_TOTAL_APPS),(int)g_tiles.size());
        TextC(buf, g_tfBody, RectF(x0,y,x1,y+S(26)), db, DWRITE_TEXT_ALIGNMENT_LEADING);
        db->Release();
    }

    D2D1_RECT_F r = RectF(cx+cw-S(46), cy+S(18), cx+cw-S(18), cy+S(46));
    ID2D1SolidColorBrush* b = Br(g_theme->textDim);
    g_rt->DrawLine(Point2F(r.left+S(5), r.top+S(5)), Point2F(r.right-S(5), r.bottom-S(5)), b, S(1.8f));
    g_rt->DrawLine(Point2F(r.right-S(5), r.top+S(5)), Point2F(r.left+S(5), r.bottom-S(5)), b, S(1.8f));
    b->Release();
    g_hot.push_back({ r, HK_CLOSE, 0, false });

    // 重命名输入框
    if (g_renameRow >= 0) {
        ID2D1SolidColorBrush* od = Br(ColorF(0,0,0,0.45f));
        g_rt->FillRectangle(RectF(cx, cy, cx+cw, cy+ch), od); od->Release();
        float bw = S(420), bh = S(140);
        float bx = cx + (cw-bw)*0.5f, by = cy + (ch-bh)*0.5f;
        D2D1_ROUNDED_RECT br2 = RoundedRect(RectF(bx,by,bx+bw,by+bh), S(14), S(14));
        D2D1_COLOR_F rbg = g_theme->card; rbg.a = 0.99f;
        ID2D1SolidColorBrush* b1 = Br(rbg); g_rt->FillRoundedRectangle(br2, b1); b1->Release();
        ID2D1SolidColorBrush* t1 = Br(g_theme->text);
        TextC(T(S_RENAME), g_tfBody, RectF(bx+S(20), by+S(16), bx+bw-S(20), by+S(44)), t1, DWRITE_TEXT_ALIGNMENT_LEADING);
        t1->Release();
        D2D1_ROUNDED_RECT irr = RoundedRect(RectF(bx+S(20), by+S(48), bx+bw-S(20), by+S(88)), S(8), S(8));
        ID2D1SolidColorBrush* ib = Br(Fg(0.08f)); g_rt->FillRoundedRectangle(irr, ib); ib->Release();
        ID2D1SolidColorBrush* il = Br(Fg(0.20f)); g_rt->DrawRoundedRectangle(irr, il, S(1)); il->Release();
        ID2D1SolidColorBrush* t2 = Br(g_theme->text);
        Text(g_renameBuf, g_tfBody, RectF(bx+S(32), by+S(57), bx+bw-S(32), by+S(80)), t2, DWRITE_TEXT_ALIGNMENT_LEADING);
        t2->Release();
        if (CaretOn()) {
            float cwid2 = TextWidth(g_renameBuf, g_tfBody);
            float cx3 = bx + S(32) + cwid2 + S(1);
            ID2D1SolidColorBrush* cb3 = Br(g_theme->text);
            g_rt->FillRectangle(RectF(cx3, by+S(58), cx3+S(1.6f), by+S(80)), cb3);
            cb3->Release();
        }
        ID2D1SolidColorBrush* t3 = Br(g_theme->textDim);
        TextC((std::wstring(T(S_OKB)) + L"  Enter    " + T(S_CANCEL) + L"  Esc").c_str(), g_tfCap,
              RectF(bx+S(20), by+S(98), bx+bw-S(20), by+S(126)), t3, DWRITE_TEXT_ALIGNMENT_LEADING);
        t3->Release();
        // 整张卡吃掉点击，避免透到后面的行/开关
        g_hot.push_back({ RectF(cx, cy, cx+cw, cy+ch), HK_RENAME_BOX, 0, false });
    }

    g_rt->PopLayer();
    g_rt->SetTransform(Matrix3x2F::Identity());
}

static void Render()
{
    RECT rc; GetClientRect(g_hwnd, &rc);
    float W = (float)rc.right, H = (float)rc.bottom;
    g_rt->BeginDraw();
    g_rt->SetTransform(Matrix3x2F::Identity());
    g_rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    g_rt->Clear(ColorF(0,0,0,0));
    g_hot.clear();
    DrawLauncher(W, H);
    if (g_settingsOpen) DrawSettings(W, H);
    g_rt->EndDraw();
}

// ---------------- 托盘 ----------------
static NOTIFYICONDATAW g_nid;

static HICON MakeTrayIcon()
{
    const int SZ = 32;
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = SZ; bi.bmiHeader.biHeight = -SZ;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    HBITMAP hbm = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HDC mem = CreateCompatibleDC(screen);
    HGDIOBJ old = SelectObject(mem, hbm);

    RECT all = {0,0,SZ,SZ};
    HBRUSH bk = CreateSolidBrush(RGB(0,0,0));
    FillRect(mem, &all, bk); DeleteObject(bk);

    HBRUSH blue = CreateSolidBrush(RGB(59,130,246));
    HPEN bp = CreatePen(PS_SOLID, 1, RGB(59,130,246));
    HGDIOBJ ob = SelectObject(mem, blue), op = SelectObject(mem, bp);
    RoundRect(mem, 1,1,SZ-1,SZ-1, 9, 9);
    SelectObject(mem, ob); SelectObject(mem, op);
    DeleteObject(blue); DeleteObject(bp);

    HPEN wp = CreatePen(PS_SOLID, 2, RGB(255,255,255));
    SelectObject(mem, wp);
    for (int i = 0; i < 8; i++) {
        float a = (float)i * 3.14159265f / 4.0f;
        MoveToEx(mem, (int)(16+cosf(a)*7), (int)(16+sinf(a)*7), nullptr);
        LineTo(mem,   (int)(16+cosf(a)*11), (int)(16+sinf(a)*11));
    }
    HGDIOBJ nb = SelectObject(mem, GetStockObject(NULL_BRUSH));
    Ellipse(mem, 8,8,24,24);
    SelectObject(mem, nb); SelectObject(mem, wp); DeleteObject(wp);
    SelectObject(mem, old);
    DeleteDC(mem); ReleaseDC(nullptr, screen);

    BYTE* px = (BYTE*)bits;
    for (int i = 0; i < SZ*SZ; i++) {
        BYTE b = px[i*4], g = px[i*4+1], r = px[i*4+2];
        px[i*4+3] = (r|g|b) ? 255 : 0;
    }
    HBITMAP mask = CreateBitmap(SZ, SZ, 1, 1, nullptr);
    ICONINFO ii = {};
    ii.fIcon = TRUE; ii.hbmColor = hbm; ii.hbmMask = mask;
    HICON ic = CreateIconIndirect(&ii);
    DeleteObject(hbm); DeleteObject(mask);
    return ic;
}

static void AddTray()
{
    if (g_trayAdded) return;
    if (!g_trayIcon) {
        g_trayIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1),
                                       IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
        if (!g_trayIcon) g_trayIcon = MakeTrayIcon();   // 资源缺失时退回手绘
    }
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY;
    g_nid.hIcon = g_trayIcon;
    wcscpy(g_nid.szTip, L"MaxPad");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
    g_trayAdded = true;
}
static void RemoveTray()
{
    if (!g_trayAdded) return;
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    g_trayAdded = false;
}

// ---------------- 交互 ----------------
static bool PickFile(std::wstring& out)
{
    wchar_t buf[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"\u5e94\u7528\u7a0b\u5e8f (*.exe;*.lnk)\0*.exe;*.lnk\0\u6240\u6709\u6587\u4ef6 (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return false;
    out = buf;
    return true;
}

static void AddCustomFromPath(const std::wstring& path)
{
    CustomApp ca; ca.path = path;
    size_t sl = path.find_last_of(L"\\/");
    std::wstring base = (sl == std::wstring::npos) ? path : path.substr(sl+1);
    size_t dot = base.find_last_of(L'.');
    ca.name = (dot == std::wstring::npos) ? base : base.substr(0,dot);
    g_set.custom.push_back(ca);
    Commit();
}

static void ApplySlider(int id, float mx)
{
    D2D1_RECT_F r = g_sliderRect[id];
    float t = (mx - r.left) / std::max(1.0f, r.right - r.left);
    t = std::max(0.0f, std::min(1.0f, t));
    if (id == 0)      { g_set.fontSize  = (int)(80 + t*80);  MakeFormats(); }
    else if (id == 1) { g_set.iconSize  = (int)(56 + t*72); }
    else if (id == 2) { g_set.opacity   = (int)(40 + t*60);  ApplyTheme(); }
    else              { g_set.bgOpacity = (int)(t*100); ApplyTheme(); }
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

static void CommitRename()
{
    if (g_renameRow < 0 || g_renameRow >= (int)g_rows.size()) { g_renameRow = -1; return; }
    std::wstring path = g_rows[g_renameRow].path;
    std::wstring nm = g_renameBuf;
    if (!nm.empty()) {
        bool found = false;
        for (auto& p : g_set.renamed) if (p.first == path) { p.second = nm; found = true; break; }
        if (!found) g_set.renamed.push_back({ path, nm });
    }
    g_renameRow = -1;
    Commit();
}

static void HandleHotClick(int idx)
{
    const Hot& h = g_hot[idx];
    if (g_renameRow >= 0 && h.kind != HK_RENAME_BOX) return;   // 重命名弹窗是模态的
    if (h.kind != HK_LANG && h.kind != HK_LANG_BTN) g_langOpen = false;
    if (h.kind != HK_MORE && h.kind != HK_MENU_REMOVE && h.kind != HK_MENU_RENAME) {
        if (g_menuDir != 0) { g_menuDir = 0; g_menuRow = -1; }
    }
    switch (h.kind) {
    case HK_GEAR: OpenSettings(); break;
    case HK_CLOSE: CloseSettings(); break;
    case HK_TAB: g_tab = h.id; g_scroll = 0; g_langOpen = false; break;
    case HK_THEME: g_set.theme = h.id; Commit(); break;
    case HK_STYLE:
        g_styleFrom = StyleT();
        g_styleTo = (h.id == 1) ? 1.0f : 0.0f;
        g_styleT0 = GetTickCount64();
        g_styleRun = g_set.anim && (g_styleFrom != g_styleTo);
        if (g_styleRun) SetTimer(g_hwnd, 1, 8, nullptr);
        g_set.style = h.id;
        Commit();
        break;
    case HK_LANG:
        g_set.lang = h.id;
        g_langDir = 2; g_langT0 = GetTickCount64();
        SetTimer(g_hwnd, 1, 8, nullptr);
        Commit();
        break;
    case HK_AUTOSTART: SetAutostart(!IsAutostart()); break;
    case HK_SEARCH:    SetFocus(g_hwnd); break;
    case HK_RENAME_BOX: SetFocus(g_hwnd); break;
    case HK_LANG_BTN:
        if (g_langOpen) { g_langDir = 2; g_langT0 = GetTickCount64(); }
        else { g_langOpen = true; g_langDir = 1; g_langT0 = GetTickCount64(); g_langA = 0; }
        SetTimer(g_hwnd, 1, 8, nullptr);
        break;
    case HK_ANIM: g_set.anim = g_set.anim ? 0 : 1; Commit(); break;
    case HK_ADDPATH: { std::wstring p; if (PickFile(p)) AddCustomFromPath(p); break; }
    case HK_SLIDER: ApplySlider(h.id, h.r.left); break;
    case HK_ROW: {
        if (h.id < 0 || h.id >= (int)g_rows.size()) break;
        const Row& r = g_rows[h.id];
        std::wstring orig = OriginalName(r.path);
        if (r.custom) {
            // 自定义应用：只切开关，不删除条目
            for (auto& ca : g_set.custom) if (ca.path == r.path) { ca.enabled = !ca.enabled; break; }
        } else {
            if (r.added) g_set.hidden.push_back(orig);
            else {
                for (size_t i = 0; i < g_set.hidden.size(); i++)
                    if (_wcsicmp(g_set.hidden[i].c_str(), orig.c_str())==0) { g_set.hidden.erase(g_set.hidden.begin()+i); break; }
            }
        }
        Commit();
        break;
    }
    case HK_MORE: {
        if (g_menuRow == h.id) { g_menuDir = 2; g_menuT0 = GetTickCount64(); }
        else { g_menuRow = h.id; g_menuDir = 1; g_menuT0 = GetTickCount64(); g_menuAnim = 0; }
        SetTimer(g_hwnd, 1, 8, nullptr);
        break;
    }
    case HK_MENU_REMOVE: {
        if (g_menuRow < 0 || g_menuRow >= (int)g_rows.size()) break;
        Row r = g_rows[g_menuRow];
        if (r.custom) {
            for (size_t i = 0; i < g_set.custom.size(); i++)
                if (g_set.custom[i].path == r.path) { g_set.custom.erase(g_set.custom.begin()+i); break; }
        } else {
            g_set.removed.push_back(OriginalName(r.path));
        }
        g_menuRow = -1; g_menuDir = 0; g_menuAnim = 0;
        Commit();
        break;
    }
    case HK_MENU_RENAME: {
        if (g_menuRow < 0 || g_menuRow >= (int)g_rows.size()) break;
        g_renameRow = g_menuRow;
        g_renameBuf = g_rows[g_menuRow].name;
        g_menuRow = -1; g_menuDir = 0; g_menuAnim = 0;
        break;
    }
    }
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

static void ShowPad()
{
    ShowWindow(g_hwnd, SW_SHOW);
    SetForegroundWindow(g_hwnd);
    SetFocus(g_hwnd);
    g_query.clear(); g_page = 0; g_settingsOpen = false;
    g_setAnimActive = false;
    g_slideActive = false;
    RebuildView();
    if (g_set.anim) StartPadAnim(1);
    {
        DWORD bt2 = GetCaretBlinkTime();
        if (bt2 == 0 || bt2 == INFINITE) bt2 = 530;
        SetTimer(g_hwnd, 2, bt2, nullptr);   // 光标闪烁
    }
    InvalidateRect(g_hwnd, nullptr, FALSE);
}
static void HidePadNow()
{
    ShowWindow(g_hwnd, SW_HIDE);
    KillTimer(g_hwnd, 2);
}

// 翻页：带左右对滑 + 动态模糊（受「过渡动画」开关控制）
static void GoPage(int delta, int pages)
{
    if (pages <= 1) return;
    int np = ((g_page + delta) % pages + pages) % pages;
    if (np == g_page) return;
    if (!g_set.anim) { g_page = np; g_hover = -1; InvalidateRect(g_hwnd, nullptr, FALSE); return; }
    g_slideFrom  = g_page;
    g_page       = np;
    g_slideDir   = (delta > 0) ? 1 : -1;
    g_slideStart = GetTickCount64();
    g_slideActive = true;
    g_hover = -1;
    SetTimer(g_hwnd, 1, 8, nullptr);
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// 进场 / 退场动画
static void StartPadAnim(int dir)
{
    g_animDir = dir;
    g_animStart = GetTickCount64();
    g_animActive = true;
    SetTimer(g_hwnd, 1, 8, nullptr);
}
static void StartSetAnim(int dir)
{
    g_setAnimDir = dir;
    g_setAnimStart = GetTickCount64();
    g_setAnimActive = true;
    SetTimer(g_hwnd, 1, 8, nullptr);
}
static void HidePad()
{
    if (!g_animActive || g_animDir == 1) {
        if (g_set.anim) { StartPadAnim(2); return; }
    }
    HidePadNow();
}
static void OpenSettings()
{
    g_settingsOpen = true;
    g_tab = 0; g_scroll = 0;
    StartSetAnim(1);
}
static void CloseSettings()
{
    if (!g_settingsOpen) return;
    if (g_set.anim) StartSetAnim(2);
    else g_settingsOpen = false;
}

// ---------------- 窗口过程 ----------------
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: { PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd,&ps); (void)dc; Render(); EndPaint(hwnd,&ps); return 0; }

    case WM_APP + 1:
        DrainIcons();
        return 0;

    case WM_TIMER:
        if (wp == 2) { InvalidateRect(hwnd, nullptr, FALSE); return 0; }
        if (wp == 1) {
            ULONGLONG now = GetTickCount64();
            bool any = false;
            if (g_animActive) {
                float total = (g_animDir == 1) ? (420.0f + (ROWS - 1) * 40.0f + 60.0f)
                                               : (300.0f + (ROWS - 1) * 30.0f + 60.0f);
                if ((float)(now - g_animStart) > total) {
                    g_animActive = false;
                    if (g_animDir == 2) HidePadNow();
                } else any = true;
            }
            if (g_slideActive) {
                if ((float)(now - g_slideStart) > 330.0f) g_slideActive = false;
                else any = true;
            }
            if (g_setAnimActive) {
                if ((float)(now - g_setAnimStart) > 260.0f) {
                    g_setAnimActive = false;
    g_slideActive = false;
                    if (g_setAnimDir == 2) g_settingsOpen = false;
                } else any = true;
            }
            if (g_styleRun) {
                if ((float)(now - g_styleT0) < 210.0f) any = true;
                else g_styleRun = false;
            }
            if (g_langOpen && g_langDir != 0) {
                if ((float)(now - g_langT0) < 170.0f) any = true;
                else if (g_langDir == 2) { g_langOpen = false; g_langDir = 0; }
            }
            if (g_menuDir != 0) {
                if ((float)(now - g_menuT0) < 150.0f) any = true;
                else if (g_menuDir == 2) { g_menuRow = -1; g_menuDir = 0; }
            }
            if (g_swBusy) {
                bool act = false;
                for (auto& kv : g_sw)
                    if (kv.second.running && (float)(now - kv.second.t0) < 150.0f) act = true;
                if (act) any = true; else g_swBusy = false;
            }
            if (!any) KillTimer(hwnd, 1);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_HOTKEY:
        if (wp == 1) { if (IsWindowVisible(hwnd)) HidePad(); else ShowPad(); }
        return 0;

    case WM_TRAY: {
        UINT ev = LOWORD(lp);
        if (ev == WM_LBUTTONUP) {
            if (IsWindowVisible(hwnd)) HidePad(); else ShowPad();
        } else if (ev == WM_RBUTTONUP || ev == WM_CONTEXTMENU) {
            POINT pt; GetCursorPos(&pt);
            HMENU m = CreatePopupMenu();
            AppendMenuW(m, MF_STRING, 1, T(S_TRAY_OPEN));
            AppendMenuW(m, MF_STRING, 2, T(S_TRAY_SETTINGS));
            AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(m, MF_STRING, 3, T(S_TRAY_QUIT));
            SetForegroundWindow(hwnd);
            int cmd = (int)TrackPopupMenu(m, TPM_RETURNCMD|TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(m);
            if (cmd == 1) ShowPad();
            else if (cmd == 2) { ShowPad(); g_settingsOpen = true; g_tab = 0; InvalidateRect(hwnd,nullptr,FALSE); }
            else if (cmd == 3) DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        float mx = (float)GET_X_LPARAM(lp), my = (float)GET_Y_LPARAM(lp);
        if (g_dragSlider >= 0) { ApplySlider(g_dragSlider, mx); return 0; }
        if (g_sbDrag) {
            float lo = g_sbListTop, hi = g_sbListTop + g_sbListH - g_sbThumbH;
            float nt = my - g_sbGrab;
            if (nt < lo) nt = lo;
            if (nt > hi) nt = hi;
            g_scroll = (hi > lo) ? (nt - lo) / (hi - lo) * g_sbMax : 0.0f;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_settingsOpen) return 0;
        RECT rc; GetClientRect(hwnd,&rc);
        int s = HitCell(mx,my,(float)rc.right,(float)rc.bottom);
        if (s != g_hover) { g_hover = s; InvalidateRect(hwnd,nullptr,FALSE); }
        return 0;
    }

    case WM_MOUSEACTIVATE:
        return MA_ACTIVATE;      // 点一下就激活，键盘输入才进得来

    case WM_LBUTTONDOWN: {
        SetFocus(hwnd);          // 任何点击都拿回键盘焦点
        float dmx = (float)GET_X_LPARAM(lp), dmy = (float)GET_Y_LPARAM(lp);
        if (g_settingsOpen && g_tab == 1 && g_sbMax > 0) {
            D2D1_RECT_F t = g_sbTrack;
            if (dmx >= t.left && dmx <= t.right && dmy >= t.top && dmy <= t.bottom) {
                g_sbDrag = true;
                g_sbGrab = (dmy >= g_sbThumb.top && dmy <= g_sbThumb.bottom)
                         ? (dmy - g_sbThumb.top) : (g_sbThumbH * 0.5f);
                float lo = g_sbListTop, hi = g_sbListTop + g_sbListH - g_sbThumbH;
                float nt = dmy - g_sbGrab;
                if (nt < lo) nt = lo;
                if (nt > hi) nt = hi;
                g_scroll = (hi > lo) ? (nt - lo) / (hi - lo) * g_sbMax : 0.0f;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
        }
        if (!g_settingsOpen || g_tab != 0) return 0;
        float mx = (float)GET_X_LPARAM(lp), my = (float)GET_Y_LPARAM(lp);
        for (int i = (int)g_hot.size()-1; i >= 0; i--) {
            if (g_hot[i].kind != HK_SLIDER) continue;
            D2D1_RECT_F r = g_hot[i].r;
            if (my >= r.top && my <= r.bottom && mx >= r.left - S(10) && mx <= r.right + S(10)) {
                g_dragSlider = g_hot[i].id;
                ApplySlider(g_dragSlider, mx);
                return 0;
            }
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (g_sbDrag) { g_sbDrag = false; return 0; }
        if (g_dragSlider >= 0) { g_dragSlider = -1; SaveSettings(g_set, g_dir); return 0; }
        float mx = (float)GET_X_LPARAM(lp), my = (float)GET_Y_LPARAM(lp);

        for (int i = (int)g_hot.size()-1; i >= 0; i--) {
            D2D1_RECT_F r = g_hot[i].r;
            if (mx>=r.left && mx<=r.right && my>=r.top && my<=r.bottom) { HandleHotClick(i); return 0; }
        }
        if (g_menuDir != 0) { g_menuDir = 2; g_menuT0 = GetTickCount64(); SetTimer(g_hwnd, 1, 8, nullptr); InvalidateRect(g_hwnd, nullptr, FALSE); return 0; }
        if (g_langOpen) { g_langOpen = false; InvalidateRect(g_hwnd, nullptr, FALSE); return 0; }
        if (g_settingsOpen) return 0;
        RECT rc; GetClientRect(hwnd,&rc);
        int s = HitCell(mx,my,(float)rc.right,(float)rc.bottom);
        if (s >= 0) {
            int idx = g_page*PER_PAGE + s;
            if (idx < (int)g_view.size()) {
                std::wstring p = g_tiles[g_view[idx]].path;
                HidePadNow();
                LaunchApp(p);
            }
        } else HidePad();
        return 0;
    }

    case WM_RBUTTONUP:
        if (!g_settingsOpen) HidePad();
        return 0;

    case WM_MOUSEWHEEL: {
        int d = GET_WHEEL_DELTA_WPARAM(wp);
        if (g_settingsOpen) {
            if (g_tab == 1) {
                g_scroll -= (d > 0 ? S(60) : -S(60));
                if (g_scroll < 0) g_scroll = 0;
                InvalidateRect(hwnd,nullptr,FALSE);
            }
        } else {
            int pages = TotalPages();
            GoPage(d < 0 ? 1 : -1, pages);
        }
        return 0;
    }

    case WM_CHAR: {
        if (g_renameRow >= 0) {
            wchar_t rc = (wchar_t)wp;
            if (rc == 8) { if (!g_renameBuf.empty()) g_renameBuf.pop_back(); }
            else if (rc == 27) { g_renameRow = -1; }
            else if (rc == 13) { CommitRename(); }
            else if (rc >= 32 && g_renameBuf.size() < 40) g_renameBuf.push_back(rc);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_settingsOpen) return 0;
        wchar_t ch = (wchar_t)wp;
        if (ch == 8) { if (!g_query.empty()) { g_query.pop_back(); RebuildView(); InvalidateRect(hwnd,nullptr,FALSE); } }
        else if (ch == 27) HidePad();
        else if (ch >= 32) { g_query.push_back(ch); RebuildView(); InvalidateRect(hwnd,nullptr,FALSE); }
        return 0;
    }

    case WM_KEYDOWN:
        if (g_renameRow >= 0) {
            if (wp == VK_RETURN) CommitRename();
            else if (wp == VK_ESCAPE) { g_renameRow = -1; InvalidateRect(hwnd, nullptr, FALSE); }
            return 0;
        }
        if (wp == VK_ESCAPE) {
            if (g_settingsOpen) { CloseSettings(); }
            else HidePad();
            return 0;
        }
        if (g_settingsOpen) return 0;
        if (wp == VK_RETURN) {
            if (!g_view.empty()) { std::wstring p = g_tiles[g_view[0]].path; HidePadNow(); LaunchApp(p); }
            return 0;
        }
        if (wp == VK_RIGHT || wp == VK_NEXT) { GoPage(1, TotalPages()); return 0; }
        if (wp == VK_LEFT  || wp == VK_PRIOR){ GoPage(-1, TotalPages()); return 0; }
        return 0;

    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    InitializeCriticalSection(&g_icoCS);
    timeBeginPeriod(1);   // 让 SetTimer(16ms) 真正按 1ms 精度触发，否则被系统时钟量化到 ~40fps
    SetProcessDpiAwarenessContext((DPI_AWARENESS_CONTEXT)-4);

    wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring p(exe);
    size_t sl = p.find_last_of(L"\\");
    g_dir = (sl == std::wstring::npos) ? L"." : p.substr(0, sl);

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_fac);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_dw);

    LoadSettings(g_set, g_dir);
    SetCurrentLang(g_set.lang);
    if (g_set.fontSize < 80) g_set.fontSize = 100;
    if (g_set.iconSize < 56) g_set.iconSize = 88;
    if (g_set.opacity  < 40) g_set.opacity  = 100;
    if (g_set.bgOpacity < 0 || g_set.bgOpacity > 100) g_set.bgOpacity = 100;

    { HDC sdc = GetDC(nullptr); g_scale = (float)GetDeviceCaps(sdc, LOGPIXELSX)/96.0f; ReleaseDC(nullptr,sdc); }

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon   = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    wc.hIconSm = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    wc.lpszClassName = L"MaxPadWindow";
    RegisterClassExW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    g_hwnd = CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST, wc.lpszClassName, L"MaxPad",
                             WS_POPUP, 0,0,sw,sh, nullptr,nullptr,hInst,nullptr);
    if (!g_hwnd) return 1;

    { MARGINS mg = { -1,-1,-1,-1 }; DwmExtendFrameIntoClientArea(g_hwnd, &mg); }
    ApplyTheme();

    D2D1_RENDER_TARGET_PROPERTIES rtp = RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        g_scale*96.0f, g_scale*96.0f);
    D2D1_HWND_RENDER_TARGET_PROPERTIES hrtp = HwndRenderTargetProperties(
        g_hwnd, SizeU(sw,sh), D2D1_PRESENT_OPTIONS_IMMEDIATELY);
    g_fac->CreateHwndRenderTarget(rtp, hrtp, &g_rt);
    if (!g_rt) return 2;
    g_rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    MakeFormats();
    g_apps = ScanApps();
    RebuildRows();
    RebuildTiles();
    RebuildView();

    AddTray();
    EnsureIconsForPage(0);
    RegisterHotKey(g_hwnd, 1, 0, VK_F4);

    MSG m;
    while (GetMessageW(&m,nullptr,0,0)) { TranslateMessage(&m); DispatchMessageW(&m); }
    RemoveTray();
    timeEndPeriod(1);
    UnregisterHotKey(g_hwnd, 1);
    SaveSettings(g_set, g_dir);
    return 0;
}
