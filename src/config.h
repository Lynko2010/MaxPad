#pragma once
#include <windows.h>
#include <string>
#include <vector>

// ---------------- 语言 ----------------
enum Lang {
    LANG_ZH_CN = 0, LANG_ZH_TW, LANG_EN, LANG_JA, LANG_KO,
    LANG_RU, LANG_FR, LANG_DE, LANG_ES, LANG_PT,
    LANG_COUNT
};

enum StrKey {
    S_SEARCH = 0, S_SETTINGS, S_TAB_APPEARANCE, S_TAB_APPS, S_TAB_GENERAL, S_TAB_ABOUT,
    S_THEME, S_THEME_SYSTEM, S_THEME_LIGHT, S_THEME_DARK,
    S_LANGUAGE, S_ADDED, S_AVAILABLE, S_ADD, S_REMOVE, S_ADD_PATH,
    S_CLOSE, S_NO_MATCH, S_ABOUT_DESC, S_VERSION, S_TOTAL_APPS,
    S_CHOOSE_FILE, S_EMPTY_ADDED, S_EMPTY_AVAILABLE,
    S_STYLE, S_STYLE_CLASSIC, S_STYLE_LIQUID,
    S_FONT_SIZE, S_ICON_SIZE, S_OPACITY, S_BG_OPACITY, S_AUTOSTART,
    S_TRAY_OPEN, S_TRAY_SETTINGS, S_TRAY_QUIT, S_ANIMATION,
    S_RENAME, S_OKB, S_CANCEL,
    S_KEY_COUNT
};

const wchar_t* T(StrKey k);                 // 当前语言
void SetCurrentLang(int lang);
int  GetCurrentLang();
const wchar_t* LangName(int lang);          // 语言自身的名字
const wchar_t* FontForLang(int lang);       // 该语言合适的字体族

// ---------------- 主题 ----------------
enum ThemeMode { THEME_SYSTEM = 0, THEME_LIGHT, THEME_DARK };

// ---------------- 设置 ----------------
struct CustomApp {
    std::wstring path;
    std::wstring name;
    bool enabled = true;     // false = 保留条目但不在启动台显示
};

struct Settings {
    int  lang  = LANG_ZH_CN;
    int  theme = THEME_SYSTEM;
    int  style = 1;          // 0=经典  1=液态
    int  fontSize = 100;     // 80..160 (%)
    int  iconSize = 88;      // 56..128 (px)
    int  opacity  = 100;     // 40..100 (%)  控件不透明度
    int  bgOpacity = 100;    // 20..100 (%)  主页背景不透明度
    int  anim     = 1;       // 0=关闭 1=开启 过渡动画
    std::vector<std::wstring> hidden;    // 启动台不显示（但仍列在应用更改里）
    std::vector<CustomApp>    custom;    // 手动添加的应用
    std::vector<std::wstring> removed;   // 从「应用更改」列表彻底移除
    std::vector<std::pair<std::wstring, std::wstring>> renamed;  // 路径 -> 显示名
};

bool LoadSettings(Settings& s, const std::wstring& dir);
bool SaveSettings(const Settings& s, const std::wstring& dir);

// 系统当前是否浅色（读注册表偏好，只读）
bool SystemUsesLightTheme();
