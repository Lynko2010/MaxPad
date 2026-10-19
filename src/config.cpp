#include "config.h"
#include <stdio.h>

// ================= 语言表 =================
// 顺序与 StrKey 一致
static const wchar_t* kStr[LANG_COUNT][S_KEY_COUNT] = {
/* zh-CN */ {
  L"搜索", L"设置", L"外观", L"应用更改", L"通用", L"关于",
  L"主题", L"跟随系统", L"浅色模式", L"深色模式",
  L"语言", L"已添加", L"可添加", L"添加", L"移除", L"手动添加路径…",
  L"关闭", L"没有匹配的应用", L"Windows 上的 macOS 风格启动台", L"版本", L"共 %d 个应用",
  L"选择应用程序", L"还没有添加任何应用", L"没有更多可添加的应用",
  L"样式", L"经典", L"液态", L"字号", L"图标大小", L"不透明度", L"背景不透明度", L"开机自启动",
  L"打开启动台", L"设置", L"退出", L"过渡动画", L"重命名", L"确定", L"取消"
},
/* zh-TW */ {
  L"搜尋", L"設定", L"外觀", L"應用程式變更", L"一般", L"關於",
  L"主題", L"跟隨系統", L"淺色模式", L"深色模式",
  L"語言", L"已加入", L"可加入", L"加入", L"移除", L"手動加入路徑…",
  L"關閉", L"沒有符合的應用程式", L"Windows 上的 macOS 風格啟動台", L"版本", L"共 %d 個應用程式",
  L"選擇應用程式", L"尚未加入任何應用程式", L"沒有更多可加入的應用程式",
  L"樣式", L"經典", L"液態", L"字級", L"圖示大小", L"不透明度", L"背景不透明度", L"開機自動啟動",
  L"開啟啟動台", L"設定", L"結束", L"過場動畫", L"重新命名", L"確定", L"取消"
},
/* en */ {
  L"Search", L"Settings", L"Appearance", L"Apps", L"General", L"About",
  L"Theme", L"Follow system", L"Light", L"Dark",
  L"Language", L"Added", L"Available", L"Add", L"Remove", L"Add by path…",
  L"Close", L"No matching apps", L"A macOS-style launcher for Windows", L"Version", L"%d apps",
  L"Choose an application", L"No apps added yet", L"Nothing left to add",
  L"Style", L"Classic", L"Liquid", L"Font size", L"Icon size", L"Opacity", L"Background opacity", L"Launch at startup",
  L"Open launcher", L"Settings", L"Quit", L"Animation", L"Rename", L"OK", L"Cancel"
},
/* ja */ {
  L"検索", L"設定", L"外観", L"アプリ", L"一般", L"情報",
  L"テーマ", L"システムに従う", L"ライト", L"ダーク",
  L"言語", L"追加済み", L"追加可能", L"追加", L"削除", L"パスから追加…",
  L"閉じる", L"一致するアプリはありません", L"Windows 向け macOS 風ランチャー", L"バージョン", L"%d 個のアプリ",
  L"アプリを選択", L"まだアプリが追加されていません", L"追加できるアプリはありません",
  L"スタイル", L"クラシック", L"リキッド", L"文字サイズ", L"アイコンサイズ", L"不透明度", L"背景の不透明度", L"スタートアップに登録",
  L"ランチャーを開く", L"設定", L"終了", L"アニメーション", L"名前を変更", L"OK", L"キャンセル"
},
/* ko */ {
  L"검색", L"설정", L"모양", L"앱", L"일반", L"정보",
  L"테마", L"시스템 설정 따르기", L"라이트", L"다크",
  L"언어", L"추가됨", L"추가 가능", L"추가", L"제거", L"경로로 추가…",
  L"닫기", L"일치하는 앱이 없습니다", L"Windows용 macOS 스타일 런처", L"버전", L"앱 %d개",
  L"앱 선택", L"아직 추가된 앱이 없습니다", L"추가할 앱이 없습니다",
  L"스타일", L"클래식", L"리퀴드", L"글꼴 크기", L"아이콘 크기", L"불투명도", L"배경 불투명도", L"시작 시 실행",
  L"런처 열기", L"설정", L"종료", L"애니메이션", L"이름 변경", L"확인", L"취소"
},
/* ru */ {
  L"Поиск", L"Настройки", L"Внешний вид", L"Приложения", L"Общие", L"О программе",
  L"Тема", L"Как в системе", L"Светлая", L"Тёмная",
  L"Язык", L"Добавлено", L"Доступно", L"Добавить", L"Удалить", L"Добавить по пути…",
  L"Закрыть", L"Нет подходящих приложений", L"Лаунчер в стиле macOS для Windows", L"Версия", L"Приложений: %d",
  L"Выберите приложение", L"Приложения ещё не добавлены", L"Больше нечего добавлять",
  L"Стиль", L"Классика", L"Жидкость", L"Размер шрифта", L"Размер значков", L"Непрозрачность", L"Непрозрачность фона", L"Автозапуск",
  L"Открыть лаунчер", L"Настройки", L"Выход", L"Анимация", L"Переименовать", L"ОК", L"Отмена"
},
/* fr */ {
  L"Rechercher", L"Réglages", L"Apparence", L"Applications", L"Général", L"À propos",
  L"Thème", L"Suivre le système", L"Clair", L"Sombre",
  L"Langue", L"Ajoutées", L"Disponibles", L"Ajouter", L"Retirer", L"Ajouter par chemin…",
  L"Fermer", L"Aucune application correspondante", L"Un lanceur façon macOS pour Windows", L"Version", L"%d applications",
  L"Choisir une application", L"Aucune application ajoutée", L"Rien de plus à ajouter",
  L"Style", L"Classique", L"Liquide", L"Taille du texte", L"Taille des icônes", L"Opacité", L"Opacité du fond", L"Lancer au démarrage",
  L"Ouvrir le lanceur", L"Réglages", L"Quitter", L"Animation", L"Renommer", L"OK", L"Annuler"
},
/* de */ {
  L"Suchen", L"Einstellungen", L"Darstellung", L"Apps", L"Allgemein", L"Info",
  L"Design", L"Systemeinstellung folgen", L"Hell", L"Dunkel",
  L"Sprache", L"Hinzugefügt", L"Verfügbar", L"Hinzufügen", L"Entfernen", L"Über Pfad hinzufügen…",
  L"Schließen", L"Keine passenden Apps", L"Ein macOS-artiger Launcher für Windows", L"Version", L"%d Apps",
  L"App auswählen", L"Noch keine Apps hinzugefügt", L"Nichts mehr hinzuzufügen",
  L"Stil", L"Klassisch", L"Flüssig", L"Schriftgröße", L"Symbolgröße", L"Deckkraft", L"Hintergrunddeckkraft", L"Beim Start ausführen",
  L"Launcher öffnen", L"Einstellungen", L"Beenden", L"Animation", L"Umbenennen", L"OK", L"Abbrechen"
},
/* es */ {
  L"Buscar", L"Ajustes", L"Apariencia", L"Aplicaciones", L"General", L"Acerca de",
  L"Tema", L"Seguir al sistema", L"Claro", L"Oscuro",
  L"Idioma", L"Añadidas", L"Disponibles", L"Añadir", L"Quitar", L"Añadir por ruta…",
  L"Cerrar", L"Ninguna aplicación coincide", L"Un lanzador estilo macOS para Windows", L"Versión", L"%d aplicaciones",
  L"Elegir aplicación", L"Aún no hay aplicaciones", L"No hay nada más que añadir",
  L"Estilo", L"Clásico", L"Líquido", L"Tamaño de fuente", L"Tamaño de iconos", L"Opacidad", L"Opacidad del fondo", L"Iniciar al arrancar",
  L"Abrir el lanzador", L"Ajustes", L"Salir", L"Animación", L"Cambiar nombre", L"Aceptar", L"Cancelar"
},
/* pt */ {
  L"Pesquisar", L"Configurações", L"Aparência", L"Aplicativos", L"Geral", L"Sobre",
  L"Tema", L"Seguir o sistema", L"Claro", L"Escuro",
  L"Idioma", L"Adicionados", L"Disponíveis", L"Adicionar", L"Remover", L"Adicionar por caminho…",
  L"Fechar", L"Nenhum aplicativo correspondente", L"Um launcher no estilo macOS para Windows", L"Versão", L"%d aplicativos",
  L"Escolher aplicativo", L"Nenhum aplicativo adicionado", L"Nada mais para adicionar",
  L"Estilo", L"Clássico", L"Líquido", L"Tamanho da fonte", L"Tamanho dos ícones", L"Opacidade", L"Opacidade do fundo", L"Iniciar com o sistema",
  L"Abrir o launcher", L"Configurações", L"Sair", L"Animação", L"Renomear", L"OK", L"Cancelar"
},
};

static const wchar_t* kLangName[LANG_COUNT] = {
    L"简体中文", L"繁體中文", L"English", L"日本語", L"한국어",
    L"Русский", L"Français", L"Deutsch", L"Español", L"Português"
};

static const wchar_t* kFont[LANG_COUNT] = {
    L"Microsoft YaHei UI", L"Microsoft JhengHei UI", L"Segoe UI", L"Yu Gothic UI", L"Malgun Gothic",
    L"Segoe UI", L"Segoe UI", L"Segoe UI", L"Segoe UI", L"Segoe UI"
};

static int g_lang = LANG_ZH_CN;

const wchar_t* T(StrKey k)
{
    int l = g_lang; if (l < 0 || l >= LANG_COUNT) l = 0;
    int i = (int)k;  if (i < 0 || i >= S_KEY_COUNT) i = 0;
    return kStr[l][i];
}
const wchar_t* LangName(int lang) { return (lang >= 0 && lang < LANG_COUNT) ? kLangName[lang] : L""; }
const wchar_t* FontForLang(int lang) { return (lang >= 0 && lang < LANG_COUNT) ? kFont[lang] : L"Segoe UI"; }

void SetCurrentLang(int lang) { g_lang = (lang >= 0 && lang < LANG_COUNT) ? lang : 0; }
int  GetCurrentLang() { return g_lang; }

// ================= 系统主题（只读） =================
bool SystemUsesLightTheme()
{
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &k) != ERROR_SUCCESS) return false;
    DWORD v = 0, sz = sizeof(v), type = 0;
    bool light = false;
    if (RegQueryValueExW(k, L"AppsUseLightTheme", nullptr, &type, (LPBYTE)&v, &sz) == ERROR_SUCCESS)
        light = (v != 0);
    RegCloseKey(k);
    return light;
}

// ================= 设置读写 =================
static std::string W2U8(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}
static std::wstring U82W(const std::string& s)
{
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}
static std::string Trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}

bool LoadSettings(Settings& s, const std::wstring& dir)
{
    FILE* f = _wfopen((dir + L"\\MaxPad.ini").c_str(), L"rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    std::string buf((size_t)(n > 0 ? n : 0), 0);
    if (n > 0) fread(&buf[0], 1, (size_t)n, f);
    fclose(f);
    if (buf.size() >= 3 && (unsigned char)buf[0] == 0xEF) buf = buf.substr(3); // 去 BOM

    std::string line;
    for (size_t i = 0; i <= buf.size(); i++) {
        if (i == buf.size() || buf[i] == '\n') {
            std::string l = Trim(line);
            line.clear();
            if (l.empty() || l[0] == ';' || l[0] == '#') continue;
            size_t eq = l.find('=');
            if (eq == std::string::npos) continue;
            std::string k = Trim(l.substr(0, eq)), v = Trim(l.substr(eq + 1));
            if (k == "lang")  s.lang  = atoi(v.c_str());
            else if (k == "theme") s.theme = atoi(v.c_str());
            else if (k == "style") s.style = atoi(v.c_str());
            else if (k == "fontsize") s.fontSize = atoi(v.c_str());
            else if (k == "iconsize") s.iconSize = atoi(v.c_str());
            else if (k == "opacity")  s.opacity  = atoi(v.c_str());
            else if (k == "bgopacity") s.bgOpacity = atoi(v.c_str());
            else if (k == "anim")     s.anim     = atoi(v.c_str());
            else if (k == "hidden") {
                size_t p = 0;
                while (p <= v.size()) {
                    size_t q = v.find('|', p);
                    std::string one = (q == std::string::npos) ? v.substr(p) : v.substr(p, q - p);
                    one = Trim(one);
                    if (!one.empty()) s.hidden.push_back(U82W(one));
                    if (q == std::string::npos) break;
                    p = q + 1;
                }
            } else if (k == "removed") {
                size_t p = 0;
                while (p <= v.size()) {
                    size_t q = v.find('|', p);
                    std::string one = Trim((q == std::string::npos) ? v.substr(p) : v.substr(p, q - p));
                    if (!one.empty()) s.removed.push_back(U82W(one));
                    if (q == std::string::npos) break;
                    p = q + 1;
                }
            } else if (k == "renamed") {
                size_t p = 0;
                while (p <= v.size()) {
                    size_t q = v.find('|', p);
                    std::string one = Trim((q == std::string::npos) ? v.substr(p) : v.substr(p, q - p));
                    if (!one.empty()) {
                        size_t c = one.find(';');
                        if (c != std::string::npos)
                            s.renamed.push_back({ U82W(one.substr(0, c)), U82W(one.substr(c + 1)) });
                    }
                    if (q == std::string::npos) break;
                    p = q + 1;
                }
            } else if (k == "custom") {
                size_t p = 0;
                while (p <= v.size()) {
                    size_t q = v.find('|', p);
                    std::string one = (q == std::string::npos) ? v.substr(p) : v.substr(p, q - p);
                    one = Trim(one);
                    if (!one.empty()) {
                        size_t c = one.find(';');
                        CustomApp ca;
                        ca.path = U82W(c == std::string::npos ? one : one.substr(0, c));
                        std::string rest = (c == std::string::npos) ? "" : one.substr(c + 1);
                        size_t c2 = rest.find(';');
                        ca.name = U82W(c2 == std::string::npos ? rest : rest.substr(0, c2));
                        if (c2 != std::string::npos) ca.enabled = (atoi(rest.c_str() + c2 + 1) != 0);
                        if (ca.name.empty()) {
                            size_t sl = ca.path.find_last_of(L"\\/");
                            ca.name = (sl == std::wstring::npos) ? ca.path : ca.path.substr(sl + 1);
                            size_t dot = ca.name.find_last_of(L'.');
                            if (dot != std::wstring::npos) ca.name = ca.name.substr(0, dot);
                        }
                        s.custom.push_back(ca);
                    }
                    if (q == std::string::npos) break;
                    p = q + 1;
                }
            }
            continue;
        }
        line.push_back(buf[i]);
    }
    SetCurrentLang(s.lang);
    return true;
}

bool SaveSettings(const Settings& s, const std::wstring& dir)
{
    FILE* f = _wfopen((dir + L"\\MaxPad.ini").c_str(), L"wb");
    if (!f) return false;
    fputs("; MaxPad settings (portable, nothing written to the registry)\r\n", f);
    fprintf(f, "lang=%d\r\n", s.lang);
    fprintf(f, "theme=%d\r\n", s.theme);
    fprintf(f, "style=%d\r\n", s.style);
    fprintf(f, "fontsize=%d\r\n", s.fontSize);
    fprintf(f, "iconsize=%d\r\n", s.iconSize);
    fprintf(f, "opacity=%d\r\n", s.opacity);
    fprintf(f, "bgopacity=%d\r\n", s.bgOpacity);
    fprintf(f, "anim=%d\r\n", s.anim);

    std::string h;
    for (size_t i = 0; i < s.hidden.size(); i++) { if (i) h += "|"; h += W2U8(s.hidden[i]); }
    fprintf(f, "hidden=%s\r\n", h.c_str());

    std::string c;
    for (size_t i = 0; i < s.custom.size(); i++) {
        if (i) c += "|";
        c += W2U8(s.custom[i].path) + ";" + W2U8(s.custom[i].name) + ";" + (s.custom[i].enabled ? "1" : "0");
    }
    fprintf(f, "custom=%s\r\n", c.c_str());
    std::string rm;
    for (size_t i = 0; i < s.removed.size(); i++) { if (i) rm += "|"; rm += W2U8(s.removed[i]); }
    fprintf(f, "removed=%s\r\n", rm.c_str());
    std::string rn;
    for (size_t i = 0; i < s.renamed.size(); i++) { if (i) rn += "|"; rn += W2U8(s.renamed[i].first) + ";" + W2U8(s.renamed[i].second); }
    fprintf(f, "renamed=%s\r\n", rn.c_str());    fclose(f);
    return true;
}
