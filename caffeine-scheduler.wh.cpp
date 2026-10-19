// ==WindhawkMod==
// @id              caffeine-scheduler
// @name            Caffeine Scheduler
// @description     Keep the PC awake on a schedule - set the hours and days, with a tray icon to override it
// @version         1.1
// @author          aariv
// @include         windhawk.exe
// @compilerOptions -lgdi32 -lgdiplus -luser32 -lshell32 -lole32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Caffeine Scheduler

Stops the PC sleeping and the screen switching off - but only during the hours
you say. Write the times once and it looks after itself; the tray icon is there
for when you want to overrule it.

## The schedule

One rule per line, days then times:

```
Mon-Fri 09:00-17:00
Sat,Sun 10:00-14:00
Daily 22:00-06:00
```

* **Days** can be a range (`Mon-Fri`), a list (`Mon,Wed,Fri`), both at once
  (`Mon-Thu,Sat`), or one of `Daily`, `Weekdays`, `Weekends`.
* **Times** are 24-hour. A rule whose end is earlier than its start runs through
  midnight, so `22:00-06:00` means ten at night until six the next morning.
* Blank lines, and lines starting with `#` or `;`, are ignored.

The PC is kept awake whenever **any** rule covers the current moment. With no
rules at all, nothing happens until you use the tray icon.

## The tray icon

* **Left-click** toggles it, overruling the schedule.
* **Right-click** gives a menu: keep awake for a set time, follow the schedule
  again, or turn the schedule off entirely.
* **Hover** to see what it is doing, why, and when that changes next.

By default an override lasts only **until the schedule would next change state**
- so turning it off during your 9-to-5 rule lasts until 17:00, and then normal
service resumes rather than the override quietly persisting for days. If you
would rather an override stay put until you undo it yourself, there is a setting
for that.

## How it keeps the PC awake

Through `SetThreadExecutionState`, the documented Windows API for it, which is
the same mechanism Windows' own media players use. Nothing simulates keypresses
or moves the mouse, so it will not interfere with anything you are doing, and it
shows up in `powercfg /requests` like any other legitimate request.

Inspired by the **Caffeine** mod by ALMAS CP, which is the one to use if you
only want the tray toggle without any of the scheduling.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- schedule:
  - "Mon-Fri 09:00-17:00"
  $name: Schedule
  $description: >-
    One rule per line, as "Mon-Fri 09:00-17:00". Days can be a range, a list, or
    Daily / Weekdays / Weekends. Times are 24-hour, and an end earlier than the
    start runs through midnight. Lines starting with # or ; are ignored.
- scheduleEnabled: true
  $name: Follow the schedule
  $description: >-
    Turn this off to leave the PC alone except when you switch it on from the
    tray icon.
- preventSleep: true
  $name: Keep the system awake
- preventDisplayOff: true
  $name: Keep the display on
  $description: >-
    Turn this off to let the screen switch off while still stopping the machine
    from sleeping - useful for a long download overnight.
- overrideLasts: nextChange
  $name: A tray override lasts
  $options:
  - nextChange: Until the schedule would next change
  - untilUndone: Until I change it back myself
- showTrayIcon: true
  $name: Show the tray icon
  $description: >-
    With this off the schedule still runs, but there is no way to overrule it.
- activeColor: "#FFB400"
  $name: Colour when awake
  $description: Hex colour of the tray icon while the PC is being kept awake.
- inactiveColor: "#808080"
  $name: Colour when idle
  $description: Hex colour of the tray icon while the PC is left alone.
- iconSize: 16
  $name: Tray icon size
- logVerbose: false
  $name: Verbose logging
  $description: Log every decision, for working out why it did something.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <gdiplus.h>
#include <shellapi.h>
#include <windowsx.h>

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <algorithm>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// Settings.

enum class OverrideLasts {
    nextChange,
    untilUndone,
};

struct {
    std::vector<std::wstring> schedule;
    bool scheduleEnabled;
    bool preventSleep;
    bool preventDisplayOff;
    OverrideLasts overrideLasts;
    bool showTrayIcon;
    COLORREF activeColor;
    COLORREF inactiveColor;
    int iconSize;
    bool logVerbose;
} g_settings;

// Logs only when the verbose setting is on. Every decision this mod makes is
// invisible by nature - the PC either slept or it did not - so there is a lot
// worth being able to switch on when something looks wrong.
void LogVerbose(PCWSTR message) {
    if (g_settings.logVerbose) {
        Wh_Log(L"%s", message);
    }
}

int ClampInt(int value, int low, int high) {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

std::wstring Trim(const std::wstring& text) {
    size_t begin = text.find_first_not_of(L" \t\r\n");
    if (begin == std::wstring::npos) {
        return std::wstring();
    }
    size_t end = text.find_last_not_of(L" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::wstring ToLower(const std::wstring& text) {
    std::wstring result = text;
    for (wchar_t& c : result) {
        if (c >= L'A' && c <= L'Z') {
            c = (wchar_t)(c - L'A' + L'a');
        }
    }
    return result;
}

// "#FFB400" or "FFB400" to a COLORREF. Returns `fallback` if it is not a colour.
COLORREF ParseHexColor(const std::wstring& text, COLORREF fallback) {
    std::wstring hex = Trim(text);
    if (!hex.empty() && hex[0] == L'#') {
        hex = hex.substr(1);
    }
    if (hex.size() != 6) {
        return fallback;
    }
    for (wchar_t c : hex) {
        bool isHexDigit = (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') ||
                          (c >= L'A' && c <= L'F');
        if (!isHexDigit) {
            return fallback;
        }
    }
    unsigned long value = wcstoul(hex.c_str(), nullptr, 16);
    return RGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
}

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
    LoadStringList(L"schedule[%d]", g_settings.schedule);

    g_settings.scheduleEnabled = Wh_GetIntSetting(L"scheduleEnabled");
    g_settings.preventSleep = Wh_GetIntSetting(L"preventSleep");
    g_settings.preventDisplayOff = Wh_GetIntSetting(L"preventDisplayOff");
    g_settings.showTrayIcon = Wh_GetIntSetting(L"showTrayIcon");
    g_settings.iconSize = ClampInt(Wh_GetIntSetting(L"iconSize"), 8, 64);
    g_settings.logVerbose = Wh_GetIntSetting(L"logVerbose");

    PCWSTR overrideLasts = Wh_GetStringSetting(L"overrideLasts");
    g_settings.overrideLasts = (overrideLasts && wcscmp(overrideLasts,
                                                        L"untilUndone") == 0)
                                   ? OverrideLasts::untilUndone
                                   : OverrideLasts::nextChange;
    Wh_FreeStringSetting(overrideLasts);

    PCWSTR activeColor = Wh_GetStringSetting(L"activeColor");
    g_settings.activeColor =
        ParseHexColor(activeColor ? activeColor : L"", RGB(255, 180, 0));
    Wh_FreeStringSetting(activeColor);

    PCWSTR inactiveColor = Wh_GetStringSetting(L"inactiveColor");
    g_settings.inactiveColor =
        ParseHexColor(inactiveColor ? inactiveColor : L"", RGB(128, 128, 128));
    Wh_FreeStringSetting(inactiveColor);
}

////////////////////////////////////////////////////////////////////////////////
// The schedule.
//
// A rule is a set of weekdays and a span of minutes within the day. A span
// whose end is at or before its start runs through midnight, which is handled
// by treating it as two pieces: from the start to midnight on each of its own
// days, and from midnight to the end on each following day.

struct ScheduleRule {
    unsigned char days;  // Bit 0 is Sunday, matching SYSTEMTIME's wDayOfWeek.
    int startMinute;
    int endMinute;
};

std::vector<ScheduleRule> g_rules;

// The weekday a name means as a bit, or 0. Accepts the three-letter form and
// the full name.
unsigned char DayBitFromName(const std::wstring& rawName) {
    static const PCWSTR kShort[7] = {L"sun", L"mon", L"tue", L"wed",
                                     L"thu", L"fri", L"sat"};
    static const PCWSTR kLong[7] = {L"sunday",   L"monday", L"tuesday",
                                    L"wednesday", L"thursday", L"friday",
                                    L"saturday"};

    std::wstring name = ToLower(Trim(rawName));
    for (int i = 0; i < 7; i++) {
        if (name == kShort[i] || name == kLong[i]) {
            return (unsigned char)(1 << i);
        }
    }
    return 0;
}

// Reads the days half of a rule: "Mon-Fri", "Sat,Sun", "Mon-Thu,Sat", "Daily".
bool ParseDays(const std::wstring& text, unsigned char* outDays) {
    std::wstring lower = ToLower(Trim(text));
    if (lower == L"daily" || lower == L"everyday" || lower == L"every" ||
        lower == L"all" || lower == L"*") {
        *outDays = 0x7F;
        return true;
    }
    if (lower == L"weekdays" || lower == L"weekday") {
        *outDays = 0x3E;  // Monday to Friday.
        return true;
    }
    if (lower == L"weekends" || lower == L"weekend") {
        *outDays = 0x41;  // Saturday and Sunday.
        return true;
    }

    unsigned char days = 0;
    size_t start = 0;
    while (start <= lower.size()) {
        size_t comma = lower.find(L',', start);
        std::wstring piece = lower.substr(
            start, comma == std::wstring::npos ? std::wstring::npos
                                               : comma - start);
        piece = Trim(piece);

        if (!piece.empty()) {
            size_t dash = piece.find(L'-');
            if (dash == std::wstring::npos) {
                unsigned char bit = DayBitFromName(piece);
                if (!bit) {
                    return false;
                }
                days |= bit;
            } else {
                unsigned char fromBit = DayBitFromName(piece.substr(0, dash));
                unsigned char toBit = DayBitFromName(piece.substr(dash + 1));
                if (!fromBit || !toBit) {
                    return false;
                }
                // Walk forwards from the first day to the last, wrapping past
                // Saturday so "Fri-Mon" means what it looks like.
                int from = 0;
                int to = 0;
                for (int i = 0; i < 7; i++) {
                    if (fromBit == (1 << i)) {
                        from = i;
                    }
                    if (toBit == (1 << i)) {
                        to = i;
                    }
                }
                for (int i = from;; i = (i + 1) % 7) {
                    days |= (unsigned char)(1 << i);
                    if (i == to) {
                        break;
                    }
                }
            }
        }

        if (comma == std::wstring::npos) {
            break;
        }
        start = comma + 1;
    }

    if (!days) {
        return false;
    }
    *outDays = days;
    return true;
}

// "09:00" or "9:00" to minutes since midnight, or -1.
int ParseTimeOfDay(const std::wstring& text) {
    std::wstring time = Trim(text);
    size_t colon = time.find(L':');
    if (colon == std::wstring::npos) {
        return -1;
    }

    std::wstring hourText = Trim(time.substr(0, colon));
    std::wstring minuteText = Trim(time.substr(colon + 1));
    if (hourText.empty() || minuteText.empty()) {
        return -1;
    }
    for (wchar_t c : hourText + minuteText) {
        if (c < L'0' || c > L'9') {
            return -1;
        }
    }

    int hour = (int)wcstol(hourText.c_str(), nullptr, 10);
    int minute = (int)wcstol(minuteText.c_str(), nullptr, 10);
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return -1;
    }
    return hour * 60 + minute;
}

// Reads one line of the schedule. Returns false, having said why, for a line
// that was meant to be a rule but could not be read.
bool ParseScheduleLine(const std::wstring& rawLine, ScheduleRule* outRule) {
    std::wstring line = Trim(rawLine);
    if (line.empty() || line[0] == L'#' || line[0] == L';') {
        return false;
    }

    size_t split = line.find_last_of(L" \t");
    if (split == std::wstring::npos) {
        Wh_Log(L"schedule: \"%s\" has no time range", line.c_str());
        return false;
    }

    std::wstring daysText = Trim(line.substr(0, split));
    std::wstring timesText = Trim(line.substr(split + 1));

    size_t dash = timesText.find(L'-');
    if (dash == std::wstring::npos) {
        Wh_Log(L"schedule: \"%s\" needs a time range like 09:00-17:00",
               line.c_str());
        return false;
    }

    int startMinute = ParseTimeOfDay(timesText.substr(0, dash));
    int endMinute = ParseTimeOfDay(timesText.substr(dash + 1));
    if (startMinute < 0 || endMinute < 0) {
        Wh_Log(L"schedule: \"%s\" has a time that is not 24-hour HH:MM",
               line.c_str());
        return false;
    }

    unsigned char days = 0;
    if (!ParseDays(daysText, &days)) {
        Wh_Log(L"schedule: \"%s\" has days that could not be read",
               line.c_str());
        return false;
    }

    outRule->days = days;
    outRule->startMinute = startMinute;
    outRule->endMinute = endMinute;
    return true;
}

void RebuildSchedule() {
    g_rules.clear();
    for (const std::wstring& line : g_settings.schedule) {
        ScheduleRule rule;
        if (ParseScheduleLine(line, &rule)) {
            g_rules.push_back(rule);
        }
    }
    Wh_Log(L"schedule: %d rules in effect", (int)g_rules.size());
}

// True if the given weekday and minute fall inside the rule.
bool RuleCovers(const ScheduleRule& rule, int dayOfWeek, int minuteOfDay) {
    int previousDay = (dayOfWeek + 6) % 7;

    if (rule.startMinute < rule.endMinute) {
        return (rule.days & (1 << dayOfWeek)) &&
               minuteOfDay >= rule.startMinute && minuteOfDay < rule.endMinute;
    }

    // Through midnight: the tail of one of its own days, or the head of the day
    // after one of them. A rule whose start equals its end covers the lot.
    if ((rule.days & (1 << dayOfWeek)) && minuteOfDay >= rule.startMinute) {
        return true;
    }
    if ((rule.days & (1 << previousDay)) && minuteOfDay < rule.endMinute) {
        return true;
    }
    return false;
}

bool ScheduleCoversMoment(int dayOfWeek, int minuteOfDay) {
    for (const ScheduleRule& rule : g_rules) {
        if (RuleCovers(rule, dayOfWeek, minuteOfDay)) {
            return true;
        }
    }
    return false;
}

bool ScheduleWantsAwakeNow() {
    if (!g_settings.scheduleEnabled || g_rules.empty()) {
        return false;
    }
    SYSTEMTIME now;
    GetLocalTime(&now);
    return ScheduleCoversMoment(now.wDayOfWeek, now.wHour * 60 + now.wMinute);
}

// How many minutes until the schedule's answer changes, or -1 if it never does.
// Found by walking forward a minute at a time over a week, which is a few
// thousand trivial comparisons - far simpler to get right than reasoning about
// where the edges of overlapping rules fall, and it runs only when the tooltip
// or the menu is built.
int MinutesUntilScheduleChanges() {
    if (!g_settings.scheduleEnabled || g_rules.empty()) {
        return -1;
    }

    SYSTEMTIME now;
    GetLocalTime(&now);
    int dayOfWeek = now.wDayOfWeek;
    int minuteOfDay = now.wHour * 60 + now.wMinute;
    bool current = ScheduleCoversMoment(dayOfWeek, minuteOfDay);

    constexpr int kMinutesInWeek = 7 * 24 * 60;
    for (int ahead = 1; ahead <= kMinutesInWeek; ahead++) {
        int minute = (minuteOfDay + ahead) % (24 * 60);
        int day = (dayOfWeek + (minuteOfDay + ahead) / (24 * 60)) % 7;
        if (ScheduleCoversMoment(day, minute) != current) {
            return ahead;
        }
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Keeping the PC awake, and deciding when to.

// What is in force now, and why - the tooltip exists to answer "why is my
// screen still on", so the reason is kept rather than recomputed.
bool g_awake;
bool g_overrideActive;
bool g_overrideAwake;
ULONGLONG g_overrideExpiryTick;  // 0 when the override has no timer.
bool g_schedulePaused;           // From the tray menu; not saved.
bool g_lastScheduleWanted;
bool g_haveLastSchedule;

HWND g_trayWnd;
HICON g_trayIcon;
bool g_trayIconAdded;
UINT g_taskbarCreatedMessage;
ULONG_PTR g_gdiplusToken;
HINSTANCE g_hInst;
DWORD g_uiThreadId;
HANDLE g_uiThread;
HANDLE g_readyEvent;

constexpr UINT WM_APP_TRAY = WM_APP + 1;
constexpr UINT WM_APP_SETTINGS = WM_APP + 2;
constexpr UINT WM_APP_QUIT = WM_APP + 3;

constexpr UINT_PTR kTickTimerId = 1;
constexpr UINT kTickIntervalMs = 15000;
constexpr UINT kTrayIconId = 1;

enum MenuCommand {
    kCmdToggle = 100,
    kCmdFollowSchedule,
    kCmdPauseSchedule,
    kCmdFor15Minutes,
    kCmdFor30Minutes,
    kCmdFor1Hour,
    kCmdFor2Hours,
    kCmdFor4Hours,
};

// Tells Windows the machine is in use, or stops telling it. The flags are held
// on this one thread for as long as the mod runs, which is what ES_CONTINUOUS
// means - a thread that exits takes its request with it, so every call has to
// come from the UI thread.
void ApplyPowerState(bool awake) {
    EXECUTION_STATE flags = ES_CONTINUOUS;
    if (awake) {
        if (g_settings.preventSleep) {
            flags |= ES_SYSTEM_REQUIRED;
        }
        if (g_settings.preventDisplayOff) {
            flags |= ES_DISPLAY_REQUIRED;
        }
    }

    if (!SetThreadExecutionState(flags)) {
        Wh_Log(L"SetThreadExecutionState failed");
        return;
    }
    Wh_Log(L"now %s", awake ? L"keeping the PC awake" : L"letting the PC rest");
}

bool ScheduleIsLive() {
    return g_settings.scheduleEnabled && !g_schedulePaused;
}

bool ScheduleWantsAwake() {
    return ScheduleIsLive() && ScheduleWantsAwakeNow();
}

void ClearOverride(PCWSTR reason) {
    if (!g_overrideActive) {
        return;
    }
    g_overrideActive = false;
    g_overrideExpiryTick = 0;
    Wh_Log(L"override cleared: %s", reason);
}

void SetOverride(bool awake, int minutes) {
    g_overrideActive = true;
    g_overrideAwake = awake;
    g_overrideExpiryTick =
        minutes > 0 ? GetTickCount64() + (ULONGLONG)minutes * 60000 : 0;
    Wh_Log(L"override: %s%s", awake ? L"awake" : L"asleep",
           minutes > 0 ? L", on a timer" : L"");
}

// Formats a count of minutes as "1h 20m", "45m", or "now".
std::wstring FormatDuration(int minutes) {
    if (minutes <= 0) {
        return L"now";
    }
    if (minutes < 60) {
        return std::to_wstring(minutes) + L"m";
    }
    std::wstring text = std::to_wstring(minutes / 60) + L"h";
    if (minutes % 60) {
        text += L" " + std::to_wstring(minutes % 60) + L"m";
    }
    return text;
}

// The wall-clock time `minutes` from now, as "17:00".
std::wstring FormatClockAfter(int minutes) {
    SYSTEMTIME now;
    GetLocalTime(&now);
    int total = (now.wHour * 60 + now.wMinute + minutes) % (24 * 60);
    WCHAR text[16];
    swprintf_s(text, L"%02d:%02d", total / 60, total % 60);
    return text;
}

// The second line of the tooltip, and the greyed line at the foot of the menu:
// what is happening and what will end it.
std::wstring DescribeState() {
    std::wstring state = g_awake ? L"Awake" : L"Idle";

    if (g_overrideActive) {
        if (g_overrideExpiryTick) {
            ULONGLONG now = GetTickCount64();
            int left = g_overrideExpiryTick > now
                           ? (int)((g_overrideExpiryTick - now) / 60000) + 1
                           : 0;
            return state + L" - set by you for " + FormatDuration(left);
        }
        if (g_settings.overrideLasts == OverrideLasts::nextChange &&
            ScheduleIsLive() && !g_rules.empty()) {
            int until = MinutesUntilScheduleChanges();
            if (until > 0) {
                return state + L" - set by you until " + FormatClockAfter(until);
            }
        }
        return state + L" - set by you";
    }

    if (!ScheduleIsLive()) {
        return state + (g_schedulePaused ? L" - schedule paused"
                                         : L" - schedule off");
    }
    if (g_rules.empty()) {
        return state + L" - no schedule rules";
    }

    int until = MinutesUntilScheduleChanges();
    if (until > 0) {
        return state + L" - on schedule until " + FormatClockAfter(until);
    }
    return state + L" - on schedule";
}

////////////////////////////////////////////////////////////////////////////////
// The tray icon.

// A filled disc when the PC is being kept awake, a ring when it is not. Two
// shapes rather than two colours alone, so it reads at a glance and still works
// for anyone who cannot tell the two colours apart.
HICON CreateStateIcon(int size, bool awake, COLORREF color) {
    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = size;
    bitmapInfo.bmiHeader.biHeight = -size;  // Top-down.
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC screenDC = GetDC(nullptr);
    void* bits = nullptr;
    HBITMAP colorBitmap = CreateDIBSection(screenDC, &bitmapInfo,
                                           DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!colorBitmap) {
        ReleaseDC(nullptr, screenDC);
        return nullptr;
    }

    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, colorBitmap);
    {
        Gdiplus::Graphics graphics(memDC);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

        Gdiplus::Color drawColor(255, GetRValue(color), GetGValue(color),
                                 GetBValue(color));
        float inset = size / 8.0f;
        Gdiplus::RectF disc(inset, inset, size - inset * 2,
                            size - inset * 2);
        if (awake) {
            Gdiplus::SolidBrush brush(drawColor);
            graphics.FillEllipse(&brush, disc);
        } else {
            float thickness = size / 8.0f;
            if (thickness < 1.5f) {
                thickness = 1.5f;
            }
            Gdiplus::Pen pen(drawColor, thickness);
            Gdiplus::RectF ring(disc.X + thickness / 2, disc.Y + thickness / 2,
                                disc.Width - thickness,
                                disc.Height - thickness);
            graphics.DrawEllipse(&pen, ring);
        }
    }
    SelectObject(memDC, oldBitmap);

    // An all-zero mask means "take every pixel from the colour bitmap", which
    // then supplies its own transparency through its alpha channel.
    int maskStride = ((size + 15) / 16) * 2;
    std::vector<BYTE> maskBits((size_t)maskStride * size, 0);
    HBITMAP maskBitmap = CreateBitmap(size, size, 1, 1, maskBits.data());

    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmMask = maskBitmap;
    iconInfo.hbmColor = colorBitmap;
    HICON icon = CreateIconIndirect(&iconInfo);

    if (maskBitmap) {
        DeleteObject(maskBitmap);
    }
    DeleteObject(colorBitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
    return icon;
}

void FillTrayIconData(NOTIFYICONDATAW* data) {
    *data = {};
    data->cbSize = sizeof(NOTIFYICONDATAW);
    data->hWnd = g_trayWnd;
    data->uID = kTrayIconId;
}

void RemoveTrayIcon() {
    if (!g_trayIconAdded) {
        return;
    }
    NOTIFYICONDATAW data;
    FillTrayIconData(&data);
    Shell_NotifyIconW(NIM_DELETE, &data);
    g_trayIconAdded = false;
}

void UpdateTrayIcon() {
    if (!g_settings.showTrayIcon) {
        RemoveTrayIcon();
        return;
    }

    HICON icon = CreateStateIcon(
        g_settings.iconSize, g_awake,
        g_awake ? g_settings.activeColor : g_settings.inactiveColor);
    if (!icon) {
        return;
    }

    std::wstring tip = L"Caffeine Scheduler\r\n" + DescribeState();
    if (tip.size() > 127) {
        tip.resize(127);
    }

    NOTIFYICONDATAW data;
    FillTrayIconData(&data);
    data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    data.uCallbackMessage = WM_APP_TRAY;
    data.hIcon = icon;
    wcscpy_s(data.szTip, ARRAYSIZE(data.szTip), tip.c_str());

    if (Shell_NotifyIconW(g_trayIconAdded ? NIM_MODIFY : NIM_ADD, &data)) {
        g_trayIconAdded = true;
    }

    // The shell copies the icon, so the old one can go now that the new one has
    // been handed over.
    if (g_trayIcon) {
        DestroyIcon(g_trayIcon);
    }
    g_trayIcon = icon;
}

////////////////////////////////////////////////////////////////////////////////
// The decision, run on a timer and after anything that could change it.

void Reevaluate() {
    bool scheduleWants = ScheduleWantsAwake();

    // A change in what the schedule wants is what retires an override, so that
    // overruling the 9-to-5 rule lasts until 17:00 rather than for ever.
    if (g_haveLastSchedule && scheduleWants != g_lastScheduleWanted &&
        g_settings.overrideLasts == OverrideLasts::nextChange) {
        ClearOverride(L"the schedule changed");
    }
    g_lastScheduleWanted = scheduleWants;
    g_haveLastSchedule = true;

    if (g_overrideActive && g_overrideExpiryTick &&
        GetTickCount64() >= g_overrideExpiryTick) {
        ClearOverride(L"the timer ran out");
    }

    bool desired = g_overrideActive ? g_overrideAwake : scheduleWants;
    if (desired != g_awake) {
        g_awake = desired;
        ApplyPowerState(desired);
    } else if (g_settings.logVerbose) {
        LogVerbose(DescribeState().c_str());
    }

    UpdateTrayIcon();
}

void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    AppendMenuW(menu, MF_STRING, kCmdToggle,
                g_awake ? L"Let it sleep" : L"Keep awake now");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kCmdFor15Minutes, L"Keep awake 15 minutes");
    AppendMenuW(menu, MF_STRING, kCmdFor30Minutes, L"Keep awake 30 minutes");
    AppendMenuW(menu, MF_STRING, kCmdFor1Hour, L"Keep awake 1 hour");
    AppendMenuW(menu, MF_STRING, kCmdFor2Hours, L"Keep awake 2 hours");
    AppendMenuW(menu, MF_STRING, kCmdFor4Hours, L"Keep awake 4 hours");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (g_overrideActive ? 0 : MF_GRAYED),
                kCmdFollowSchedule, L"Follow the schedule again");
    AppendMenuW(menu, MF_STRING | (g_schedulePaused ? MF_CHECKED : 0),
                kCmdPauseSchedule, L"Pause the schedule");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, DescribeState().c_str());

    POINT cursor;
    GetCursorPos(&cursor);
    // Without taking the foreground first, the menu never closes when the user
    // clicks away from it; the null message afterwards is the other half of the
    // same long-standing shell workaround.
    SetForegroundWindow(g_trayWnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, g_trayWnd,
                   nullptr);
    PostMessageW(g_trayWnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

void HandleMenuCommand(UINT command) {
    switch (command) {
        case kCmdToggle:
            SetOverride(!g_awake, 0);
            break;
        case kCmdFollowSchedule:
            ClearOverride(L"asked to follow the schedule");
            break;
        case kCmdPauseSchedule:
            g_schedulePaused = !g_schedulePaused;
            // Pausing is about the schedule, not about right now, so a standing
            // override would only confuse what the user just asked for.
            ClearOverride(g_schedulePaused ? L"the schedule was paused"
                                           : L"the schedule was resumed");
            // The next evaluation is a fresh start; without this, resuming
            // would look like a schedule change and clear nothing.
            g_haveLastSchedule = false;
            break;
        case kCmdFor15Minutes:
            SetOverride(true, 15);
            break;
        case kCmdFor30Minutes:
            SetOverride(true, 30);
            break;
        case kCmdFor1Hour:
            SetOverride(true, 60);
            break;
        case kCmdFor2Hours:
            SetOverride(true, 120);
            break;
        case kCmdFor4Hours:
            SetOverride(true, 240);
            break;
        default:
            return;
    }
    Reevaluate();
}

LRESULT CALLBACK TrayWndProc(HWND hwnd,
                             UINT message,
                             WPARAM wParam,
                             LPARAM lParam) {
    if (message == WM_APP_TRAY) {
        switch (LOWORD(lParam)) {
            case WM_LBUTTONUP:
                SetOverride(!g_awake, 0);
                Reevaluate();
                return 0;
            case WM_RBUTTONUP:
            case WM_CONTEXTMENU:
                ShowTrayMenu();
                return 0;
        }
        return 0;
    }

    if (g_taskbarCreatedMessage && message == g_taskbarCreatedMessage) {
        // Explorer restarted and took the notification area with it.
        g_trayIconAdded = false;
        UpdateTrayIcon();
        return 0;
    }

    switch (message) {
        case WM_COMMAND:
            HandleMenuCommand(LOWORD(wParam));
            return 0;
        case WM_TIMER:
            if (wParam == kTickTimerId) {
                Reevaluate();
                return 0;
            }
            break;
        case WM_POWERBROADCAST:
            // Coming back from sleep, the clock has moved on without us.
            if (wParam == PBT_APMRESUMEAUTOMATIC ||
                wParam == PBT_APMRESUMESUSPEND) {
                Reevaluate();
            }
            return TRUE;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// The UI thread and the mod's lifecycle.

DWORD WINAPI UiThreadProc(LPVOID param) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

    WNDCLASSEXW windowClass = {sizeof(windowClass)};
    windowClass.lpfnWndProc = TrayWndProc;
    windowClass.hInstance = g_hInst;
    windowClass.lpszClassName = L"WH_CaffeineScheduler";
    RegisterClassExW(&windowClass);

    // Not a message-only window: the tray menu has to be able to take the
    // foreground, and HWND_MESSAGE windows cannot.
    g_trayWnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"WH_CaffeineScheduler", L"",
                                WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
                                g_hInst, nullptr);
    if (!g_trayWnd) {
        Wh_Log(L"could not create the tray window");
        SetEvent(g_readyEvent);
        return 1;
    }

    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    RebuildSchedule();
    Reevaluate();
    SetTimer(g_trayWnd, kTickTimerId, kTickIntervalMs, nullptr);

    SetEvent(g_readyEvent);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.hwnd == nullptr && msg.message == WM_APP_QUIT) {
            PostQuitMessage(0);
            continue;
        }
        if (msg.hwnd == nullptr && msg.message == WM_APP_SETTINGS) {
            LoadSettings();
            RebuildSchedule();
            // The rules may have moved underneath us, so the next evaluation
            // must not mistake that for a boundary and retire an override.
            g_haveLastSchedule = false;
            Reevaluate();
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(g_trayWnd, kTickTimerId);
    RemoveTrayIcon();

    // Hand the power request back before the thread that holds it goes away.
    ApplyPowerState(false);

    if (g_trayIcon) {
        DestroyIcon(g_trayIcon);
        g_trayIcon = nullptr;
    }
    if (g_trayWnd) {
        DestroyWindow(g_trayWnd);
        g_trayWnd = nullptr;
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

    g_readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_readyEvent) {
        return FALSE;
    }

    g_uiThread =
        CreateThread(nullptr, 0, UiThreadProc, nullptr, 0, &g_uiThreadId);
    if (!g_uiThread) {
        CloseHandle(g_readyEvent);
        g_readyEvent = nullptr;
        return FALSE;
    }

    WaitForSingleObject(g_readyEvent, 5000);
    return TRUE;
}

void WhTool_ModSettingsChanged() {
    if (g_uiThreadId) {
        PostThreadMessageW(g_uiThreadId, WM_APP_SETTINGS, 0, 0);
    }
}

void WhTool_ModUninit() {
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
    if (g_readyEvent) {
        CloseHandle(g_readyEvent);
        g_readyEvent = nullptr;
    }
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

// How long a starting tool process waits for the previous one to let go.
// Windhawk starts the replacement before the old process has finished its
// uninit, so on every update the new one finds the mutex still held and, left
// to the stock behaviour, gives up for good - the old one then exits and
// nothing is running at all. Measured, the handover needs about 200ms.
constexpr DWORD kToolModHandoverTimeoutMs = 3000;

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
            // Usually the previous instance on its way out rather than a
            // genuine second copy, so wait for it before giving up. Owning an
            // abandoned mutex counts too: that is the previous process having
            // died without releasing it.
            DWORD waited = WaitForSingleObject(g_toolModProcessMutex,
                                               kToolModHandoverTimeoutMs);
            if (waited != WAIT_OBJECT_0 && waited != WAIT_ABANDONED) {
                Wh_Log(L"Tool mod already running (%s)", WH_MOD_ID);
                ExitProcess(1);
            }
            Wh_Log(L"Took over from the previous instance");
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
