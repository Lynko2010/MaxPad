# MaxPad

> Windows 上的全屏应用启动台 · 纯原生 C++ / Direct2D · 单文件 1.3 MB · 零依赖
>
> A full-screen app launcher for Windows · native C++ / Direct2D · 1.3 MB single file · zero dependencies

**版本 / Version: 2.1.0**

[简体中文](#简体中文) · [English](#english)

---

# 简体中文

## 这是什么

MaxPad 把「打开一个应用」变成**用眼睛找图标**，而不是在开始菜单的文件夹树里翻路径。

按 `F4`，整个桌面被一层磨砂玻璃盖住，你机器上的应用排成 7×5 的网格浮上来。点一下就启动，打字就过滤，再按一次消失。

它不装任何运行库，不写注册表，不联网。整个程序就是一个 1.3 MB 的 `exe`。

## 两种安装方式

### A. 绿色版（免安装）

1. 把整个文件夹放到任意位置（桌面、D 盘、U 盘都行）
2. 双击 **MaxPad.exe**
3. （可选）右键 exe → 发送到 → 桌面快捷方式

首次运行会在 exe 同目录生成 `MaxPad.ini`（你的设置）。换个位置就换一套配置。

### B. 安装包

双击 **`installer\MaxPadSetup.exe`**，会弹出安装界面：

| 选项 | 说明 |
|---|---|
| **安装位置** | 可直接编辑，也可以点「浏览...」选目录（会自动补 `\MaxPad` 子目录） |
| **创建桌面快捷方式** | 勾选框，默认勾选 |
| **创建开始菜单快捷方式** | 勾选框，默认勾选 |

默认装到 `%LOCALAPPDATA%\Programs\MaxPad\`，同时：

- 在 `HKCU\...\Uninstall\MaxPad` 写入卸载信息，让它出现在「应用和功能」里
- **不需要管理员权限**（全程只写当前用户）

**卸载**：设置 → 应用 → MaxPad → 卸载，或直接运行安装目录里的 `MaxPadSetup.exe /uninstall`。

**静默安装/卸载**（给脚本用）：

```bat
MaxPadSetup.exe /silent                            :: 默认路径 + 两个快捷方式
MaxPadSetup.exe /silent /dir="D:\Apps\MaxPad"     :: 自定义路径
MaxPadSetup.exe /uninstall /silent
```

## 启动

无论哪种方式，窗口都不会自己弹出来——**它是托盘常驻程序**，靠 `F4` 和托盘图标唤起。

## 用法

| 操作 | 行为 |
|---|---|
| **F4** | 打开 / 关闭启动台（全局热键，任何程序里都能按） |
| **直接打字** | 实时过滤应用 |
| **Enter** | 启动第一个匹配项 |
| **← →** / 滚轮 | 翻页 |
| **Esc** / 点空白 / 右键 | 关闭 |
| **点搜索栏** | 鼠标点一下即进入输入状态（带闪烁光标） |
| **托盘图标** | 左键开关；右键菜单：打开 / 设置 / 退出 |

> 点搜索栏之外的空白会关闭启动台——这是设计如此，和 macOS 启动台一致。想输入就先点一下搜索栏。

## 设置

点搜索栏右边的 **⚙ 齿轮** 打开。四个标签页：

### 外观

| 项目 | 说明 |
|---|---|
| 主题 | 跟随系统 / 浅色 / 深色 |
| 样式 | **经典**：背景纯色不透明 / **液态**：亚克力模糊 + 玻璃高光 |
| 过渡动画 | 总开关，控制启动台开合、翻页、设置面板、菜单、拨杆、语言下拉 |
| 字号 | 80 ~ 160 % |
| 图标大小 | 56 ~ 128 px |
| 不透明度 | 控件层（卡片、玻璃底、搜索框） |
| 背景不透明度 | 0 ~ 100 %，拉到 0 时背景消失，图标直接浮在桌面上 |

> 选「经典」样式时，最后两个透明度滑块会自动隐藏。

### 应用更改

扫描你机器上所有开始菜单应用，每行左侧是图标，右侧两个控件：

- **拨杆**：绿色 = 显示在启动台，灰色 = 不显示（**关掉不会删除条目**）
- **···**：打开菜单，可以**移除**（从列表删掉）或**重命名**（启动台上的名字同步变）

顶部「手动添加路径…」可以加入开始菜单里没有的程序。右侧滚动条支持鼠标拖动。

### 通用

- **开机自启动**：打开后会在启动文件夹放一个快捷方式
- **语言**：下拉选择，支持 10 种语言，字体自动匹配

### 关于

版本号、技术栈、当前扫描到的应用数量。

## 目录结构

```
MaxPad/
├── MaxPad.exe          1.3 MB   主程序（单文件，双击即用）
├── MaxPad.ini                   设置文件，首次运行自动生成
├── app.ico                      应用图标（16/24/32/48/64/128/256 七个尺寸）
├── app.rc                       资源脚本，把图标编进 exe
├── build.cmd                    一键编译脚本（含资源编译）
├── run.cmd                      启动脚本（等价于双击 exe）
├── INTRO.md                     项目介绍：缘起 / 功能 / 技术手记
├── README.md                    本文件
├── preview.png                  截图：启动台
├── preview-settings.png         截图：设置 · 外观（4 个滑块）
├── preview-apps.png             截图：设置 · 应用更改
├── preview-caret.png            截图：搜索栏闪烁光标
├── preview-about.png            截图：设置 · 关于
├── installer/
│   ├── MaxPadSetup.exe              安装包（内嵌主程序，2.0 MB）
│   ├── setup.cpp                    安装/卸载逻辑
│   ├── setup.rc                     安装包资源
│   └── build-setup.cmd              编译安装包
└── src/
    ├── main.cpp                 1534 行  窗口 / 渲染 / 动画 / 输入 / 设置界面 / 托盘
    ├── apps.cpp                  135 行  扫描开始菜单、提取图标、启动应用
    ├── apps.h                     19 行  上面的声明
    ├── config.cpp                272 行  10 种语言表、设置读写、系统主题检测
    └── config.h                   44 行  设置结构体与语言枚举
```

### 各文件作用

| 文件 | 作用 |
|---|---|
| **MaxPad.exe** | 主程序。静态链接，不依赖 MSVC 运行库。删掉文件夹即彻底卸载 |
| **MaxPad.ini** | 纯文本设置。删掉它 = 恢复出厂默认 |
| **build.cmd** | 调用 g++ 编译。文件头有编译器路径，换机器要改 |
| **run.cmd** | 只是 `start` 一下 exe，方便加桌面快捷方式 |
| **INTRO.md** | 偏叙事的项目介绍，适合发给别人看 |
| **README.md** | 使用说明（就是本文件） |
| **preview*.png** | 界面截图，可以删，不影响运行 |
| **src/main.cpp** | 全部 UI 逻辑：分层窗口、D2D 渲染、六套动画、鼠标键盘处理、设置面板、托盘 |
| **src/apps.cpp** | 扫两级开始菜单目录，用 `IShellItemImageFactory` 抠 96×96 图标，`ShellExecuteEx` 启动 |
| **src/config.cpp** | 语言字符串表（10×40 条）、ini 读写、读注册表判断系统深浅色 |

## 配置文件

`MaxPad.ini` 在 exe 同目录，纯文本 UTF-8：

```ini
; MaxPad settings (portable, nothing written to the registry)
lang=0
theme=2
style=1
fontsize=100
iconsize=88
opacity=100
bgopacity=100
anim=1
hidden=
custom=
removed=
renamed=
```

| 键 | 取值 |
|---|---|
| `lang` | 0 简中 / 1 繁中 / 2 英 / 3 日 / 4 韩 / 5 俄 / 6 法 / 7 德 / 8 西 / 9 葡 |
| `theme` | 0 跟随系统 / 1 浅色 / 2 深色 |
| `style` | 0 经典 / 1 液态 |
| `fontsize` | 80 ~ 160 |
| `iconsize` | 56 ~ 128 |
| `opacity` | 40 ~ 100（控件不透明度） |
| `bgopacity` | 0 ~ 100（背景不透明度） |
| `anim` | 0 关 / 1 开（过渡动画） |
| `hidden` | 启动台不显示的应用名，`|` 分隔 |
| `custom` | 手动添加：`路径;名称;启用状态` |
| `removed` | 从应用更改列表移除的应用名 |
| `renamed` | 重命名：`路径;新名称` |

换一份 ini 就是换一套配置，可以带 U 盘走。

## 编译

需要 **MinGW-w64 g++**（不是 MSVC）。`build.cmd` 顶部的 `GPP` 变量指向编译器路径：

```bat
build.cmd
```

输出 `MaxPad.exe`。依赖的系统库：

```
d2d1  dwrite  dwmapi  ole32  uuid  gdi32  user32  shell32  shlwapi  comdlg32  winmm
```

## 常见问题

**图标显示成空白方块？**
应用图标由后台线程逐个提取，启动后需要一两秒补齐。如果一直空白，可能是该快捷方式指向的程序已卸载。

**托盘图标找不到？**
Win10 默认把新图标塞进折叠区（任务栏的 `^` 箭头里）。点开折叠区，把蓝色齿轮图标拖到任务栏上即可常驻。

**按 F4 没反应？**
F4 是全局热键，可能被其他程序抢占了。检查是否有多个 MaxPad 实例在跑，或者关掉占用热键的软件。

**开机自启动失效？**
自启动靠启动文件夹里的快捷方式。如果你移动了 exe 位置，旧快捷方式会指向不存在的路径——去「通用」把开关关一次再开，会重新生成。

**中文输入法打不了字？**
目前只支持直接输入英文和数字。中文需要走输入法，尚未接入。

**能放 U 盘吗？**
能。所有路径都是运行时算的，配置跟着 exe 走。

**会写注册表吗？**
不写。唯一会动系统的是「开机自启动」——它在启动文件夹放一个 .lnk，而且只有你主动打开才创建。

## 卸载

1. 设置 → 通用，关掉「开机自启动」
2. 托盘图标右键 → 退出
3. 删掉整个文件夹

三步之后，系统里不留任何痕迹。

---

# English

## What is it

MaxPad turns "open an app" into **finding the icon with your eyes** instead of digging through a folder tree in the Start menu.

Press `F4` and the desktop is covered by a sheet of frosted glass while your apps float up in a 7×5 grid. Click to launch, type to filter, press again to dismiss.

No runtime to install. No registry writes. No network. The whole program is a single 1.3 MB `exe`.

## Two ways to install

### A. Portable (no installation)

1. Put the folder anywhere (desktop, D drive, USB stick)
2. Double-click **MaxPad.exe**
3. (Optional) right-click the exe → Send to → Desktop (create shortcut)

On first run it creates `MaxPad.ini` next to the exe.

### B. Installer

Run **`installer\MaxPadSetup.exe`**. A dialog appears with:

| Option | Description |
|---|---|
| **Install location** | Editable field, or click **Browse...** (a `\MaxPad` subfolder is appended automatically) |
| **Create desktop shortcut** | Checkbox, checked by default |
| **Create Start menu shortcut** | Checkbox, checked by default |

Defaults to `%LOCALAPPDATA%\Programs\MaxPad\`. It also:

- Writes an entry under `HKCU\...\Uninstall\MaxPad` so it shows up in Apps & features
- Requires no administrator rights (everything is per-user)

**Uninstall**: Settings → Apps → MaxPad → Uninstall, or run `MaxPadSetup.exe /uninstall` from the install folder.

**Silent install / uninstall** (for scripts):

```bat
MaxPadSetup.exe /silent                            :: default path + both shortcuts
MaxPadSetup.exe /silent /dir="D:\Apps\MaxPad"     :: custom path
MaxPadSetup.exe /uninstall /silent
```

## Launching

Either way, the window does not pop up by itself — **it lives in the system tray** and is summoned by `F4` or the tray icon.

## Usage

| Action | Result |
|---|---|
| **F4** | Open / close the launcher (global hotkey, works from any app) |
| **Type** | Filter apps live |
| **Enter** | Launch the first match |
| **← →** / wheel | Change page |
| **Esc** / click empty space / right-click | Close |
| **Click the search bar** | Focus it for typing (blinking caret) |
| **Tray icon** | Left-click to toggle; right-click for Open / Settings / Quit |

> Clicking any empty space outside the search bar closes the launcher — by design, same as the macOS launcher. Click the search bar first if you want to type.

## Settings

Click the **⚙ gear** to the right of the search bar. Four tabs:

### Appearance

| Item | Description |
|---|---|
| Theme | Follow system / Light / Dark |
| Style | **Classic**: solid opaque background / **Liquid**: acrylic blur + glass highlights |
| Transitions | Master switch for all animations |
| Font size | 80 ~ 160 % |
| Icon size | 56 ~ 128 px |
| Opacity | Controls layer (card, glass tiles, search box) |
| Background opacity | 0 ~ 100 %; at 0 the background disappears and icons float on the desktop |

> The two opacity sliders are hidden automatically in Classic style.

### Apps

Scans every Start-menu app on your machine. Each row shows an icon, a name, and two controls:

- **Toggle**: green = shown in the launcher, grey = hidden (**switching off does not delete the entry**)
- **···**: opens a menu with **Remove** (delete from this list) and **Rename** (the launcher name updates too)

"Add by path…" lets you add programs that are not in the Start menu. The scrollbar on the right supports dragging.

### General

- **Launch at startup**: creates a shortcut in the Startup folder
- **Language**: dropdown with 10 languages, matching fonts applied automatically

### About

Version, tech stack, and how many apps were found.

## Project structure

```
MaxPad/
├── MaxPad.exe          1.3 MB   Main binary (single file, just run it)
├── MaxPad.ini                   Settings, created on first run
├── app.ico                      App icon (16/24/32/48/64/128/256)
├── app.rc                       Resource script that embeds the icon
├── build.cmd                    One-click build script (includes resources)
├── run.cmd                      Launcher script (same as double-clicking the exe)
├── INTRO.md                     Project write-up: origin / features / engineering notes
├── README.md                    This file
├── preview*.png                 Screenshots (safe to delete)
└── src/
    ├── main.cpp                 1534 lines  Window / rendering / animation / input / settings UI / tray
    ├── apps.cpp                  135 lines  Start-menu scan, icon extraction, launching
    ├── apps.h                     19 lines  Declarations
    ├── config.cpp                272 lines  10-language string table, settings I/O, system theme detection
    └── config.h                   44 lines  Settings struct and language enum
```

### What each file does

| File | Purpose |
|---|---|
| **MaxPad.exe** | The program. Statically linked, no MSVC runtime needed. Delete the folder to uninstall |
| **MaxPad.ini** | Plain-text settings. Delete it to restore defaults |
| **build.cmd** | Invokes g++. The compiler path is at the top of the file |
| **run.cmd** | Just `start`s the exe; handy for desktop shortcuts |
| **INTRO.md** | Narrative introduction, good for sharing |
| **README.md** | Usage documentation (this file) |
| **preview*.png** | Screenshots; deleting them does not affect the program |
| **src/main.cpp** | All UI logic: layered window, D2D rendering, six animations, mouse/keyboard, settings panel, tray |
| **src/apps.cpp** | Scans both Start-menu roots, pulls 96×96 icons via `IShellItemImageFactory`, launches with `ShellExecuteEx` |
| **src/config.cpp** | Language table (10×40 strings), ini I/O, reads the registry to detect system light/dark |

## Configuration

`MaxPad.ini` sits next to the exe, plain UTF-8 text:

```ini
; MaxPad settings (portable, nothing written to the registry)
lang=0
theme=2
style=1
fontsize=100
iconsize=88
opacity=100
bgopacity=100
anim=1
hidden=
custom=
removed=
renamed=
```

| Key | Value |
|---|---|
| `lang` | 0 zh-CN / 1 zh-TW / 2 en / 3 ja / 4 ko / 5 ru / 6 fr / 7 de / 8 es / 9 pt |
| `theme` | 0 follow system / 1 light / 2 dark |
| `style` | 0 classic / 1 liquid |
| `fontsize` | 80 ~ 160 |
| `iconsize` | 56 ~ 128 |
| `opacity` | 40 ~ 100 (controls layer) |
| `bgopacity` | 0 ~ 100 (background) |
| `anim` | 0 off / 1 on (transitions) |
| `hidden` | App names hidden from the launcher, separated by `|` |
| `custom` | Manually added: `path;name;enabled` |
| `removed` | App names removed from the Apps list |
| `renamed` | Renames: `path;new name` |

A different ini means a different profile — carry it on a USB stick.

## Building

Requires **MinGW-w64 g++** (not MSVC). The `GPP` variable at the top of `build.cmd` points to the compiler:

```bat
build.cmd
```

Produces `MaxPad.exe`. System libraries used:

```
d2d1  dwrite  dwmapi  ole32  uuid  gdi32  user32  shell32  shlwapi  comdlg32  winmm
```

## FAQ

**Icons show as blank squares?**
Icons are extracted on a background thread and take a second or two to fill in. If they stay blank, the shortcut probably points at an uninstalled program.

**Can't find the tray icon?**
Windows 10 hides new tray icons in the overflow area (the `^` chevron). Open it and drag the blue gear onto the taskbar to pin it.

**F4 does nothing?**
F4 is a global hotkey and may be taken by another app. Check for a second MaxPad instance, or close the software that grabbed the hotkey.

**Autostart stopped working?**
Autostart relies on a shortcut in the Startup folder. If you moved the exe, the old shortcut points nowhere — toggle it off and on again under General to recreate it.

**Can I type Chinese?**
Only ASCII is supported right now. IME input is not wired up yet.

**Can I put it on a USB stick?**
Yes. All paths are resolved at runtime and the config travels with the exe.

**Does it write to the registry?**
No. The only system change is the optional startup shortcut, and only when you turn that toggle on.

## Uninstall

1. Settings → General, turn off **Launch at startup**
2. Right-click the tray icon → Quit
3. Delete the folder

Nothing is left behind.

---

## License

No third-party libraries or assets are bundled. The tray icon is drawn at runtime with GDI.
