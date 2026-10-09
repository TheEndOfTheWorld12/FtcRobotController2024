// ==WindhawkMod==
// @id              shortcut-guide-plus
// @name            Shortcut Guide Plus
// @description     Hold Win, Ctrl or Alt to see every shortcut on it - including your own, read from AutoHotkey, PowerToys, Windhawk and Windows shortcuts
// @version         1.0
// @author          aariv
// @include         windhawk.exe
// @compilerOptions -lole32 -loleaut32 -luuid -lshlwapi -lshell32 -lgdi32 -lgdiplus -ladvapi32 -luser32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Shortcut Guide Plus

Hold **Win**, **Ctrl** or **Alt** on its own and an overlay lists the shortcuts
on that key. Add **Shift** (or another modifier) while it is up and the list
narrows to that combination. Press any other key, or let go, and it disappears
and the shortcut runs as normal - nothing is swallowed.

The point of it is the part PowerToys' Shortcut Guide leaves out: **your own
shortcuts**. It reads them from wherever they are actually defined, rather than
from a fixed list of what Windows ships with:

* **AutoHotkey** - every `.ahk` it can find, including the scripts you have
  running right now, whose paths it reads from their own windows. The label
  comes from the comment on the hotkey line, or from what the hotkey does.
* **PowerToys** - the activation shortcut of every module (PowerToys Run,
  FancyZones, Color Picker, Always On Top, Text Extractor and the rest), read
  from each module's own settings, plus every **Keyboard Manager** remap.
* **Windhawk** - any hotkey setting in any of your installed mods.
* **Windows shortcuts** - the "Shortcut key" field on `.lnk` files in the Start
  menu and on the desktop.
* **Windows itself** - the built-in Win, Ctrl and Alt shortcuts.
* **Anything else** - a list you write yourself, for the ones nothing publishes
  (see below).

## What it cannot read on its own

A program that registers a hotkey privately does not write it anywhere this, or
anything else, can read. **Logitech Options+ is in this group**, as is most
vendor software. Two ways to cover them:

* Add them to **Extra shortcuts** in the settings, one per line, as
  `Win+Shift+D = Toggle do not disturb`. These win over every other source, so
  this is also how you correct a label you do not like.
* Turn on **Probe for shortcuts owned by other apps**. Windows will say whether
  a combination is already claimed, without saying by what, so those appear as
  "in use by another app". It is a blunt signal - it cannot see hotkeys that
  work by keyboard hook, which is how AutoHotkey and PowerToys mostly work - and
  it briefly claims each combination it tests, so it is off by default.

## When two things want the same key

Only one entry is shown: the one most likely to actually win. Your own list
first, then AutoHotkey, PowerToys, Windhawk, `.lnk` shortcuts, and Windows last.
If the guide credits the wrong one, put the right answer in **Extra shortcuts**.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- holdDelay: 400
  $name: Hold delay (ms)
  $description: >-
    How long a modifier must be held on its own before the guide appears, from
    100 to 3000.
- showWin: true
  $name: Show for the Windows key
- showCtrl: true
  $name: Show for Ctrl
- showAlt: true
  $name: Show for Alt
- sourceAutoHotkey: true
  $name: Read AutoHotkey scripts
  $description: >-
    Hotkeys defined in .ahk files. Scripts that are running are always read;
    the folders below are searched as well.
- ahkFolders:
  - "%USERPROFILE%\\Documents"
  - "%USERPROFILE%\\Desktop"
  - "%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"
  $name: AutoHotkey folders
  $description: >-
    Folders to search for .ahk files, including sub-folders. Environment
    variables are expanded.
- sourcePowerToys: true
  $name: Read PowerToys
  $description: >-
    Each module's activation shortcut, and every Keyboard Manager remap.
- sourceWindhawk: true
  $name: Read Windhawk mod settings
  $description: Any setting in any installed mod whose value looks like a hotkey.
- sourceShortcutFiles: true
  $name: Read Windows shortcut (.lnk) hotkeys
  $description: >-
    The "Shortcut key" field on shortcuts in the Start menu, the desktop and
    the startup folder.
- showBuiltins: true
  $name: Show the shortcuts built into Windows
- probeOtherApps: false
  $name: Probe for shortcuts owned by other apps
  $description: >-
    Ask Windows which combinations are already claimed, and list the ones
    nothing else accounts for as "in use by another app". Cannot see hotkeys
    that work by keyboard hook, and briefly claims each combination it tests.
- extraShortcuts:
  - ""
  $name: Extra shortcuts
  $description: >-
    One per line, as "Win+Shift+D = Toggle do not disturb". These override
    every other source, so use them for anything that cannot be read and to
    correct a label.
- extraFile: ""
  $name: Extra shortcuts file
  $description: >-
    Optional path to a text file of the same "Win+Shift+D = description" lines,
    for when there are too many to keep in the box above. Lines starting with #
    or ; are ignored.
- fontSize: 14
  $name: Text size
- opacity: 95
  $name: Opacity
  $description: Percentage, from 20 to 100.
- maxColumns: 4
  $name: Maximum columns
  $description: How wide the guide may grow before it starts scrolling off.
- refreshMinutes: 5
  $name: Re-read sources every (minutes)
  $description: >-
    How often the shortcut list is rebuilt in the background, so a hotkey you
    just added turns up without restarting the mod. 0 reads them once only.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <initguid.h>  // Must come first so the shell GUIDs we use get storage.

#include <commctrl.h>
#include <gdiplus.h>
#include <objbase.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <windowsx.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// Settings.

struct {
    int holdDelayMs;
    bool showWin;
    bool showCtrl;
    bool showAlt;
    bool sourceAutoHotkey;
    bool sourcePowerToys;
    bool sourceWindhawk;
    bool sourceShortcutFiles;
    bool showBuiltins;
    bool probeOtherApps;
    int fontSize;
    int opacity;
    int maxColumns;
    int refreshMinutes;
    std::vector<std::wstring> ahkFolders;
    std::vector<std::wstring> extraShortcuts;
    std::wstring extraFile;
} g_settings;

int ClampInt(int value, int low, int high) {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

// Reads a Windhawk string array setting into `out`, stopping at the first empty
// entry (which is what an untouched list looks like). `format` is the setting
// name with an index, e.g. L"ahkFolders[%d]".
void LoadStringList(PCWSTR format, std::vector<std::wstring>& out) {
    out.clear();
    for (int i = 0;; i++) {
        PCWSTR value = Wh_GetStringSetting(format, i);
        if (!value) {
            break;
        }
        bool empty = !*value;
        if (!empty) {
            out.push_back(value);
        }
        Wh_FreeStringSetting(value);
        if (empty) {
            break;
        }
    }
}

void LoadSettings() {
    g_settings.holdDelayMs =
        ClampInt(Wh_GetIntSetting(L"holdDelay"), 100, 3000);
    g_settings.showWin = Wh_GetIntSetting(L"showWin");
    g_settings.showCtrl = Wh_GetIntSetting(L"showCtrl");
    g_settings.showAlt = Wh_GetIntSetting(L"showAlt");
    g_settings.sourceAutoHotkey = Wh_GetIntSetting(L"sourceAutoHotkey");
    g_settings.sourcePowerToys = Wh_GetIntSetting(L"sourcePowerToys");
    g_settings.sourceWindhawk = Wh_GetIntSetting(L"sourceWindhawk");
    g_settings.sourceShortcutFiles = Wh_GetIntSetting(L"sourceShortcutFiles");
    g_settings.showBuiltins = Wh_GetIntSetting(L"showBuiltins");
    g_settings.probeOtherApps = Wh_GetIntSetting(L"probeOtherApps");
    g_settings.fontSize = ClampInt(Wh_GetIntSetting(L"fontSize"), 8, 40);
    g_settings.opacity = ClampInt(Wh_GetIntSetting(L"opacity"), 20, 100);
    g_settings.maxColumns = ClampInt(Wh_GetIntSetting(L"maxColumns"), 1, 8);
    g_settings.refreshMinutes =
        ClampInt(Wh_GetIntSetting(L"refreshMinutes"), 0, 1440);

    LoadStringList(L"ahkFolders[%d]", g_settings.ahkFolders);
    LoadStringList(L"extraShortcuts[%d]", g_settings.extraShortcuts);

    PCWSTR extraFile = Wh_GetStringSetting(L"extraFile");
    g_settings.extraFile = extraFile ? extraFile : L"";
    Wh_FreeStringSetting(extraFile);
}

////////////////////////////////////////////////////////////////////////////////
// The shortcut model.

constexpr UINT kModWin = 1;
constexpr UINT kModCtrl = 2;
constexpr UINT kModAlt = 4;
constexpr UINT kModShift = 8;

// Which source gets to describe a combination when more than one claims it.
// Higher wins. The order is "most likely to actually run": what the user told
// us outright, then the hook-based tools that see a key before anything else,
// then the ones that go through RegisterHotKey, then Windows itself.
constexpr int kPriManual = 100;
constexpr int kPriAutoHotkey = 80;
constexpr int kPriPowerToys = 70;
constexpr int kPriWindhawk = 60;
constexpr int kPriShortcutFile = 40;
constexpr int kPriBuiltin = 20;
constexpr int kPriProbe = 10;

struct Shortcut {
    UINT mods;
    UINT vk;
    std::wstring label;
    std::wstring source;
    int priority;
};

////////////////////////////////////////////////////////////////////////////////
// Small string helpers.

std::wstring ToLower(const std::wstring& s) {
    std::wstring r = s;
    for (wchar_t& c : r) {
        if (c >= L'A' && c <= L'Z') {
            c = (wchar_t)(c - L'A' + L'a');
        }
    }
    return r;
}

std::wstring Trim(const std::wstring& s) {
    size_t begin = s.find_first_not_of(L" \t\r\n");
    if (begin == std::wstring::npos) {
        return std::wstring();
    }
    size_t end = s.find_last_not_of(L" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

// Turns a settings key such as "open_powerlauncher" into "Open powerlauncher",
// which is a better label than nothing when a source names things in code
// style.
std::wstring Prettify(const std::wstring& name) {
    std::wstring r;
    for (wchar_t c : name) {
        r += (c == L'_' || c == L'-') ? L' ' : c;
    }
    r = Trim(r);
    if (!r.empty() && r[0] >= L'a' && r[0] <= L'z') {
        r[0] = (wchar_t)(r[0] - L'a' + L'A');
    }
    return r;
}

std::wstring ExpandEnv(const std::wstring& path) {
    WCHAR buffer[1024];
    DWORD len = ExpandEnvironmentStringsW(path.c_str(), buffer,
                                          ARRAYSIZE(buffer));
    if (len == 0 || len > ARRAYSIZE(buffer)) {
        return path;
    }
    return buffer;
}

std::wstring JoinPath(const std::wstring& dir, const std::wstring& name) {
    if (dir.empty()) {
        return name;
    }
    if (dir.back() == L'\\' || dir.back() == L'/') {
        return dir + name;
    }
    return dir + L"\\" + name;
}

std::wstring FileStem(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    std::wstring name =
        slash == std::wstring::npos ? path : path.substr(slash + 1);
    size_t dot = name.find_last_of(L'.');
    return dot == std::wstring::npos ? name : name.substr(0, dot);
}

// Shortens a label so one over-long line cannot stretch the whole guide.
std::wstring Shorten(const std::wstring& text, size_t maxChars) {
    std::wstring t = Trim(text);
    if (t.size() <= maxChars) {
        return t;
    }
    return t.substr(0, maxChars - 1) + L"\u2026";
}

// Reads a text file as UTF-16. UTF-8 (with or without a BOM) and UTF-16LE are
// all handled, since the files read here are written by different tools.
bool ReadTextFile(const std::wstring& path, std::wstring& out, DWORD maxBytes) {
    out.clear();

    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ |
                                  FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 ||
        size.QuadPart > (LONGLONG)maxBytes) {
        CloseHandle(file);
        return false;
    }

    std::string bytes;
    bytes.resize((size_t)size.QuadPart);
    DWORD read = 0;
    bool ok = ReadFile(file, &bytes[0], (DWORD)bytes.size(), &read, nullptr) &&
              read == bytes.size();
    CloseHandle(file);
    if (!ok) {
        return false;
    }

    if (bytes.size() >= 2 && (BYTE)bytes[0] == 0xFF && (BYTE)bytes[1] == 0xFE) {
        out.assign((const wchar_t*)(bytes.data() + 2),
                   (bytes.size() - 2) / sizeof(wchar_t));
        return true;
    }

    size_t offset = 0;
    if (bytes.size() >= 3 && (BYTE)bytes[0] == 0xEF && (BYTE)bytes[1] == 0xBB &&
        (BYTE)bytes[2] == 0xBF) {
        offset = 3;
    }
    int needed = MultiByteToWideChar(CP_UTF8, 0, bytes.data() + offset,
                                     (int)(bytes.size() - offset), nullptr, 0);
    if (needed <= 0) {
        return false;
    }
    out.resize((size_t)needed);
    MultiByteToWideChar(CP_UTF8, 0, bytes.data() + offset,
                        (int)(bytes.size() - offset), &out[0], needed);
    return true;
}

std::vector<std::wstring> SplitLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    size_t start = 0;
    while (start <= text.size()) {
        size_t end = text.find(L'\n', start);
        if (end == std::wstring::npos) {
            lines.push_back(text.substr(start));
            break;
        }
        std::wstring line = text.substr(start, end - start);
        if (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }
        lines.push_back(line);
        start = end + 1;
    }
    return lines;
}

////////////////////////////////////////////////////////////////////////////////
// Key names, in both directions.

// True for the keys whose scan code needs the extended flag before Windows will
// name them correctly - without it, Home comes back as Numpad 7 and so on.
bool IsExtendedKey(UINT vk) {
    switch (vk) {
        case VK_INSERT:
        case VK_DELETE:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_NUMLOCK:
        case VK_DIVIDE:
        case VK_RCONTROL:
        case VK_RMENU:
        case VK_SNAPSHOT:
        case VK_CANCEL:
            return true;
    }
    return false;
}

// The name to print for a virtual key. Windows is asked for it wherever it can
// answer, so the guide reads in the same language and layout as the keyboard
// itself; the cases below are the ones it answers badly or not at all.
std::wstring VkName(UINT vk) {
    switch (vk) {
        case VK_LWIN:
        case VK_RWIN:
            return L"Win";
        case VK_RETURN:
            return L"Enter";
        case VK_ESCAPE:
            return L"Esc";
        case VK_PRIOR:
            return L"Page Up";
        case VK_NEXT:
            return L"Page Down";
        case VK_SNAPSHOT:
            return L"Print Screen";
        case VK_LEFT:
            return L"Left";
        case VK_RIGHT:
            return L"Right";
        case VK_UP:
            return L"Up";
        case VK_DOWN:
            return L"Down";
        case VK_OEM_COMMA:
            return L",";
        case VK_OEM_PERIOD:
            return L".";
        case VK_OEM_PLUS:
            return L"+";
        case VK_OEM_MINUS:
            return L"-";
    }

    if (vk >= '0' && vk <= '9') {
        return std::wstring(1, (wchar_t)vk);
    }
    if (vk >= 'A' && vk <= 'Z') {
        return std::wstring(1, (wchar_t)vk);
    }
    if (vk >= VK_F1 && vk <= VK_F24) {
        return L"F" + std::to_wstring(vk - VK_F1 + 1);
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
        return L"Numpad " + std::to_wstring(vk - VK_NUMPAD0);
    }

    UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    if (scanCode) {
        LONG lParam = (LONG)(scanCode << 16);
        if (IsExtendedKey(vk)) {
            lParam |= (1 << 24);
        }
        WCHAR name[64];
        if (GetKeyNameTextW(lParam, name, ARRAYSIZE(name)) > 0 && *name) {
            return name;
        }
    }
    return L"0x" + std::to_wstring(vk);
}

std::wstring ModsName(UINT mods) {
    std::wstring r;
    if (mods & kModWin) {
        r += L"Win + ";
    }
    if (mods & kModCtrl) {
        r += L"Ctrl + ";
    }
    if (mods & kModAlt) {
        r += L"Alt + ";
    }
    if (mods & kModShift) {
        r += L"Shift + ";
    }
    return r;
}

std::wstring FormatShortcut(UINT mods, UINT vk) {
    return ModsName(mods) + VkName(vk);
}

// The virtual key a written key name means, or 0. Covers what the sources this
// mod reads actually write: single characters, function keys, numpad keys, and
// the usual spelled-out names (including AutoHotkey's abbreviations).
UINT VkFromName(const std::wstring& rawName) {
    std::wstring name = ToLower(Trim(rawName));
    if (name.empty()) {
        return 0;
    }

    if (name.size() == 1) {
        SHORT scan = VkKeyScanW(name[0]);
        if (scan != -1) {
            return (UINT)(BYTE)(scan & 0xFF);
        }
        return 0;
    }

    if (name[0] == L'f' && name.size() <= 3) {
        bool digits = true;
        for (size_t i = 1; i < name.size(); i++) {
            if (name[i] < L'0' || name[i] > L'9') {
                digits = false;
                break;
            }
        }
        if (digits) {
            int n = (int)wcstol(name.c_str() + 1, nullptr, 10);
            if (n >= 1 && n <= 24) {
                return VK_F1 + (n - 1);
            }
        }
    }

    if (name.compare(0, 6, L"numpad") == 0 && name.size() == 7 &&
        name[6] >= L'0' && name[6] <= L'9') {
        return VK_NUMPAD0 + (name[6] - L'0');
    }

    // "vkA2" / "vk0xA2", as AutoHotkey writes a raw key.
    if (name.compare(0, 2, L"vk") == 0 && name.size() > 2) {
        return (UINT)wcstoul(name.c_str() + 2, nullptr, 16);
    }

    struct NamedKey {
        PCWSTR name;
        UINT vk;
    };
    static const NamedKey kNamed[] = {
        {L"enter", VK_RETURN},      {L"return", VK_RETURN},
        {L"esc", VK_ESCAPE},        {L"escape", VK_ESCAPE},
        {L"space", VK_SPACE},       {L"spacebar", VK_SPACE},
        {L"tab", VK_TAB},           {L"backspace", VK_BACK},
        {L"bs", VK_BACK},           {L"delete", VK_DELETE},
        {L"del", VK_DELETE},        {L"insert", VK_INSERT},
        {L"ins", VK_INSERT},        {L"home", VK_HOME},
        {L"end", VK_END},           {L"pgup", VK_PRIOR},
        {L"pageup", VK_PRIOR},      {L"prior", VK_PRIOR},
        {L"pgdn", VK_NEXT},         {L"pagedown", VK_NEXT},
        {L"next", VK_NEXT},         {L"up", VK_UP},
        {L"down", VK_DOWN},         {L"left", VK_LEFT},
        {L"right", VK_RIGHT},       {L"printscreen", VK_SNAPSHOT},
        {L"prtsc", VK_SNAPSHOT},    {L"printscrn", VK_SNAPSHOT},
        {L"pause", VK_PAUSE},       {L"break", VK_PAUSE},
        {L"capslock", VK_CAPITAL},  {L"numlock", VK_NUMLOCK},
        {L"scrolllock", VK_SCROLL}, {L"appskey", VK_APPS},
        {L"apps", VK_APPS},         {L"menu", VK_APPS},
        {L"numpadenter", VK_RETURN},
        {L"numpadadd", VK_ADD},     {L"numpadsub", VK_SUBTRACT},
        {L"numpadmult", VK_MULTIPLY},
        {L"numpaddiv", VK_DIVIDE},  {L"numpaddot", VK_DECIMAL},
        {L"comma", VK_OEM_COMMA},   {L"period", VK_OEM_PERIOD},
        {L"plus", VK_OEM_PLUS},     {L"minus", VK_OEM_MINUS},
        {L"volume_up", VK_VOLUME_UP},
        {L"volume_down", VK_VOLUME_DOWN},
        {L"volume_mute", VK_VOLUME_MUTE},
        {L"media_play_pause", VK_MEDIA_PLAY_PAUSE},
        {L"media_next", VK_MEDIA_NEXT_TRACK},
        {L"media_prev", VK_MEDIA_PREV_TRACK},
    };
    for (const NamedKey& key : kNamed) {
        if (name == key.name) {
            return key.vk;
        }
    }
    return 0;
}

// The modifier bit a virtual key is, or 0 if it is an ordinary key.
UINT ModFromVk(UINT vk) {
    switch (vk) {
        case VK_LWIN:
        case VK_RWIN:
            return kModWin;
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL:
            return kModCtrl;
        case VK_MENU:
        case VK_LMENU:
        case VK_RMENU:
            return kModAlt;
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            return kModShift;
    }
    return 0;
}

// Parses "Win+Shift+D", "ctrl + alt + t", "Win Shift D". Returns false unless
// it ends in a real key.
bool ParseShortcutText(const std::wstring& text, UINT* outMods, UINT* outVk) {
    *outMods = 0;
    *outVk = 0;

    std::vector<std::wstring> parts;
    std::wstring current;
    for (size_t i = 0; i < text.size(); i++) {
        wchar_t c = text[i];
        if (c == L'+' || c == L' ' || c == L'\t') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            } else if (c == L'+' && i + 1 == text.size() && !parts.empty()) {
                // A "+" with nothing after it is the plus key, not a separator.
                parts.push_back(L"+");
            }
            continue;
        }
        current += c;
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    if (parts.empty()) {
        return false;
    }

    UINT mods = 0;
    for (size_t i = 0; i + 1 < parts.size(); i++) {
        std::wstring part = ToLower(parts[i]);
        if (part == L"win" || part == L"windows" || part == L"lwin" ||
            part == L"rwin" || part == L"super" || part == L"meta") {
            mods |= kModWin;
        } else if (part == L"ctrl" || part == L"control" || part == L"ctl" ||
                   part == L"lctrl" || part == L"rctrl") {
            mods |= kModCtrl;
        } else if (part == L"alt" || part == L"lalt" || part == L"ralt") {
            mods |= kModAlt;
        } else if (part == L"shift" || part == L"lshift" || part == L"rshift") {
            mods |= kModShift;
        } else {
            return false;  // An unknown word where a modifier should be.
        }
    }

    UINT vk = VkFromName(parts.back());
    if (!vk || ModFromVk(vk)) {
        return false;  // No key, or the "key" is itself a modifier.
    }

    *outMods = mods;
    *outVk = vk;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// A small JSON reader, for the settings files PowerToys writes.
//
// Nodes live in one flat array and refer to each other by index, so the array
// can grow during parsing without invalidating anything held across a recursive
// call.

enum class JsonType { object, array, string, number, boolean, null };

struct JsonNode {
    JsonType type = JsonType::null;
    std::wstring str;
    double num = 0;
    bool boolean = false;
    std::vector<std::wstring> keys;  // Object member names.
    std::vector<size_t> children;    // Member values, or array items.
};

struct JsonDoc {
    std::vector<JsonNode> nodes;
    size_t root = 0;
    bool ok = false;

    const JsonNode& At(size_t index) const { return nodes[index]; }

    // The value of `key` in the object `index`, or SIZE_MAX.
    size_t Member(size_t index, PCWSTR key) const {
        const JsonNode& node = nodes[index];
        if (node.type != JsonType::object) {
            return SIZE_MAX;
        }
        for (size_t i = 0; i < node.keys.size(); i++) {
            if (node.keys[i] == key) {
                return node.children[i];
            }
        }
        return SIZE_MAX;
    }

    bool BoolMember(size_t index, PCWSTR key) const {
        size_t at = Member(index, key);
        return at != SIZE_MAX && nodes[at].type == JsonType::boolean &&
               nodes[at].boolean;
    }

    std::wstring StringMember(size_t index, PCWSTR key) const {
        size_t at = Member(index, key);
        if (at == SIZE_MAX || nodes[at].type != JsonType::string) {
            return std::wstring();
        }
        return nodes[at].str;
    }
};

class JsonReader {
   public:
    JsonReader(const std::wstring& text, JsonDoc& doc)
        : m_text(text), m_doc(doc) {}

    bool Read() {
        size_t root = 0;
        if (!ReadValue(&root)) {
            return false;
        }
        m_doc.root = root;
        m_doc.ok = true;
        return true;
    }

   private:
    static constexpr int kMaxDepth = 64;

    const std::wstring& m_text;
    JsonDoc& m_doc;
    size_t m_pos = 0;
    int m_depth = 0;

    void SkipSpace() {
        while (m_pos < m_text.size()) {
            wchar_t c = m_text[m_pos];
            if (c == L' ' || c == L'\t' || c == L'\r' || c == L'\n') {
                m_pos++;
            } else {
                break;
            }
        }
    }

    bool Literal(PCWSTR word) {
        size_t len = wcslen(word);
        if (m_text.compare(m_pos, len, word) != 0) {
            return false;
        }
        m_pos += len;
        return true;
    }

    size_t AddNode(JsonType type) {
        m_doc.nodes.push_back(JsonNode());
        m_doc.nodes.back().type = type;
        return m_doc.nodes.size() - 1;
    }

    bool ReadString(std::wstring* out) {
        if (m_pos >= m_text.size() || m_text[m_pos] != L'"') {
            return false;
        }
        m_pos++;
        out->clear();
        while (m_pos < m_text.size()) {
            wchar_t c = m_text[m_pos++];
            if (c == L'"') {
                return true;
            }
            if (c != L'\\') {
                *out += c;
                continue;
            }
            if (m_pos >= m_text.size()) {
                return false;
            }
            wchar_t esc = m_text[m_pos++];
            switch (esc) {
                case L'n': *out += L'\n'; break;
                case L't': *out += L'\t'; break;
                case L'r': *out += L'\r'; break;
                case L'b': *out += L'\b'; break;
                case L'f': *out += L'\f'; break;
                case L'u': {
                    if (m_pos + 4 > m_text.size()) {
                        return false;
                    }
                    std::wstring hex = m_text.substr(m_pos, 4);
                    m_pos += 4;
                    *out += (wchar_t)wcstoul(hex.c_str(), nullptr, 16);
                    break;
                }
                default: *out += esc; break;
            }
        }
        return false;
    }

    bool ReadValue(size_t* out) {
        if (++m_depth > kMaxDepth) {
            m_depth--;
            return false;
        }
        bool ok = ReadValueInner(out);
        m_depth--;
        return ok;
    }

    bool ReadValueInner(size_t* out) {
        SkipSpace();
        if (m_pos >= m_text.size()) {
            return false;
        }

        wchar_t c = m_text[m_pos];
        if (c == L'{') {
            m_pos++;
            size_t index = AddNode(JsonType::object);
            SkipSpace();
            if (m_pos < m_text.size() && m_text[m_pos] == L'}') {
                m_pos++;
                *out = index;
                return true;
            }
            for (;;) {
                SkipSpace();
                std::wstring key;
                if (!ReadString(&key)) {
                    return false;
                }
                SkipSpace();
                if (m_pos >= m_text.size() || m_text[m_pos] != L':') {
                    return false;
                }
                m_pos++;
                size_t value = 0;
                if (!ReadValue(&value)) {
                    return false;
                }
                m_doc.nodes[index].keys.push_back(key);
                m_doc.nodes[index].children.push_back(value);
                SkipSpace();
                if (m_pos < m_text.size() && m_text[m_pos] == L',') {
                    m_pos++;
                    continue;
                }
                if (m_pos < m_text.size() && m_text[m_pos] == L'}') {
                    m_pos++;
                    *out = index;
                    return true;
                }
                return false;
            }
        }

        if (c == L'[') {
            m_pos++;
            size_t index = AddNode(JsonType::array);
            SkipSpace();
            if (m_pos < m_text.size() && m_text[m_pos] == L']') {
                m_pos++;
                *out = index;
                return true;
            }
            for (;;) {
                size_t value = 0;
                if (!ReadValue(&value)) {
                    return false;
                }
                m_doc.nodes[index].children.push_back(value);
                SkipSpace();
                if (m_pos < m_text.size() && m_text[m_pos] == L',') {
                    m_pos++;
                    continue;
                }
                if (m_pos < m_text.size() && m_text[m_pos] == L']') {
                    m_pos++;
                    *out = index;
                    return true;
                }
                return false;
            }
        }

        if (c == L'"') {
            std::wstring str;
            if (!ReadString(&str)) {
                return false;
            }
            size_t index = AddNode(JsonType::string);
            m_doc.nodes[index].str = str;
            *out = index;
            return true;
        }

        if (Literal(L"true")) {
            size_t index = AddNode(JsonType::boolean);
            m_doc.nodes[index].boolean = true;
            *out = index;
            return true;
        }

        if (Literal(L"false")) {
            size_t index = AddNode(JsonType::boolean);
            m_doc.nodes[index].boolean = false;
            *out = index;
            return true;
        }

        if (Literal(L"null")) {
            *out = AddNode(JsonType::null);
            return true;
        }

        size_t start = m_pos;
        while (m_pos < m_text.size() &&
               ((m_text[m_pos] >= L'0' && m_text[m_pos] <= L'9') ||
                m_text[m_pos] == L'-' || m_text[m_pos] == L'+' ||
                m_text[m_pos] == L'.' || m_text[m_pos] == L'e' ||
                m_text[m_pos] == L'E')) {
            m_pos++;
        }
        if (m_pos == start) {
            return false;
        }
        size_t index = AddNode(JsonType::number);
        m_doc.nodes[index].num =
            wcstod(m_text.substr(start, m_pos - start).c_str(), nullptr);
        *out = index;
        return true;
    }
};

bool ParseJsonFile(const std::wstring& path, JsonDoc& doc) {
    std::wstring text;
    if (!ReadTextFile(path, text, 4 * 1024 * 1024)) {
        return false;
    }
    JsonReader reader(text, doc);
    return reader.Read();
}

////////////////////////////////////////////////////////////////////////////////
// Collecting shortcuts.

// Holds one entry per key combination. When two sources claim the same one,
// the higher priority wins outright: the guide is meant to say what will
// happen, not to list everything that would like to.
struct ShortcutTable {
    std::unordered_map<UINT64, Shortcut> byCombo;

    void Add(UINT mods,
             UINT vk,
             const std::wstring& label,
             const std::wstring& source,
             int priority) {
        // Only the three keys the guide opens on are worth listing; a
        // Shift-only or plain key is never shown, so it would only be weight.
        if (!vk || !(mods & (kModWin | kModCtrl | kModAlt))) {
            return;
        }

        UINT64 key = ((UINT64)mods << 32) | vk;
        auto it = byCombo.find(key);
        if (it != byCombo.end() && it->second.priority >= priority) {
            return;
        }

        Shortcut entry;
        entry.mods = mods;
        entry.vk = vk;
        entry.label = Shorten(label.empty() ? source : label, 60);
        entry.source = source;
        entry.priority = priority;
        byCombo[key] = entry;
    }

    std::vector<Shortcut> Sorted() const {
        std::vector<Shortcut> all;
        all.reserve(byCombo.size());
        for (const auto& entry : byCombo) {
            all.push_back(entry.second);
        }
        std::sort(all.begin(), all.end(),
                  [](const Shortcut& a, const Shortcut& b) {
                      if (a.mods != b.mods) {
                          return a.mods < b.mods;
                      }
                      return VkName(a.vk) < VkName(b.vk);
                  });
        return all;
    }
};

////////////////////////////////////////////////////////////////////////////////
// Source: the list the user writes.

void AddManualLine(const std::wstring& rawLine, ShortcutTable& table) {
    std::wstring line = Trim(rawLine);
    if (line.empty() || line[0] == L'#' || line[0] == L';') {
        return;
    }

    size_t equals = line.find(L'=');
    std::wstring combo =
        equals == std::wstring::npos ? line : Trim(line.substr(0, equals));
    std::wstring label =
        equals == std::wstring::npos ? std::wstring()
                                     : Trim(line.substr(equals + 1));

    UINT mods = 0;
    UINT vk = 0;
    if (ParseShortcutText(combo, &mods, &vk)) {
        table.Add(mods, vk, label, L"Yours", kPriManual);
    } else {
        Wh_Log(L"extra shortcut not understood: %s", line.c_str());
    }
}

void CollectManual(ShortcutTable& table) {
    for (const std::wstring& line : g_settings.extraShortcuts) {
        AddManualLine(line, table);
    }

    if (g_settings.extraFile.empty()) {
        return;
    }
    std::wstring text;
    if (!ReadTextFile(ExpandEnv(g_settings.extraFile), text, 1024 * 1024)) {
        Wh_Log(L"could not read the extra shortcuts file");
        return;
    }
    for (const std::wstring& line : SplitLines(text)) {
        AddManualLine(line, table);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Source: AutoHotkey.

// Reads the modifier prefix and key of an AutoHotkey hotkey label, the part
// before "::". AutoHotkey spells the modifiers as symbols - # Win, ! Alt,
// ^ Ctrl, + Shift - and allows $ ~ * < > in front of them, which change how the
// hotkey is hooked rather than which keys it is.
bool ParseAhkHotkey(const std::wstring& spec, UINT* outMods, UINT* outVk) {
    *outMods = 0;
    *outVk = 0;

    UINT mods = 0;
    size_t i = 0;
    for (; i < spec.size(); i++) {
        wchar_t c = spec[i];
        if (c == L'#') {
            mods |= kModWin;
        } else if (c == L'!') {
            mods |= kModAlt;
        } else if (c == L'^') {
            mods |= kModCtrl;
        } else if (c == L'+') {
            mods |= kModShift;
        } else if (c == L'$' || c == L'~' || c == L'*' || c == L'<' ||
                   c == L'>') {
            continue;
        } else {
            break;
        }
    }

    std::wstring key = Trim(spec.substr(i));
    if (key.empty()) {
        return false;
    }
    // "a & b" is a custom two-key combination, not a modifier shortcut.
    if (key.find(L'&') != std::wstring::npos) {
        return false;
    }
    // "#f up::" fires on release; same keys, so drop the suffix.
    std::wstring lower = ToLower(key);
    if (lower.size() > 3 && lower.compare(lower.size() - 3, 3, L" up") == 0) {
        key = Trim(key.substr(0, key.size() - 3));
    }

    UINT vk = VkFromName(key);
    if (!vk || ModFromVk(vk)) {
        return false;
    }

    *outMods = mods;
    *outVk = vk;
    return true;
}

// The comment at the end of a line of code, or empty. Quoted text is skipped so
// a semicolon inside a string is not mistaken for one.
std::wstring TrailingComment(const std::wstring& line) {
    bool inString = false;
    wchar_t quote = 0;
    for (size_t i = 0; i < line.size(); i++) {
        wchar_t c = line[i];
        if (inString) {
            if (c == quote) {
                inString = false;
            }
            continue;
        }
        if (c == L'"' || c == L'\'') {
            inString = true;
            quote = c;
            continue;
        }
        // AutoHotkey needs whitespace before a trailing comment.
        if (c == L';' && i > 0 &&
            (line[i - 1] == L' ' || line[i - 1] == L'\t')) {
            return Trim(line.substr(i + 1));
        }
    }
    return std::wstring();
}

bool IsCommentLine(const std::wstring& line, std::wstring* text) {
    std::wstring trimmed = Trim(line);
    if (trimmed.empty() || trimmed[0] != L';') {
        return false;
    }
    *text = Trim(trimmed.substr(1));
    return true;
}

void ParseAhkText(const std::wstring& text,
                  const std::wstring& scriptName,
                  ShortcutTable& table) {
    std::vector<std::wstring> lines = SplitLines(text);
    std::wstring context;
    bool inBlockComment = false;

    for (size_t i = 0; i < lines.size(); i++) {
        std::wstring trimmed = Trim(lines[i]);

        if (inBlockComment) {
            if (trimmed.compare(0, 2, L"*/") == 0) {
                inBlockComment = false;
            }
            continue;
        }
        if (trimmed.compare(0, 2, L"/*") == 0) {
            inBlockComment = true;
            continue;
        }
        if (trimmed.empty() || trimmed[0] == L';') {
            continue;
        }

        // Hotkey context: everything after it applies only in that window.
        std::wstring lowerLine = ToLower(trimmed);
        if (lowerLine.compare(0, 6, L"#hotif") == 0 ||
            lowerLine.compare(0, 3, L"#if") == 0) {
            context = Trim(trimmed.substr(trimmed.find_first_of(L" \t") ==
                                                  std::wstring::npos
                                              ? trimmed.size()
                                              : trimmed.find_first_of(L" \t")));
            continue;
        }

        // A hotstring starts with "::" and expands typed text; not a shortcut.
        if (trimmed.compare(0, 2, L"::") == 0) {
            continue;
        }
        size_t colons = trimmed.find(L"::");
        if (colons == std::wstring::npos || colons == 0) {
            continue;
        }

        UINT mods = 0;
        UINT vk = 0;
        if (!ParseAhkHotkey(trimmed.substr(0, colons), &mods, &vk)) {
            continue;
        }

        // What it does, in the order the answer is most likely to be good: a
        // comment on the line, then a one-line body, then a comment just above,
        // then the first line of the body, then the script's own name.
        std::wstring rest = Trim(trimmed.substr(colons + 2));
        std::wstring label = TrailingComment(trimmed);
        if (label.empty() && !rest.empty() && rest != L"{") {
            label = TrailingComment(rest);
            if (label.empty()) {
                label = rest;
            }
        }
        if (label.empty() && i > 0) {
            std::wstring above;
            if (IsCommentLine(lines[i - 1], &above)) {
                label = above;
            }
        }
        if (label.empty()) {
            for (size_t j = i + 1; j < lines.size() && j <= i + 3; j++) {
                std::wstring body = Trim(lines[j]);
                if (body.empty() || body == L"{") {
                    continue;
                }
                std::wstring comment;
                label = IsCommentLine(body, &comment) ? comment : body;
                break;
            }
        }
        if (label.empty()) {
            label = scriptName;
        }
        if (!context.empty()) {
            label += L"  [" + Shorten(context, 24) + L"]";
        }

        table.Add(mods, vk, label, L"AutoHotkey - " + scriptName,
                  kPriAutoHotkey);
    }
}

void ParseAhkFile(const std::wstring& path, ShortcutTable& table) {
    std::wstring text;
    if (!ReadTextFile(path, text, 2 * 1024 * 1024)) {
        return;
    }
    ParseAhkText(text, FileStem(path), table);
}

// Collects the full paths of the AutoHotkey scripts that are running now. Each
// one keeps a hidden window whose title is its own path, which is a far better
// list than any folder search: it is exactly the scripts in effect.
struct AhkWindowScan {
    std::vector<std::wstring>* paths;
};

BOOL CALLBACK FindAhkWindowsProc(HWND hwnd, LPARAM lParam) {
    AhkWindowScan* scan = (AhkWindowScan*)lParam;

    WCHAR className[64];
    if (!GetClassNameW(hwnd, className, ARRAYSIZE(className)) ||
        wcscmp(className, L"AutoHotkey") != 0) {
        return TRUE;
    }

    WCHAR title[1024];
    if (GetWindowTextW(hwnd, title, ARRAYSIZE(title)) <= 0) {
        return TRUE;
    }

    // "C:\path\script.ahk - AutoHotkey v2.0.11"
    std::wstring text = title;
    size_t dash = text.find(L" - AutoHotkey");
    if (dash != std::wstring::npos) {
        text = text.substr(0, dash);
    }
    text = Trim(text);
    if (text.size() > 4 && ToLower(text).compare(text.size() - 4, 4, L".ahk") == 0) {
        scan->paths->push_back(text);
    }
    return TRUE;
}

// Adds every .ahk under `dir`, to a bounded depth and count so a folder that
// turns out to be enormous cannot stall the scan.
void ScanFolderForAhk(const std::wstring& dir,
                      int depth,
                      int* budget,
                      std::vector<std::wstring>& out) {
    if (depth <= 0 || *budget <= 0) {
        return;
    }

    WIN32_FIND_DATAW find;
    HANDLE handle = FindFirstFileW(JoinPath(dir, L"*").c_str(), &find);
    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }

    std::vector<std::wstring> subFolders;
    do {
        if (*budget <= 0) {
            break;
        }
        std::wstring name = find.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Folders that are always large and never hold a user's scripts.
            std::wstring lower = ToLower(name);
            if (lower == L"node_modules" || lower == L".git" ||
                lower == L"appdata" || lower == L"$recycle.bin") {
                continue;
            }
            subFolders.push_back(JoinPath(dir, name));
            continue;
        }
        if (name.size() > 4 &&
            ToLower(name).compare(name.size() - 4, 4, L".ahk") == 0) {
            out.push_back(JoinPath(dir, name));
            (*budget)--;
        }
    } while (FindNextFileW(handle, &find));
    FindClose(handle);

    for (const std::wstring& sub : subFolders) {
        ScanFolderForAhk(sub, depth - 1, budget, out);
    }
}

void CollectAutoHotkey(ShortcutTable& table) {
    std::vector<std::wstring> paths;

    AhkWindowScan scan;
    scan.paths = &paths;
    EnumWindows(FindAhkWindowsProc, (LPARAM)&scan);
    size_t running = paths.size();

    int budget = 400;
    for (const std::wstring& folder : g_settings.ahkFolders) {
        ScanFolderForAhk(ExpandEnv(folder), 4, &budget, paths);
    }

    // The same script can be both running and found in a folder.
    std::unordered_set<std::wstring> seen;
    int parsed = 0;
    for (const std::wstring& path : paths) {
        if (!seen.insert(ToLower(path)).second) {
            continue;
        }
        ParseAhkFile(path, table);
        parsed++;
    }
    Wh_Log(L"AutoHotkey: %d scripts (%d running)", parsed, (int)running);
}

////////////////////////////////////////////////////////////////////////////////
// Source: PowerToys.

// Every PowerToys module writes its activation shortcut the same way: an object
// carrying the four modifier flags and the key's code. Finding them by that
// shape rather than by a list of module names means a module added to PowerToys
// later is picked up without this mod knowing anything about it.
void WalkPowerToysJson(const JsonDoc& doc,
                       size_t index,
                       const std::wstring& key,
                       const std::wstring& module,
                       ShortcutTable& table) {
    const JsonNode& node = doc.At(index);

    if (node.type == JsonType::object) {
        size_t code = doc.Member(index, L"code");
        if (code != SIZE_MAX && doc.At(code).type == JsonType::number) {
            UINT mods = 0;
            if (doc.BoolMember(index, L"win")) {
                mods |= kModWin;
            }
            if (doc.BoolMember(index, L"ctrl")) {
                mods |= kModCtrl;
            }
            if (doc.BoolMember(index, L"alt")) {
                mods |= kModAlt;
            }
            if (doc.BoolMember(index, L"shift")) {
                mods |= kModShift;
            }
            UINT vk = (UINT)doc.At(code).num;
            if (mods && vk) {
                std::wstring label = Prettify(key);
                if (label.empty()) {
                    label = module;
                }
                table.Add(mods, vk, label, L"PowerToys - " + module,
                          kPriPowerToys);
            }
        }
        for (size_t i = 0; i < node.children.size(); i++) {
            WalkPowerToysJson(doc, node.children[i], node.keys[i], module,
                              table);
        }
        return;
    }

    if (node.type == JsonType::array) {
        for (size_t child : node.children) {
            WalkPowerToysJson(doc, child, key, module, table);
        }
    }
}

// Turns Keyboard Manager's "91;16;68" into modifiers plus a key.
bool ParseVkList(const std::wstring& list, UINT* outMods, UINT* outVk) {
    *outMods = 0;
    *outVk = 0;

    UINT mods = 0;
    UINT vk = 0;
    size_t start = 0;
    while (start <= list.size()) {
        size_t end = list.find(L';', start);
        std::wstring piece =
            list.substr(start, end == std::wstring::npos ? std::wstring::npos
                                                         : end - start);
        piece = Trim(piece);
        if (!piece.empty()) {
            UINT code = (UINT)wcstoul(piece.c_str(), nullptr, 10);
            UINT mod = ModFromVk(code);
            if (mod) {
                mods |= mod;
            } else if (code) {
                vk = code;
            }
        }
        if (end == std::wstring::npos) {
            break;
        }
        start = end + 1;
    }

    if (!vk) {
        return false;
    }
    *outMods = mods;
    *outVk = vk;
    return true;
}

void CollectKeyboardManagerList(const JsonDoc& doc,
                                size_t arrayIndex,
                                ShortcutTable& table) {
    if (arrayIndex == SIZE_MAX || doc.At(arrayIndex).type != JsonType::array) {
        return;
    }

    for (size_t item : doc.At(arrayIndex).children) {
        if (doc.At(item).type != JsonType::object) {
            continue;
        }

        std::wstring original = doc.StringMember(item, L"originalKeys");
        UINT mods = 0;
        UINT vk = 0;
        if (original.empty() || !ParseVkList(original, &mods, &vk)) {
            continue;
        }

        std::wstring label;
        std::wstring program = doc.StringMember(item, L"runProgramFilePath");
        std::wstring text = doc.StringMember(item, L"unicodeText");
        std::wstring remapped = doc.StringMember(item, L"newRemapKeys");
        if (!program.empty()) {
            label = L"Run " + FileStem(program);
        } else if (!text.empty()) {
            label = L"Type \"" + Shorten(text, 24) + L"\"";
        } else if (!remapped.empty()) {
            UINT toMods = 0;
            UINT toVk = 0;
            label = ParseVkList(remapped, &toMods, &toVk)
                        ? L"Sends " + FormatShortcut(toMods, toVk)
                        : L"Remapped";
        } else {
            label = L"Remapped";
        }

        std::wstring app = doc.StringMember(item, L"targetApp");
        if (!app.empty() && app != L"AllApps") {
            label += L"  [" + Shorten(app, 20) + L"]";
        }

        table.Add(mods, vk, label, L"PowerToys - Keyboard Manager",
                  kPriPowerToys);
    }
}

void CollectKeyboardManager(const std::wstring& base, ShortcutTable& table) {
    JsonDoc doc;
    std::wstring path = JoinPath(base, L"Keyboard Manager\\default.json");
    if (!ParseJsonFile(path, doc) || !doc.ok) {
        return;
    }

    size_t shortcuts = doc.Member(doc.root, L"remapShortcuts");
    if (shortcuts == SIZE_MAX) {
        return;
    }
    CollectKeyboardManagerList(doc, doc.Member(shortcuts, L"global"), table);
    CollectKeyboardManagerList(doc, doc.Member(shortcuts, L"appSpecific"),
                               table);
}

void CollectPowerToys(ShortcutTable& table) {
    std::wstring base = ExpandEnv(L"%LOCALAPPDATA%\\Microsoft\\PowerToys");

    WIN32_FIND_DATAW find;
    HANDLE handle = FindFirstFileW(JoinPath(base, L"*").c_str(), &find);
    if (handle == INVALID_HANDLE_VALUE) {
        Wh_Log(L"PowerToys settings folder not found");
        return;
    }

    int modules = 0;
    do {
        std::wstring name = find.cFileName;
        if (name == L"." || name == L".." ||
            !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            continue;
        }
        JsonDoc doc;
        if (!ParseJsonFile(JoinPath(JoinPath(base, name), L"settings.json"),
                           doc) ||
            !doc.ok) {
            continue;
        }
        WalkPowerToysJson(doc, doc.root, std::wstring(), name, table);
        modules++;
    } while (FindNextFileW(handle, &find));
    FindClose(handle);

    CollectKeyboardManager(base, table);
    Wh_Log(L"PowerToys: %d modules read", modules);
}

////////////////////////////////////////////////////////////////////////////////
// Source: Windhawk's own mods.

// The readable name of an installed mod, from the @name line of its source.
std::wstring WindhawkModName(const std::wstring& modId) {
    // Mods written locally are keyed as "local@the-mod-id".
    std::wstring id = modId;
    size_t at = id.find(L'@');
    if (at != std::wstring::npos) {
        id = id.substr(at + 1);
    }

    static const PCWSTR kFolders[] = {L"ModsSource", L"Mods"};
    std::wstring root = ExpandEnv(L"%PROGRAMDATA%\\Windhawk");
    for (PCWSTR folder : kFolders) {
        std::wstring text;
        std::wstring path =
            JoinPath(JoinPath(root, folder), id + L".wh.cpp");
        if (!ReadTextFile(path, text, 64 * 1024)) {
            continue;
        }
        for (const std::wstring& line : SplitLines(text)) {
            std::wstring trimmed = Trim(line);
            if (trimmed.compare(0, 8, L"// @name") != 0) {
                continue;
            }
            // Skip "@name:en-US" and the like; the plain one is wanted.
            std::wstring rest = Trim(trimmed.substr(8));
            if (!rest.empty() && rest[0] == L':') {
                continue;
            }
            if (!rest.empty()) {
                return rest;
            }
        }
    }
    return id;
}

void CollectWindhawk(ShortcutTable& table) {
    HKEY modsKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Windhawk\\Engine\\Mods", 0,
                      KEY_READ, &modsKey) != ERROR_SUCCESS) {
        Wh_Log(L"Windhawk mod settings not found in the registry");
        return;
    }

    int found = 0;
    for (DWORD i = 0;; i++) {
        WCHAR modId[256];
        DWORD modIdLen = ARRAYSIZE(modId);
        if (RegEnumKeyExW(modsKey, i, modId, &modIdLen, nullptr, nullptr,
                          nullptr, nullptr) != ERROR_SUCCESS) {
            break;
        }

        HKEY settingsKey = nullptr;
        std::wstring sub = std::wstring(modId) + L"\\Settings";
        if (RegOpenKeyExW(modsKey, sub.c_str(), 0, KEY_READ, &settingsKey) !=
            ERROR_SUCCESS) {
            continue;
        }

        std::wstring friendly;
        for (DWORD j = 0;; j++) {
            WCHAR valueName[256];
            DWORD valueNameLen = ARRAYSIZE(valueName);
            WCHAR data[512];
            DWORD dataLen = sizeof(data);
            DWORD type = 0;
            if (RegEnumValueW(settingsKey, j, valueName, &valueNameLen, nullptr,
                              &type, (LPBYTE)data, &dataLen) != ERROR_SUCCESS) {
                break;
            }
            if (type != REG_SZ || dataLen < sizeof(WCHAR)) {
                continue;
            }

            std::wstring value(data, dataLen / sizeof(WCHAR));
            size_t nul = value.find(L'\0');
            if (nul != std::wstring::npos) {
                value.resize(nul);
            }

            UINT mods = 0;
            UINT vk = 0;
            if (!ParseShortcutText(value, &mods, &vk) ||
                !(mods & (kModWin | kModCtrl | kModAlt))) {
                continue;
            }

            if (friendly.empty()) {
                friendly = WindhawkModName(modId);
            }
            table.Add(mods, vk, Prettify(valueName),
                      L"Windhawk - " + friendly, kPriWindhawk);
            found++;
        }
        RegCloseKey(settingsKey);
    }
    RegCloseKey(modsKey);
    Wh_Log(L"Windhawk: %d hotkey settings", found);
}

////////////////////////////////////////////////////////////////////////////////
// Source: the "Shortcut key" field on .lnk files.

void AddShortcutFileHotkey(const std::wstring& path, ShortcutTable& table) {
    IShellLinkW* link = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                IID_IShellLinkW, (void**)&link)) ||
        !link) {
        return;
    }

    IPersistFile* persist = nullptr;
    if (SUCCEEDED(link->QueryInterface(IID_IPersistFile, (void**)&persist)) &&
        persist) {
        if (SUCCEEDED(persist->Load(path.c_str(), STGM_READ))) {
            WORD hotkey = 0;
            if (SUCCEEDED(link->GetHotkey(&hotkey)) && hotkey) {
                UINT vk = LOBYTE(hotkey);
                BYTE flags = HIBYTE(hotkey);
                UINT mods = 0;
                if (flags & HOTKEYF_SHIFT) {
                    mods |= kModShift;
                }
                if (flags & HOTKEYF_CONTROL) {
                    mods |= kModCtrl;
                }
                if (flags & HOTKEYF_ALT) {
                    mods |= kModAlt;
                }
                table.Add(mods, vk, FileStem(path), L"Shortcut file",
                          kPriShortcutFile);
            }
        }
        persist->Release();
    }
    link->Release();
}

void ScanFolderForLinks(const std::wstring& dir,
                        int depth,
                        int* budget,
                        ShortcutTable& table) {
    if (depth <= 0 || *budget <= 0) {
        return;
    }

    WIN32_FIND_DATAW find;
    HANDLE handle = FindFirstFileW(JoinPath(dir, L"*").c_str(), &find);
    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }

    std::vector<std::wstring> subFolders;
    do {
        if (*budget <= 0) {
            break;
        }
        std::wstring name = find.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            subFolders.push_back(JoinPath(dir, name));
            continue;
        }
        if (name.size() > 4 &&
            ToLower(name).compare(name.size() - 4, 4, L".lnk") == 0) {
            AddShortcutFileHotkey(JoinPath(dir, name), table);
            (*budget)--;
        }
    } while (FindNextFileW(handle, &find));
    FindClose(handle);

    for (const std::wstring& sub : subFolders) {
        ScanFolderForLinks(sub, depth - 1, budget, table);
    }
}

void CollectShortcutFiles(ShortcutTable& table) {
    static const PCWSTR kRoots[] = {
        L"%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs",
        L"%PROGRAMDATA%\\Microsoft\\Windows\\Start Menu\\Programs",
        L"%USERPROFILE%\\Desktop",
        L"%PUBLIC%\\Desktop",
    };

    int budget = 3000;
    for (PCWSTR root : kRoots) {
        ScanFolderForLinks(ExpandEnv(root), 5, &budget, table);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Source: what Windows itself does.

struct BuiltinShortcut {
    UINT mods;
    UINT vk;
    PCWSTR label;
};

const BuiltinShortcut kBuiltins[] = {
    {kModWin, 'A', L"Quick settings"},
    {kModWin, 'D', L"Show the desktop"},
    {kModWin, 'E', L"File Explorer"},
    {kModWin, 'G', L"Game bar"},
    {kModWin, 'H', L"Voice typing"},
    {kModWin, 'I', L"Settings"},
    {kModWin, 'K', L"Cast"},
    {kModWin, 'L', L"Lock the PC"},
    {kModWin, 'M', L"Minimise everything"},
    {kModWin, 'N', L"Notifications and calendar"},
    {kModWin, 'P', L"Project to another screen"},
    {kModWin, 'R', L"Run"},
    {kModWin, 'S', L"Search"},
    {kModWin, 'T', L"Cycle the taskbar"},
    {kModWin, 'U', L"Accessibility settings"},
    {kModWin, 'V', L"Clipboard history"},
    {kModWin, 'W', L"Widgets"},
    {kModWin, 'X', L"Quick link menu"},
    {kModWin, 'Z', L"Snap layouts"},
    {kModWin, VK_TAB, L"Task view"},
    {kModWin, VK_UP, L"Maximise"},
    {kModWin, VK_DOWN, L"Restore or minimise"},
    {kModWin, VK_LEFT, L"Snap left"},
    {kModWin, VK_RIGHT, L"Snap right"},
    {kModWin, VK_PAUSE, L"System properties"},
    {kModWin, VK_OEM_PERIOD, L"Emoji and symbols"},
    {kModWin, VK_OEM_PLUS, L"Zoom in with Magnifier"},
    {kModWin, VK_OEM_MINUS, L"Zoom out with Magnifier"},
    {kModWin | kModShift, 'S', L"Screenshot a region"},
    {kModWin | kModShift, VK_LEFT, L"Move to the screen on the left"},
    {kModWin | kModShift, VK_RIGHT, L"Move to the screen on the right"},
    {kModWin | kModCtrl, 'D', L"New virtual desktop"},
    {kModWin | kModCtrl, VK_F4, L"Close this virtual desktop"},
    {kModWin | kModCtrl, VK_LEFT, L"Previous virtual desktop"},
    {kModWin | kModCtrl, VK_RIGHT, L"Next virtual desktop"},

    {kModCtrl, 'A', L"Select all"},
    {kModCtrl, 'C', L"Copy"},
    {kModCtrl, 'F', L"Find"},
    {kModCtrl, 'N', L"New"},
    {kModCtrl, 'P', L"Print"},
    {kModCtrl, 'S', L"Save"},
    {kModCtrl, 'V', L"Paste"},
    {kModCtrl, 'W', L"Close"},
    {kModCtrl, 'X', L"Cut"},
    {kModCtrl, 'Y', L"Redo"},
    {kModCtrl, 'Z', L"Undo"},
    {kModCtrl, VK_F4, L"Close the document"},
    {kModCtrl | kModShift, VK_ESCAPE, L"Task Manager"},
    {kModCtrl | kModShift, 'N', L"New folder"},
    {kModCtrl | kModShift, 'T', L"Reopen the last tab"},

    {kModAlt, VK_TAB, L"Switch window"},
    {kModAlt, VK_F4, L"Close the window"},
    {kModAlt, VK_RETURN, L"Properties"},
    {kModAlt, VK_SPACE, L"Window menu"},
    {kModAlt, VK_LEFT, L"Back"},
    {kModAlt, VK_RIGHT, L"Forward"},
    {kModAlt, VK_UP, L"Up one folder"},
    {kModAlt | kModShift, VK_TAB, L"Switch window, backwards"},
};

void CollectBuiltins(ShortcutTable& table) {
    for (const BuiltinShortcut& entry : kBuiltins) {
        table.Add(entry.mods, entry.vk, entry.label, L"Windows", kPriBuiltin);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Source: asking Windows which combinations are already spoken for.
//
// There is no way to list the hotkeys other programs have registered, but there
// is a way to ask whether one is taken: try to register it. This says nothing
// about what owns it, and sees nothing of the hotkeys that work by keyboard
// hook - which is how AutoHotkey and PowerToys mostly work - so it is a last
// resort for the ones no config file accounts for, and is off by default.

void ProbeCombination(UINT mods, UINT vk, ShortcutTable& table) {
    UINT winMods = 0;
    if (mods & kModAlt) {
        winMods |= MOD_ALT;
    }
    if (mods & kModCtrl) {
        winMods |= MOD_CONTROL;
    }
    if (mods & kModShift) {
        winMods |= MOD_SHIFT;
    }
    if (mods & kModWin) {
        winMods |= MOD_WIN;
    }

    constexpr int kProbeId = 0x7F01;
    if (RegisterHotKey(nullptr, kProbeId, winMods, vk)) {
        // It was free. Give it straight back; the window in which we hold it is
        // a few microseconds, but it is still the user's key, not ours.
        UnregisterHotKey(nullptr, kProbeId);
        return;
    }
    if (GetLastError() == ERROR_HOTKEY_ALREADY_REGISTERED) {
        table.Add(mods, vk, L"In use by another app", L"Another app",
                  kPriProbe);
    }
}

void CollectProbed(ShortcutTable& table) {
    static const UINT kModSets[] = {
        kModWin,
        kModWin | kModShift,
        kModWin | kModCtrl,
        kModWin | kModAlt,
        kModCtrl | kModAlt,
        kModCtrl | kModAlt | kModShift,
        kModAlt | kModShift,
    };

    std::vector<UINT> keys;
    for (UINT vk = 'A'; vk <= 'Z'; vk++) {
        keys.push_back(vk);
    }
    for (UINT vk = '0'; vk <= '9'; vk++) {
        keys.push_back(vk);
    }
    for (UINT vk = VK_F1; vk <= VK_F12; vk++) {
        keys.push_back(vk);
    }

    for (UINT mods : kModSets) {
        for (UINT vk : keys) {
            // Nothing to learn where a source already named the owner.
            UINT64 key = ((UINT64)mods << 32) | vk;
            if (table.byCombo.find(key) != table.byCombo.end()) {
                continue;
            }
            ProbeCombination(mods, vk, table);
        }
    }

    // Anything that did land in our queue while probing is ours to drop.
    MSG msg;
    while (PeekMessageW(&msg, nullptr, WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) {
    }
}

////////////////////////////////////////////////////////////////////////////////
// The collected list, shared between the thread that builds it and the one
// that draws it.

CRITICAL_SECTION g_listLock;
std::vector<Shortcut> g_shortcuts;
HANDLE g_rebuildEvent;
HANDLE g_buildThread;
volatile LONG g_stopping;

void BuildShortcutList() {
    ShortcutTable table;

    // Order matters only in that a later source cannot displace an earlier one
    // of higher priority; Add settles that itself.
    CollectManual(table);
    if (g_settings.sourceAutoHotkey) {
        CollectAutoHotkey(table);
    }
    if (g_settings.sourcePowerToys) {
        CollectPowerToys(table);
    }
    if (g_settings.sourceWindhawk) {
        CollectWindhawk(table);
    }
    if (g_settings.sourceShortcutFiles) {
        CollectShortcutFiles(table);
    }
    if (g_settings.showBuiltins) {
        CollectBuiltins(table);
    }
    if (g_settings.probeOtherApps) {
        CollectProbed(table);
    }

    std::vector<Shortcut> sorted = table.Sorted();
    Wh_Log(L"collected %d shortcuts", (int)sorted.size());

    EnterCriticalSection(&g_listLock);
    g_shortcuts.swap(sorted);
    LeaveCriticalSection(&g_listLock);
}

DWORD WINAPI BuildThreadProc(LPVOID param) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // A message queue has to exist before the probe can register anything.
    MSG msg;
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    for (;;) {
        BuildShortcutList();
        if (InterlockedCompareExchange(&g_stopping, 0, 0)) {
            break;
        }
        DWORD wait = g_settings.refreshMinutes > 0
                         ? (DWORD)g_settings.refreshMinutes * 60000
                         : INFINITE;
        WaitForSingleObject(g_rebuildEvent, wait);
        if (InterlockedCompareExchange(&g_stopping, 0, 0)) {
            break;
        }
    }

    CoUninitialize();
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Drawing the guide.

ULONG_PTR g_gdiplusToken;
HINSTANCE g_hInst;
HWND g_overlayWnd;
bool g_overlayVisible;
UINT g_overlayMods;

bool IsLightTheme() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\"
                     L"Personalize",
                     L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value,
                     &size) != ERROR_SUCCESS) {
        return false;
    }
    return value != 0;
}

UINT DpiForWindowOrSystem(HWND hwnd) {
    if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        auto pGetDpiForWindow =
            (UINT(WINAPI*)(HWND))GetProcAddress(user32, "GetDpiForWindow");
        if (pGetDpiForWindow && hwnd) {
            UINT dpi = pGetDpiForWindow(hwnd);
            if (dpi) {
                return dpi;
            }
        }
    }
    HDC screen = GetDC(nullptr);
    UINT dpi = screen ? (UINT)GetDeviceCaps(screen, LOGPIXELSX) : 96;
    if (screen) {
        ReleaseDC(nullptr, screen);
    }
    return dpi ? dpi : 96;
}

// The shortcuts to show for the modifiers currently held: every combination
// that includes them. Holding Win lists Win+Shift+S as well, and pressing Shift
// while it is up narrows the list to the ones that need Shift.
std::vector<Shortcut> ShortcutsFor(UINT heldMods) {
    std::vector<Shortcut> matching;

    EnterCriticalSection(&g_listLock);
    for (const Shortcut& entry : g_shortcuts) {
        if ((entry.mods & heldMods) == heldMods) {
            matching.push_back(entry);
        }
    }
    LeaveCriticalSection(&g_listLock);

    // Exactly what is held first, then the combinations that add to it; within
    // each, by the modifiers and then the key, as Sorted already arranged.
    std::stable_sort(matching.begin(), matching.end(),
                     [heldMods](const Shortcut& a, const Shortcut& b) {
                         bool exactA = a.mods == heldMods;
                         bool exactB = b.mods == heldMods;
                         if (exactA != exactB) {
                             return exactA;
                         }
                         return false;
                     });
    return matching;
}

void AddRoundedRect(Gdiplus::GraphicsPath& path,
                    const Gdiplus::RectF& rect,
                    float radius) {
    float d = radius * 2.0f;
    path.AddArc(rect.X, rect.Y, d, d, 180.0f, 90.0f);
    path.AddArc(rect.GetRight() - d, rect.Y, d, d, 270.0f, 90.0f);
    path.AddArc(rect.GetRight() - d, rect.GetBottom() - d, d, d, 0.0f, 90.0f);
    path.AddArc(rect.X, rect.GetBottom() - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

struct GuideLayout {
    int columns;
    int rows;
    int comboWidth;
    int labelWidth;
    int rowHeight;
    int width;
    int height;
    size_t shown;
};

// Works out how many columns the list can be laid out in without running off
// the screen, and how wide each half of a row has to be.
GuideLayout MeasureGuide(Gdiplus::Graphics& graphics,
                         const Gdiplus::Font& font,
                         const Gdiplus::Font& titleFont,
                         const std::vector<Shortcut>& entries,
                         int padding,
                         int maxWidth,
                         int maxHeight) {
    GuideLayout layout = {};
    layout.rowHeight = 0;

    Gdiplus::RectF bounds;
    float comboMax = 0;
    float labelMax = 0;
    float lineHeight = 0;
    for (const Shortcut& entry : entries) {
        std::wstring combo = FormatShortcut(entry.mods, entry.vk);
        graphics.MeasureString(combo.c_str(), -1, &font,
                               Gdiplus::PointF(0, 0), &bounds);
        comboMax = (bounds.Width > comboMax) ? bounds.Width : comboMax;
        lineHeight = (bounds.Height > lineHeight) ? bounds.Height : lineHeight;

        graphics.MeasureString(entry.label.c_str(), -1, &font,
                               Gdiplus::PointF(0, 0), &bounds);
        labelMax = (bounds.Width > labelMax) ? bounds.Width : labelMax;
    }
    if (lineHeight <= 0) {
        lineHeight = 18;
    }

    int labelCap = maxWidth / 3;
    layout.comboWidth = (int)comboMax + 12;
    layout.labelWidth = (int)labelMax + 12;
    if (layout.labelWidth > labelCap) {
        layout.labelWidth = labelCap;
    }
    layout.rowHeight = (int)(lineHeight * 1.45f);

    Gdiplus::RectF titleBounds;
    graphics.MeasureString(L"Wg", -1, &titleFont, Gdiplus::PointF(0, 0),
                           &titleBounds);
    int titleHeight = (int)(titleBounds.Height * 1.9f);

    int columnWidth = layout.comboWidth + layout.labelWidth + padding;
    int availableHeight = maxHeight - padding * 2 - titleHeight;
    int rowsPerColumn = availableHeight / layout.rowHeight;
    if (rowsPerColumn < 1) {
        rowsPerColumn = 1;
    }

    int count = (int)entries.size();
    int columns = 1;
    while (columns < g_settings.maxColumns &&
           columns * rowsPerColumn < count &&
           (columns + 1) * columnWidth + padding * 2 <= maxWidth) {
        columns++;
    }

    layout.columns = columns;
    layout.rows = (count + columns - 1) / columns;
    if (layout.rows > rowsPerColumn) {
        layout.rows = rowsPerColumn;
    }
    if (layout.rows < 1) {
        layout.rows = 1;
    }
    layout.shown = (size_t)(layout.rows * columns);
    if (layout.shown > entries.size()) {
        layout.shown = entries.size();
    }

    layout.width = columns * columnWidth + padding * 2 - padding;
    layout.height = titleHeight + layout.rows * layout.rowHeight + padding * 2;
    if (layout.shown < entries.size()) {
        layout.height += layout.rowHeight;  // Room for the "and N more" line.
    }
    return layout;
}

void RenderGuide(const std::vector<Shortcut>& entries, UINT heldMods) {
    HMONITOR monitor = MonitorFromWindow(GetForegroundWindow(),
                                         MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo = {sizeof(monitorInfo)};
    if (!GetMonitorInfoW(monitor, &monitorInfo)) {
        return;
    }
    RECT work = monitorInfo.rcWork;
    int workWidth = work.right - work.left;
    int workHeight = work.bottom - work.top;

    UINT dpi = DpiForWindowOrSystem(GetForegroundWindow());
    int padding = MulDiv(22, dpi, 96);
    float em = (float)MulDiv(g_settings.fontSize, dpi, 96);

    bool light = IsLightTheme();
    Gdiplus::Color background =
        light ? Gdiplus::Color(245, 250, 250, 250)
              : Gdiplus::Color(240, 32, 32, 34);
    Gdiplus::Color border = light ? Gdiplus::Color(60, 0, 0, 0)
                                  : Gdiplus::Color(70, 255, 255, 255);
    Gdiplus::Color comboColor = light ? Gdiplus::Color(255, 0, 90, 170)
                                      : Gdiplus::Color(255, 120, 190, 255);
    Gdiplus::Color textColor = light ? Gdiplus::Color(255, 20, 20, 20)
                                     : Gdiplus::Color(255, 235, 235, 235);
    Gdiplus::Color dimColor = light ? Gdiplus::Color(255, 110, 110, 110)
                                    : Gdiplus::Color(255, 150, 150, 150);

    Gdiplus::FontFamily family(L"Segoe UI");
    Gdiplus::Font font(&family, em, Gdiplus::FontStyleRegular,
                       Gdiplus::UnitPixel);
    Gdiplus::Font titleFont(&family, em * 1.25f, Gdiplus::FontStyleBold,
                            Gdiplus::UnitPixel);

    // Measure against a throwaway surface before the real one can be sized.
    HDC screenDC = GetDC(nullptr);
    GuideLayout layout;
    {
        Gdiplus::Graphics measure(screenDC);
        measure.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
        layout = MeasureGuide(measure, font, titleFont, entries, padding,
                              workWidth - padding * 4,
                              workHeight - padding * 4);
    }

    int width = layout.width;
    int height = layout.height;
    if (width < MulDiv(260, dpi, 96)) {
        width = MulDiv(260, dpi, 96);
    }
    if (width > workWidth) {
        width = workWidth;
    }
    if (height > workHeight) {
        height = workHeight;
    }

    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;  // Top-down.
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP dib = CreateDIBSection(screenDC, &bitmapInfo, DIB_RGB_COLORS, &bits,
                                   nullptr, 0);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, dib);

    {
        Gdiplus::Graphics graphics(memDC);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
        graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

        float radius = (float)MulDiv(12, dpi, 96);
        Gdiplus::RectF panel(0.5f, 0.5f, (float)width - 1.0f,
                             (float)height - 1.0f);
        Gdiplus::GraphicsPath path;
        AddRoundedRect(path, panel, radius);
        Gdiplus::SolidBrush backBrush(background);
        graphics.FillPath(&backBrush, &path);
        Gdiplus::Pen borderPen(border, 1.0f);
        graphics.DrawPath(&borderPen, &path);

        Gdiplus::SolidBrush comboBrush(comboColor);
        Gdiplus::SolidBrush textBrush(textColor);
        Gdiplus::SolidBrush dimBrush(dimColor);

        std::wstring title = ModsName(heldMods);
        if (title.size() > 3) {
            title.resize(title.size() - 3);  // Drop the trailing " + ".
        }
        title += L"  \u2014  " + std::to_wstring((int)entries.size()) +
                 L" shortcuts";
        graphics.DrawString(title.c_str(), -1, &titleFont,
                            Gdiplus::PointF((float)padding, (float)padding),
                            &textBrush);

        Gdiplus::RectF titleBounds;
        graphics.MeasureString(L"Wg", -1, &titleFont, Gdiplus::PointF(0, 0),
                               &titleBounds);
        int top = padding + (int)(titleBounds.Height * 1.9f);

        Gdiplus::StringFormat format;
        format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
        format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);

        int columnWidth = layout.comboWidth + layout.labelWidth + padding;
        for (size_t i = 0; i < layout.shown; i++) {
            int column = (int)(i / (size_t)layout.rows);
            int row = (int)(i % (size_t)layout.rows);
            float x = (float)(padding + column * columnWidth);
            float y = (float)(top + row * layout.rowHeight);

            std::wstring combo =
                FormatShortcut(entries[i].mods, entries[i].vk);
            Gdiplus::RectF comboRect(x, y, (float)layout.comboWidth,
                                     (float)layout.rowHeight);
            graphics.DrawString(combo.c_str(), -1, &font, comboRect, &format,
                                &comboBrush);

            Gdiplus::RectF labelRect(x + layout.comboWidth, y,
                                     (float)layout.labelWidth,
                                     (float)layout.rowHeight);
            graphics.DrawString(entries[i].label.c_str(), -1, &font, labelRect,
                                &format, &textBrush);
        }

        if (layout.shown < entries.size()) {
            std::wstring more =
                L"and " + std::to_wstring((int)(entries.size() - layout.shown)) +
                L" more";
            graphics.DrawString(
                more.c_str(), -1, &font,
                Gdiplus::PointF((float)padding,
                                (float)(top + layout.rows * layout.rowHeight)),
                &dimBrush);
        }
    }

    int x = work.left + (workWidth - width) / 2;
    int y = work.top + (workHeight - height) / 2;
    POINT destination = {x, y};
    SIZE size = {width, height};
    POINT source = {0, 0};
    BYTE alpha = (BYTE)((g_settings.opacity * 255 + 50) / 100);
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
    UpdateLayeredWindow(g_overlayWnd, screenDC, &destination, &size, memDC,
                        &source, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBitmap);
    DeleteObject(dib);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}

void ShowGuide(UINT heldMods) {
    std::vector<Shortcut> entries = ShortcutsFor(heldMods);
    if (entries.empty()) {
        return;
    }

    RenderGuide(entries, heldMods);
    SetWindowPos(g_overlayWnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    g_overlayVisible = true;
    g_overlayMods = heldMods;
}

void HideGuide() {
    if (!g_overlayVisible) {
        return;
    }
    ShowWindow(g_overlayWnd, SW_HIDE);
    g_overlayVisible = false;
    g_overlayMods = 0;
}

////////////////////////////////////////////////////////////////////////////////
// Watching the modifiers.
//
// The hook does not look at which key was pressed beyond whether it is a
// modifier, and nothing about a keystroke is recorded or kept. Every key is
// passed straight on: the guide only ever appears beside what you were going to
// do anyway.

constexpr UINT_PTR kPollTimerId = 1;
constexpr UINT kPollIntervalMs = 50;
constexpr UINT WM_APP_INPUT = WM_APP + 1;
constexpr UINT WM_APP_SETTINGS = WM_APP + 2;
constexpr UINT WM_APP_QUIT = WM_APP + 3;

HHOOK g_keyboardHook;
HWND g_sinkWnd;
DWORD g_uiThreadId;
HANDLE g_uiThread;
HANDLE g_readyEvent;
ULONGLONG g_armedSince;
// Set when an ordinary key is pressed, so a modifier that was only part of a
// shortcut does not bring the guide up afterwards. Cleared once everything is
// released.
bool g_keyUsed;

// Asking the keyboard, rather than counting the hook's events, is what keeps
// this honest: a key-up swallowed by another hook would otherwise leave a
// modifier stuck down for ever.
UINT CurrentMods() {
    UINT mods = 0;
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) ||
        (GetAsyncKeyState(VK_RWIN) & 0x8000)) {
        mods |= kModWin;
    }
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
        mods |= kModCtrl;
    }
    if (GetAsyncKeyState(VK_MENU) & 0x8000) {
        mods |= kModAlt;
    }
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        mods |= kModShift;
    }
    return mods;
}

bool TriggerAllowed(UINT mods) {
    if ((mods & kModWin) && g_settings.showWin) {
        return true;
    }
    // Win takes precedence: Win+Ctrl is a Win shortcut, not a Ctrl one.
    if (mods & kModWin) {
        return false;
    }
    if ((mods & kModCtrl) && g_settings.showCtrl) {
        return true;
    }
    if ((mods & kModAlt) && g_settings.showAlt) {
        return true;
    }
    return false;
}

void UpdateGuideState() {
    UINT mods = CurrentMods();

    if (!mods) {
        g_keyUsed = false;
        g_armedSince = 0;
        HideGuide();
        KillTimer(g_sinkWnd, kPollTimerId);
        return;
    }

    if (g_keyUsed || !TriggerAllowed(mods)) {
        g_armedSince = 0;
        HideGuide();
        return;
    }

    SetTimer(g_sinkWnd, kPollTimerId, kPollIntervalMs, nullptr);

    if (g_overlayVisible) {
        if (mods != g_overlayMods) {
            ShowGuide(mods);
        }
        return;
    }

    ULONGLONG now = GetTickCount64();
    if (!g_armedSince) {
        g_armedSince = now;
        return;
    }
    if (now - g_armedSince >= (ULONGLONG)g_settings.holdDelayMs) {
        ShowGuide(mods);
    }
}

LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        KBDLLHOOKSTRUCT* info = (KBDLLHOOKSTRUCT*)lParam;
        bool down = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
        if (down && !ModFromVk(info->vkCode)) {
            g_keyUsed = true;
        }
        // The work is done on the message loop, not here: a low level hook sits
        // in front of every keystroke on the system and must return at once.
        PostMessageW(g_sinkWnd, WM_APP_INPUT, 0, 0);
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

LRESULT CALLBACK SinkWndProc(HWND hwnd,
                             UINT message,
                             WPARAM wParam,
                             LPARAM lParam) {
    switch (message) {
        case WM_APP_INPUT:
            UpdateGuideState();
            return 0;
        case WM_TIMER:
            if (wParam == kPollTimerId) {
                UpdateGuideState();
                return 0;
            }
            break;
        case WM_APP_SETTINGS:
            LoadSettings();
            HideGuide();
            SetEvent(g_rebuildEvent);
            return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd,
                                UINT message,
                                WPARAM wParam,
                                LPARAM lParam) {
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// The UI thread and the mod's lifecycle.

DWORD WINAPI UiThreadProc(LPVOID param) {
    if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        auto pSetThreadDpiAwarenessContext =
            (void*(WINAPI*)(void*))GetProcAddress(
                user32, "SetThreadDpiAwarenessContext");
        if (pSetThreadDpiAwarenessContext) {
            // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2.
            pSetThreadDpiAwarenessContext((void*)(LONG_PTR)-4);
        }
    }

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

    WNDCLASSEXW overlayClass = {sizeof(overlayClass)};
    overlayClass.lpfnWndProc = OverlayWndProc;
    overlayClass.hInstance = g_hInst;
    overlayClass.lpszClassName = L"WH_ShortcutGuidePlus";
    RegisterClassExW(&overlayClass);

    WNDCLASSEXW sinkClass = {sizeof(sinkClass)};
    sinkClass.lpfnWndProc = SinkWndProc;
    sinkClass.hInstance = g_hInst;
    sinkClass.lpszClassName = L"WH_ShortcutGuidePlusSink";
    RegisterClassExW(&sinkClass);

    g_overlayWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW |
            WS_EX_NOACTIVATE | WS_EX_TOPMOST,
        L"WH_ShortcutGuidePlus", L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
        g_hInst, nullptr);

    g_sinkWnd = CreateWindowExW(0, L"WH_ShortcutGuidePlusSink", L"", 0, 0, 0, 0,
                                0, HWND_MESSAGE, nullptr, g_hInst, nullptr);

    g_keyboardHook =
        SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    if (!g_keyboardHook) {
        Wh_Log(L"SetWindowsHookEx failed, le=%lu", GetLastError());
    }

    SetEvent(g_readyEvent);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.hwnd == nullptr && msg.message == WM_APP_QUIT) {
            PostQuitMessage(0);
            continue;
        }
        if (msg.hwnd == nullptr && msg.message == WM_APP_SETTINGS) {
            LoadSettings();
            HideGuide();
            SetEvent(g_rebuildEvent);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
    if (g_overlayWnd) {
        DestroyWindow(g_overlayWnd);
        g_overlayWnd = nullptr;
    }
    if (g_sinkWnd) {
        DestroyWindow(g_sinkWnd);
        g_sinkWnd = nullptr;
    }
    if (g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
    return 0;
}

BOOL WhTool_ModInit() {
    g_hInst = GetModuleHandleW(nullptr);

    LoadSettings();
    InitializeCriticalSection(&g_listLock);

    g_readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_rebuildEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_readyEvent || !g_rebuildEvent) {
        return FALSE;
    }

    g_uiThread =
        CreateThread(nullptr, 0, UiThreadProc, nullptr, 0, &g_uiThreadId);
    if (!g_uiThread) {
        return FALSE;
    }
    WaitForSingleObject(g_readyEvent, 5000);

    g_buildThread = CreateThread(nullptr, 0, BuildThreadProc, nullptr, 0,
                                 nullptr);
    return TRUE;
}

void WhTool_ModSettingsChanged() {
    if (g_uiThreadId) {
        PostThreadMessageW(g_uiThreadId, WM_APP_SETTINGS, 0, 0);
    }
}

void WhTool_ModUninit() {
    InterlockedExchange(&g_stopping, 1);
    if (g_rebuildEvent) {
        SetEvent(g_rebuildEvent);
    }
    if (g_buildThread) {
        WaitForSingleObject(g_buildThread, 5000);
        CloseHandle(g_buildThread);
        g_buildThread = nullptr;
    }

    if (g_uiThreadId) {
        PostThreadMessageW(g_uiThreadId, WM_APP_QUIT, 0, 0);
    }
    if (g_uiThread) {
        if (WaitForSingleObject(g_uiThread, 5000) != WAIT_OBJECT_0) {
            Wh_Log(L"UI thread did not exit in time");
            ExitProcess(1);
        }
        CloseHandle(g_uiThread);
        g_uiThread = nullptr;
        g_uiThreadId = 0;
    }

    if (g_rebuildEvent) {
        CloseHandle(g_rebuildEvent);
        g_rebuildEvent = nullptr;
    }
    if (g_readyEvent) {
        CloseHandle(g_readyEvent);
        g_readyEvent = nullptr;
    }
    DeleteCriticalSection(&g_listLock);
}

////////////////////////////////////////////////////////////////////////////////
// Windhawk tool mod implementation for mods which don't need to inject to other
// processes or hook other functions. Context:
// https://github.com/ramensoftware/windhawk/wiki/Mods-as-tools:-Running-mods-in-a-dedicated-process
//
// The mod will load and run in a dedicated windhawk.exe process.
//
// Paste the code below as part of the mod code, and use these callbacks:
// * WhTool_ModInit
// * WhTool_ModSettingsChanged
// * WhTool_ModUninit
//
// Currently, other callbacks are not supported.

bool g_isToolModProcessLauncher;
HANDLE g_toolModProcessMutex;

void WINAPI EntryPoint_Hook() {
    Wh_Log(L">");
    ExitThread(0);
}

BOOL Wh_ModInit() {
    DWORD sessionId;
    if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId) &&
        sessionId == 0) {
        return FALSE;
    }

    bool isExcluded = false;
    bool isToolModProcess = false;
    bool isCurrentToolModProcess = false;
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (!argv) {
        Wh_Log(L"CommandLineToArgvW failed");
        return FALSE;
    }

    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"-service") == 0 ||
            wcscmp(argv[i], L"-service-start") == 0 ||
            wcscmp(argv[i], L"-service-stop") == 0) {
            isExcluded = true;
            break;
        }
    }

    for (int i = 1; i < argc - 1; i++) {
        if (wcscmp(argv[i], L"-tool-mod") == 0) {
            isToolModProcess = true;
            if (wcscmp(argv[i + 1], WH_MOD_ID) == 0) {
                isCurrentToolModProcess = true;
            }
            break;
        }
    }

    LocalFree(argv);

    if (isExcluded) {
        return FALSE;
    }

    if (isCurrentToolModProcess) {
        g_toolModProcessMutex =
            CreateMutex(nullptr, TRUE, L"windhawk-tool-mod_" WH_MOD_ID);
        if (!g_toolModProcessMutex) {
            Wh_Log(L"CreateMutex failed");
            ExitProcess(1);
        }

        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            Wh_Log(L"Tool mod already running (%s)", WH_MOD_ID);
            ExitProcess(1);
        }

        if (!WhTool_ModInit()) {
            ExitProcess(1);
        }

        IMAGE_DOS_HEADER* dosHeader =
            (IMAGE_DOS_HEADER*)GetModuleHandle(nullptr);
        IMAGE_NT_HEADERS* ntHeaders =
            (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);

        DWORD entryPointRVA = ntHeaders->OptionalHeader.AddressOfEntryPoint;
        void* entryPoint = (BYTE*)dosHeader + entryPointRVA;

        Wh_SetFunctionHook(entryPoint, (void*)EntryPoint_Hook, nullptr);
        return TRUE;
    }

    if (isToolModProcess) {
        return FALSE;
    }

    g_isToolModProcessLauncher = true;
    return TRUE;
}

void Wh_ModAfterInit() {
    if (!g_isToolModProcessLauncher) {
        return;
    }

    WCHAR currentProcessPath[MAX_PATH];
    switch (GetModuleFileName(nullptr, currentProcessPath,
                              ARRAYSIZE(currentProcessPath))) {
        case 0:
        case ARRAYSIZE(currentProcessPath):
            Wh_Log(L"GetModuleFileName failed");
            return;
    }

    WCHAR
    commandLine[MAX_PATH + 2 +
                (sizeof(L" -tool-mod \"" WH_MOD_ID "\"") / sizeof(WCHAR)) - 1];
    swprintf_s(commandLine, L"\"%s\" -tool-mod \"%s\"", currentProcessPath,
               WH_MOD_ID);

    HMODULE kernelModule = GetModuleHandle(L"kernelbase.dll");
    if (!kernelModule) {
        kernelModule = GetModuleHandle(L"kernel32.dll");
        if (!kernelModule) {
            Wh_Log(L"No kernelbase.dll/kernel32.dll");
            return;
        }
    }

    using CreateProcessInternalW_t = BOOL(WINAPI*)(
        HANDLE hUserToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes, WINBOOL bInheritHandles,
        DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
        LPSTARTUPINFOW lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation,
        PHANDLE hRestrictedUserToken);
    CreateProcessInternalW_t pCreateProcessInternalW =
        (CreateProcessInternalW_t)GetProcAddress(kernelModule,
                                                 "CreateProcessInternalW");
    if (!pCreateProcessInternalW) {
        Wh_Log(L"No CreateProcessInternalW");
        return;
    }

    STARTUPINFO si{
        .cb = sizeof(STARTUPINFO),
        .dwFlags = STARTF_FORCEOFFFEEDBACK,
    };
    PROCESS_INFORMATION pi;
    if (!pCreateProcessInternalW(nullptr, currentProcessPath, commandLine,
                                 nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS,
                                 nullptr, nullptr, &si, &pi, nullptr)) {
        Wh_Log(L"CreateProcess failed");
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void Wh_ModSettingsChanged() {
    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModSettingsChanged();
}

void Wh_ModUninit() {
    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModUninit();
    ExitProcess(0);
}
