#pragma once
#include <windows.h>
#include <d2d1.h>
#include <string>
#include <vector>

struct AppEntry {
    std::wstring name;      // 显示名（.lnk 文件名去掉后缀）
    std::wstring lnk;       // .lnk 完整路径
    std::wstring exe;       // 解析出的目标 exe
    ID2D1Bitmap* icon = nullptr;
};

// 扫描开始菜单（系统 + 用户两级），去重排序
std::vector<AppEntry> ScanApps();

// 提取 96x96 图标 → D2D 位图
void LoadIcons(std::vector<AppEntry>& apps, ID2D1RenderTarget* rt);

// 单独提取一张图标为 HBITMAP（可在后台线程调用）
HBITMAP ExtractIconBitmap(const std::wstring& lnkPath);

// HBITMAP → D2D 位图（必须在渲染线程调用）
ID2D1Bitmap* HBmpToD2D(HBITMAP hbmp, ID2D1RenderTarget* rt);

// 启动
bool LaunchApp(const std::wstring& lnkOrExe);
