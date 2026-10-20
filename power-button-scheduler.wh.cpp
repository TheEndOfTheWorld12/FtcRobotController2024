// ==WindhawkMod==
// @id              power-button-scheduler
// @name            Power Button Scheduler
// @description     Schedule what closing the lid or pressing the power button does, by time, day and whether you are on charge
// @version         1.3
// @author          aariv
// @include         windhawk.exe
// @compilerOptions -lpowrprof -luser32 -lshell32 -lgdi32 -lgdiplus
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Power Button Scheduler

Windows lets you set **one** answer to "what happens when I close the lid", and
one more for when you are on battery. This lets the answer depend on the time
and the day as well.

## Rules

One per line, read from the top. **The first rule that matches wins**, so put
the specific ones first and a catch-all last:

```
Mon-Fri 08:30-16:00  charger  lid = shut down
Daily   22:00-02:30  battery  lid = nothing
Always               charger  lid = nothing
Always               battery  lid = shut down
```

That reads: on charge during the school day, closing the lid shuts the machine
down; on battery late at night it stays running; the last two lines catch
everything else.

Each rule is **when**, **on what power**, and **which button does what**:

* **When** - `Mon-Fri`, `Sat,Sun`, `Mon-Thu,Sat`, `Daily`, `Weekdays`,
  `Weekends`, or `Always`. Add a 24-hour range like `08:30-16:00`, or leave the
  time out for the whole day. A range whose end is earlier than its start runs
  through midnight, so `22:00-02:30` is what it looks like.
* **On what power** - `charger` (or `ac`, `plugged`), `battery` (or `dc`,
  `unplugged`), or `both`. Leave it out and the rule covers both.
* **Which button** - `lid`, `power`, or `sleep`.
* **What it does** - `nothing`, `sleep`, `hibernate`, or `shut down`.

The order of the parts is not fussy; `lid` just has to be the last thing before
the `=`. Blank lines and lines starting with `#` or `;` are ignored.

Anything no rule mentions is **left alone** - so a mod that only ever talks
about the lid will never touch your power button.

## Putting things back

The settings this changes are the real Windows ones, the same fields in Power
Options. It remembers what they were when it started, and puts them back when
you disable the mod, pause it from the tray icon, or when no rule applies. It
does not leave your machine configured however it happened to be at the moment
you switched it off.

## Please read this part

Two of these settings can lose you work or cook your laptop, so they are worth
saying out loud:

* **`shut down`** on lid close means closing the lid ends everything that is
  running, unsaved work included. Windows will not prompt.
* **`nothing`** means a closed laptop is still awake. In a bag, that is a
  machine running at full tilt with no airflow. Only use it for stretches when
  the laptop is sat on a desk.

The mod cannot override a *deliberate* shutdown or sleep from the Start menu;
it only sets what the lid and the buttons do.

## If nothing happens

Hover the tray icon; it says why. Confirmed working from an ordinary,
non-administrator account, so a refusal is not the expected outcome - but if
Windows does refuse, the tooltip gives the error number, and `error 5` is
"access denied".

Worth knowing: this writes the real Windows setting, the same field Power
Options shows. That field will therefore read differently at different times of
day, which is the mod working rather than a fault, and a change you make there
by hand is overwritten within about fifteen seconds while a rule covers it.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- rules:
  - "Mon-Fri 08:30-16:00 charger lid = shut down"
  - "Daily 22:00-02:30 battery lid = nothing"
  - "Always charger lid = nothing"
  - "Always battery lid = shut down"
  $name: Rules
  $description: >-
    One per line, first match wins. "Mon-Fri 08:30-16:00 charger lid = shut
    down". The when, the power source and the button can be in any order, as
    long as the button is last before the "=". Leave the time out for all day,
    leave the power source out for both. Lines starting with # or ; are ignored.
- showTrayIcon: true
  $name: Show the tray icon
  $description: >-
    Shows what the lid will do right now, and lets you pause the whole thing.
- logVerbose: false
  $name: Verbose logging
  $description: Log every decision, including the ones that changed nothing.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <gdiplus.h>
#include <powrprof.h>
// The Power*ValueIndex functions are declared here rather than in powrprof.h,
// which is easy to miss because the related PowerGetActiveScheme is in both.
#include <powersetting.h>
#include <shellapi.h>

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// The Windows power settings being driven.
//
// The GUIDs are written out rather than taken from winnt.h, because the
// header's DEFINE_GUID only produces storage when initguid.h was included
// ahead of it, and windhawk's own headers pull in windows.h first. If one of
// them were ever wrong the read below fails and the mod declines to write
// anything, so a mistake here cannot leave the machine misconfigured.

constexpr GUID kSystemButtonSubgroup = {
    0x4f971e89,
    0xeebd,
    0x4455,
    {0xa8, 0xde, 0x9e, 0x59, 0x04, 0x0e, 0x73, 0x47}};

constexpr GUID kLidCloseAction = {
    0x5ca83367,
    0x6e45,
    0x459f,
    {0xa2, 0x7b, 0x47, 0x6b, 0x1d, 0x01, 0xc9, 0x36}};

constexpr GUID kPowerButtonAction = {
    0x7648efa3,
    0xdd9c,
    0x4e3e,
    {0xb5, 0x66, 0x50, 0xf9, 0x29, 0x38, 0x62, 0x80}};

constexpr GUID kSleepButtonAction = {
    0x96996bc0,
    0xad50,
    0x47ec,
    {0x92, 0x3b, 0x6f, 0x41, 0x87, 0x4d, 0xd9, 0x6b}};

// The values Windows stores for a button action.
constexpr DWORD kActionNothing = 0;
constexpr DWORD kActionSleep = 1;
constexpr DWORD kActionHibernate = 2;
constexpr DWORD kActionShutDown = 3;
constexpr DWORD kActionUnset = 0xFFFFFFFF;  // "No rule said; leave it alone."

enum class Button {
    lid = 0,
    power = 1,
    sleep = 2,
};
constexpr int kButtonCount = 3;

enum class Source {
    charger = 0,
    battery = 1,
};
constexpr int kSourceCount = 2;

const GUID& ButtonGuid(Button button) {
    switch (button) {
        case Button::power:
            return kPowerButtonAction;
        case Button::sleep:
            return kSleepButtonAction;
        default:
            return kLidCloseAction;
    }
}

PCWSTR ButtonName(Button button) {
    switch (button) {
        case Button::power:
            return L"power button";
        case Button::sleep:
            return L"sleep button";
        default:
            return L"lid";
    }
}

PCWSTR SourceName(Source source) {
    return source == Source::battery ? L"on battery" : L"on charge";
}

PCWSTR ActionName(DWORD action) {
    switch (action) {
        case kActionNothing:
            return L"do nothing";
        case kActionSleep:
            return L"sleep";
        case kActionHibernate:
            return L"hibernate";
        case kActionShutDown:
            return L"shut down";
        default:
            return L"unmanaged";
    }
}

////////////////////////////////////////////////////////////////////////////////
// Settings.

struct {
    std::vector<std::wstring> rules;
    bool showTrayIcon;
    bool logVerbose;
} g_settings;

void LogVerbose(PCWSTR message) {
    if (g_settings.logVerbose) {
        Wh_Log(L"%s", message);
    }
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

std::vector<std::wstring> SplitWords(const std::wstring& text) {
    std::vector<std::wstring> words;
    std::wstring current;
    for (wchar_t c : text) {
        if (c == L' ' || c == L'\t') {
            if (!current.empty()) {
                words.push_back(current);
                current.clear();
            }
            continue;
        }
        current += c;
    }
    if (!current.empty()) {
        words.push_back(current);
    }
    return words;
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
    LoadStringList(L"rules[%d]", g_settings.rules);
    g_settings.showTrayIcon = Wh_GetIntSetting(L"showTrayIcon");
    g_settings.logVerbose = Wh_GetIntSetting(L"logVerbose");
}

////////////////////////////////////////////////////////////////////////////////
// Rules.

struct Rule {
    unsigned char days;  // Bit 0 is Sunday, matching SYSTEMTIME.
    int startMinute;     // -1 for "any time of day".
    int endMinute;
    bool onCharger;
    bool onBattery;
    Button button;
    DWORD action;
};

std::vector<Rule> g_rules;

unsigned char DayBitFromName(const std::wstring& rawName) {
    static const PCWSTR kShort[7] = {L"sun", L"mon", L"tue", L"wed",
                                     L"thu", L"fri", L"sat"};
    static const PCWSTR kLong[7] = {L"sunday",    L"monday",   L"tuesday",
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

bool ParseDays(const std::wstring& text, unsigned char* outDays) {
    std::wstring lower = ToLower(Trim(text));
    if (lower.empty() || lower == L"daily" || lower == L"always" ||
        lower == L"everyday" || lower == L"all" || lower == L"*") {
        *outDays = 0x7F;
        return true;
    }
    if (lower == L"weekdays" || lower == L"weekday") {
        *outDays = 0x3E;
        return true;
    }
    if (lower == L"weekends" || lower == L"weekend") {
        *outDays = 0x41;
        return true;
    }

    unsigned char days = 0;
    size_t start = 0;
    while (start <= lower.size()) {
        size_t comma = lower.find(L',', start);
        std::wstring piece = lower.substr(
            start,
            comma == std::wstring::npos ? std::wstring::npos : comma - start);
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

bool LooksLikeTimeRange(const std::wstring& word) {
    return word.find(L':') != std::wstring::npos &&
           word.find(L'-') != std::wstring::npos;
}

bool ParseAction(const std::wstring& text, DWORD* outAction) {
    std::wstring action = ToLower(Trim(text));
    // Spaces are allowed in the written form ("shut down"), so compare without
    // them rather than making the reader match a particular spelling.
    std::wstring packed;
    for (wchar_t c : action) {
        if (c != L' ' && c != L'\t' && c != L'-' && c != L'_') {
            packed += c;
        }
    }

    if (packed == L"nothing" || packed == L"none" || packed == L"stayon" ||
        packed == L"stayawake" || packed == L"donothing" || packed == L"keepon") {
        *outAction = kActionNothing;
        return true;
    }
    if (packed == L"sleep" || packed == L"standby" || packed == L"suspend") {
        *outAction = kActionSleep;
        return true;
    }
    if (packed == L"hibernate" || packed == L"hibernation") {
        *outAction = kActionHibernate;
        return true;
    }
    if (packed == L"shutdown" || packed == L"off" || packed == L"turnoff" ||
        packed == L"poweroff") {
        *outAction = kActionShutDown;
        return true;
    }
    return false;
}

bool ParseButton(const std::wstring& text, Button* outButton) {
    std::wstring button = ToLower(Trim(text));
    if (button == L"lid" || button == L"lidclose" || button == L"cover") {
        *outButton = Button::lid;
        return true;
    }
    if (button == L"power" || button == L"powerbutton") {
        *outButton = Button::power;
        return true;
    }
    if (button == L"sleep" || button == L"sleepbutton") {
        *outButton = Button::sleep;
        return true;
    }
    return false;
}

// Recognises a power-source word, saying which sources it selects.
bool ParseSource(const std::wstring& text, bool* outCharger, bool* outBattery) {
    std::wstring source = ToLower(Trim(text));
    if (source == L"charger" || source == L"ac" || source == L"plugged" ||
        source == L"pluggedin" || source == L"mains" || source == L"charging") {
        *outCharger = true;
        *outBattery = false;
        return true;
    }
    if (source == L"battery" || source == L"dc" || source == L"unplugged" ||
        source == L"onbattery") {
        *outCharger = false;
        *outBattery = true;
        return true;
    }
    if (source == L"both" || source == L"any" || source == L"either") {
        *outCharger = true;
        *outBattery = true;
        return true;
    }
    return false;
}

// Reads one rule. The parts before the "=" may be given in any order, which is
// why each is recognised by what it looks like rather than by its position -
// the only fixed point is that the button is the last word before the "=".
bool ParseRule(const std::wstring& rawLine, Rule* outRule) {
    std::wstring line = Trim(rawLine);
    if (line.empty() || line[0] == L'#' || line[0] == L';') {
        return false;
    }

    size_t equals = line.find(L'=');
    if (equals == std::wstring::npos) {
        Wh_Log(L"rule \"%s\": no \"=\", so nothing says what should happen",
               line.c_str());
        return false;
    }

    DWORD action = 0;
    if (!ParseAction(line.substr(equals + 1), &action)) {
        Wh_Log(L"rule \"%s\": the action must be nothing, sleep, hibernate or "
               L"shut down",
               line.c_str());
        return false;
    }

    std::vector<std::wstring> words = SplitWords(Trim(line.substr(0, equals)));
    if (words.empty()) {
        Wh_Log(L"rule \"%s\": nothing before the \"=\"", line.c_str());
        return false;
    }

    Button button = Button::lid;
    if (!ParseButton(words.back(), &button)) {
        Wh_Log(L"rule \"%s\": \"%s\" is not lid, power or sleep", line.c_str(),
               words.back().c_str());
        return false;
    }
    words.pop_back();

    bool onCharger = true;
    bool onBattery = true;
    int startMinute = -1;
    int endMinute = -1;
    std::wstring daysText;

    for (const std::wstring& word : words) {
        bool charger = false;
        bool battery = false;
        if (ParseSource(word, &charger, &battery)) {
            onCharger = charger;
            onBattery = battery;
            continue;
        }
        if (LooksLikeTimeRange(word)) {
            size_t dash = word.find(L'-');
            startMinute = ParseTimeOfDay(word.substr(0, dash));
            endMinute = ParseTimeOfDay(word.substr(dash + 1));
            if (startMinute < 0 || endMinute < 0) {
                Wh_Log(L"rule \"%s\": \"%s\" is not a 24-hour range like "
                       L"08:30-16:00",
                       line.c_str(), word.c_str());
                return false;
            }
            continue;
        }
        if (!daysText.empty()) {
            daysText += L",";
        }
        daysText += word;
    }

    unsigned char days = 0;
    if (!ParseDays(daysText, &days)) {
        Wh_Log(L"rule \"%s\": \"%s\" could not be read as days", line.c_str(),
               daysText.c_str());
        return false;
    }

    outRule->days = days;
    outRule->startMinute = startMinute;
    outRule->endMinute = endMinute;
    outRule->onCharger = onCharger;
    outRule->onBattery = onBattery;
    outRule->button = button;
    outRule->action = action;
    return true;
}

void RebuildRules() {
    g_rules.clear();
    for (const std::wstring& line : g_settings.rules) {
        Rule rule;
        if (ParseRule(line, &rule)) {
            g_rules.push_back(rule);
        }
    }
    Wh_Log(L"%d rules in effect", (int)g_rules.size());
}

bool RuleCoversNow(const Rule& rule, int dayOfWeek, int minuteOfDay) {
    if (rule.startMinute < 0) {
        return (rule.days & (1 << dayOfWeek)) != 0;
    }

    int previousDay = (dayOfWeek + 6) % 7;
    if (rule.startMinute < rule.endMinute) {
        return (rule.days & (1 << dayOfWeek)) &&
               minuteOfDay >= rule.startMinute && minuteOfDay < rule.endMinute;
    }

    // Through midnight: the tail of one of the rule's own days, or the head of
    // the day after one of them.
    if ((rule.days & (1 << dayOfWeek)) && minuteOfDay >= rule.startMinute) {
        return true;
    }
    if ((rule.days & (1 << previousDay)) && minuteOfDay < rule.endMinute) {
        return true;
    }
    return false;
}

// The action the rules call for now, or kActionUnset when none mentions this
// button on this power source. First match wins, so a catch-all belongs last.
DWORD ActionFor(Button button, Source source) {
    SYSTEMTIME now;
    GetLocalTime(&now);
    int dayOfWeek = now.wDayOfWeek;
    int minuteOfDay = now.wHour * 60 + now.wMinute;

    for (const Rule& rule : g_rules) {
        if (rule.button != button) {
            continue;
        }
        if (source == Source::charger ? !rule.onCharger : !rule.onBattery) {
            continue;
        }
        if (RuleCoversNow(rule, dayOfWeek, minuteOfDay)) {
            return rule.action;
        }
    }
    return kActionUnset;
}

////////////////////////////////////////////////////////////////////////////////
// Reading and writing the real Windows power settings.

GUID g_scheme;
bool g_haveScheme;
DWORD g_original[kButtonCount][kSourceCount];
bool g_haveOriginal[kButtonCount][kSourceCount];
DWORD g_applied[kButtonCount][kSourceCount];
bool g_haveApplied[kButtonCount][kSourceCount];
bool g_paused;
bool g_reportedWriteFailure;
// Why nothing happened, kept so the tray icon can say it. Logging is off per
// mod by default, so a failure that is only logged is a failure nobody sees.
DWORD g_lastWriteError;
bool g_writeRefused;
bool g_writeIgnored;

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
    kCmdPause = 100,
};

bool ReadActionValue(const GUID& scheme,
                     Button button,
                     Source source,
                     DWORD* outValue) {
    DWORD error =
        source == Source::charger
            ? PowerReadACValueIndex(nullptr, &scheme, &kSystemButtonSubgroup,
                                    &ButtonGuid(button), outValue)
            : PowerReadDCValueIndex(nullptr, &scheme, &kSystemButtonSubgroup,
                                    &ButtonGuid(button), outValue);
    if (error != ERROR_SUCCESS) {
        Wh_Log(L"could not read the %s action %s: error %lu",
               ButtonName(button), SourceName(source), error);
        return false;
    }
    return true;
}

bool WriteActionValue(const GUID& scheme,
                      Button button,
                      Source source,
                      DWORD value) {
    DWORD error =
        source == Source::charger
            ? PowerWriteACValueIndex(nullptr, &scheme, &kSystemButtonSubgroup,
                                     &ButtonGuid(button), value)
            : PowerWriteDCValueIndex(nullptr, &scheme, &kSystemButtonSubgroup,
                                     &ButtonGuid(button), value);
    if (error != ERROR_SUCCESS) {
        // Said once rather than every fifteen seconds; the number is the point,
        // and 5 is access denied.
        if (!g_reportedWriteFailure) {
            g_reportedWriteFailure = true;
            Wh_Log(L"could not write the %s action %s: error %lu%s",
                   ButtonName(button), SourceName(source), error,
                   error == ERROR_ACCESS_DENIED
                       ? L" - Windows refused; this account may not be allowed "
                         L"to change power settings"
                       : L"");
        }
        g_lastWriteError = error;
        g_writeRefused = true;
        return false;
    }
    g_writeRefused = false;
    return true;
}

// Takes note of what every setting was before anything is changed, so there is
// something to put back. A setting that cannot be read is left unmanaged
// rather than guessed at.
void CaptureOriginals(const GUID& scheme) {
    for (int b = 0; b < kButtonCount; b++) {
        for (int s = 0; s < kSourceCount; s++) {
            DWORD value = 0;
            g_haveOriginal[b][s] =
                ReadActionValue(scheme, (Button)b, (Source)s, &value);
            g_original[b][s] = g_haveOriginal[b][s] ? value : kActionUnset;
            g_haveApplied[b][s] = false;
        }
    }
    Wh_Log(L"captured the current power settings: lid is %s on charge, %s on "
           L"battery",
           g_haveOriginal[0][0] ? ActionName(g_original[0][0]) : L"unreadable",
           g_haveOriginal[0][1] ? ActionName(g_original[0][1]) : L"unreadable");
}

// Picks up the active power scheme, re-reading the originals if the user has
// switched plans since last time - the values we saved belong to the plan they
// came from.
bool RefreshActiveScheme() {
    GUID* active = nullptr;
    DWORD error = PowerGetActiveScheme(nullptr, &active);
    if (error != ERROR_SUCCESS || !active) {
        Wh_Log(L"PowerGetActiveScheme failed: error %lu", error);
        return false;
    }

    bool changed = !g_haveScheme || !IsEqualGUID(g_scheme, *active);
    if (changed) {
        g_scheme = *active;
        g_haveScheme = true;
        CaptureOriginals(g_scheme);
    }
    LocalFree(active);
    return true;
}

void CommitScheme() {
    if (!g_haveScheme) {
        return;
    }
    // Writing a value is not enough on its own; the scheme has to be made
    // active again before Windows picks the change up.
    DWORD error = PowerSetActiveScheme(nullptr, &g_scheme);
    if (error != ERROR_SUCCESS) {
        Wh_Log(L"PowerSetActiveScheme failed: error %lu", error);
    }
}

// Puts every setting back the way it was found, and forgets what was applied.
void RestoreOriginals() {
    if (!g_haveScheme) {
        return;
    }

    bool wroteAnything = false;
    for (int b = 0; b < kButtonCount; b++) {
        for (int s = 0; s < kSourceCount; s++) {
            if (!g_haveOriginal[b][s] || !g_haveApplied[b][s]) {
                continue;
            }
            if (WriteActionValue(g_scheme, (Button)b, (Source)s,
                                 g_original[b][s])) {
                wroteAnything = true;
            }
            g_haveApplied[b][s] = false;
        }
    }

    if (wroteAnything) {
        CommitScheme();
        Wh_Log(L"put the power settings back as they were");
    }
}

////////////////////////////////////////////////////////////////////////////////
// Working out and applying what should be in force.

// The action that should be in force now: what the rules say, or what the
// setting was before this mod touched it.
DWORD DesiredAction(Button button, Source source) {
    if (g_paused) {
        return g_original[(int)button][(int)source];
    }
    DWORD action = ActionFor(button, source);
    return action == kActionUnset ? g_original[(int)button][(int)source]
                                  : action;
}

void UpdateTrayIcon();

void ApplyNow() {
    if (!RefreshActiveScheme()) {
        return;
    }

    bool wroteAnything = false;
    for (int b = 0; b < kButtonCount; b++) {
        for (int s = 0; s < kSourceCount; s++) {
            if (!g_haveOriginal[b][s]) {
                continue;  // Never read it, so never write it.
            }

            DWORD desired = DesiredAction((Button)b, (Source)s);
            if (desired == kActionUnset) {
                continue;
            }
            if (g_haveApplied[b][s] && g_applied[b][s] == desired) {
                continue;
            }

            if (WriteActionValue(g_scheme, (Button)b, (Source)s, desired)) {
                g_applied[b][s] = desired;
                g_haveApplied[b][s] = true;
                wroteAnything = true;
                Wh_Log(L"%s %s is now: %s", ButtonName((Button)b),
                       SourceName((Source)s), ActionName(desired));
            }
        }
    }

    if (wroteAnything) {
        CommitScheme();

        // Read it straight back. A write can report success and still not take
        // hold - a policy or another tool putting it back - and that looks
        // exactly like nothing having happened, so it is worth telling apart
        // from a write that was refused outright.
        g_writeIgnored = false;
        for (int b = 0; b < kButtonCount; b++) {
            for (int s = 0; s < kSourceCount; s++) {
                if (!g_haveApplied[b][s]) {
                    continue;
                }
                DWORD actual = 0;
                if (ReadActionValue(g_scheme, (Button)b, (Source)s, &actual) &&
                    actual != g_applied[b][s]) {
                    g_writeIgnored = true;
                    Wh_Log(L"%s %s was set to %s but reads back as %s",
                           ButtonName((Button)b), SourceName((Source)s),
                           ActionName(g_applied[b][s]), ActionName(actual));
                }
            }
        }
    } else {
        LogVerbose(L"nothing to change");
    }

    UpdateTrayIcon();
}

// Which power source the machine is actually on, for the tray icon. The written
// settings cover both regardless, so this is only ever about what to show.
Source CurrentSource() {
    SYSTEM_POWER_STATUS status = {};
    if (GetSystemPowerStatus(&status) && status.ACLineStatus == 0) {
        return Source::battery;
    }
    return Source::charger;
}

std::wstring DescribeState() {
    // Trouble first: knowing the lid is meant to sleep is no use when the
    // reason it does not is further down a log nobody has switched on.
    if (g_writeRefused) {
        std::wstring text = L"Windows refused the change (error " +
                            std::to_wstring((int)g_lastWriteError) + L")";
        if (g_lastWriteError == ERROR_ACCESS_DENIED) {
            text += L" - needs an administrator";
        }
        return text;
    }
    if (g_writeIgnored) {
        return L"Setting was accepted but did not stick";
    }
    if (g_paused) {
        return L"Paused - Windows' own settings are back";
    }
    if (g_rules.empty()) {
        return L"No rules, so nothing is being changed";
    }

    Source source = CurrentSource();
    DWORD lid = g_haveOriginal[0][(int)source]
                    ? DesiredAction(Button::lid, source)
                    : kActionUnset;
    std::wstring text = L"Lid ";
    text += SourceName(source);
    text += L": ";
    text += ActionName(lid);
    return text;
}

////////////////////////////////////////////////////////////////////////////////
// The tray icon.

COLORREF ActionColor(DWORD action) {
    switch (action) {
        case kActionNothing:
            return RGB(80, 190, 110);
        case kActionSleep:
            return RGB(90, 160, 240);
        case kActionHibernate:
            return RGB(170, 130, 230);
        case kActionShutDown:
            return RGB(230, 90, 80);
        default:
            return RGB(140, 140, 140);
    }
}

HICON CreateStateIcon(int size, COLORREF color, bool hollow) {
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
        Gdiplus::RectF disc(inset, inset, size - inset * 2, size - inset * 2);
        if (hollow) {
            float thickness = size / 8.0f;
            if (thickness < 1.5f) {
                thickness = 1.5f;
            }
            Gdiplus::Pen pen(drawColor, thickness);
            Gdiplus::RectF ring(disc.X + thickness / 2, disc.Y + thickness / 2,
                                disc.Width - thickness, disc.Height - thickness);
            graphics.DrawEllipse(&pen, ring);
        } else {
            Gdiplus::SolidBrush brush(drawColor);
            graphics.FillEllipse(&brush, disc);
        }
    }
    SelectObject(memDC, oldBitmap);

    // An all-zero mask means every pixel comes from the colour bitmap, which
    // carries its own transparency in its alpha channel.
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

    Source source = CurrentSource();
    DWORD lid = g_haveOriginal[0][(int)source]
                    ? DesiredAction(Button::lid, source)
                    : kActionUnset;

    HICON icon = CreateStateIcon(16, ActionColor(lid), g_paused);
    if (!icon) {
        return;
    }

    std::wstring tip = L"Power Button Scheduler\r\n" + DescribeState();
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

    if (g_trayIcon) {
        DestroyIcon(g_trayIcon);
    }
    g_trayIcon = icon;
}

void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    AppendMenuW(menu, MF_STRING | (g_paused ? MF_CHECKED : 0), kCmdPause,
                L"Pause, and put Windows' settings back");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    // Every managed setting, so there is never any doubt about what the lid or
    // the buttons will do right now.
    for (int b = 0; b < kButtonCount; b++) {
        for (int s = 0; s < kSourceCount; s++) {
            if (!g_haveOriginal[b][s]) {
                continue;
            }
            std::wstring line = ButtonName((Button)b);
            line += L" ";
            line += SourceName((Source)s);
            line += L": ";
            line += ActionName(DesiredAction((Button)b, (Source)s));
            AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, line.c_str());
        }
    }

    POINT cursor;
    GetCursorPos(&cursor);
    // Taking the foreground first is what lets the menu close when the user
    // clicks away; the null message afterwards is the other half of it.
    SetForegroundWindow(g_trayWnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, g_trayWnd,
                   nullptr);
    PostMessageW(g_trayWnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

LRESULT CALLBACK TrayWndProc(HWND hwnd,
                             UINT message,
                             WPARAM wParam,
                             LPARAM lParam) {
    if (message == WM_APP_TRAY) {
        switch (LOWORD(lParam)) {
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_CONTEXTMENU:
                ShowTrayMenu();
                return 0;
        }
        return 0;
    }

    if (g_taskbarCreatedMessage && message == g_taskbarCreatedMessage) {
        g_trayIconAdded = false;
        UpdateTrayIcon();
        return 0;
    }

    switch (message) {
        case WM_COMMAND:
            if (LOWORD(wParam) == kCmdPause) {
                g_paused = !g_paused;
                if (g_paused) {
                    RestoreOriginals();
                    UpdateTrayIcon();
                } else {
                    ApplyNow();
                }
            }
            return 0;
        case WM_TIMER:
            if (wParam == kTickTimerId) {
                ApplyNow();
                return 0;
            }
            break;
        case WM_POWERBROADCAST:
            // The charger going in or out does not change what gets written,
            // but it does change which of the two the tray icon is showing.
            if (wParam == PBT_APMPOWERSTATUSCHANGE ||
                wParam == PBT_APMRESUMEAUTOMATIC ||
                wParam == PBT_APMRESUMESUSPEND) {
                ApplyNow();
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
    windowClass.lpszClassName = L"WH_PowerButtonScheduler";
    RegisterClassExW(&windowClass);

    // Not message-only: the tray menu has to be able to take the foreground.
    g_trayWnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"WH_PowerButtonScheduler",
                                L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
                                g_hInst, nullptr);
    if (!g_trayWnd) {
        Wh_Log(L"could not create the tray window");
        SetEvent(g_readyEvent);
        return 1;
    }

    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    RebuildRules();
    ApplyNow();
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
            RebuildRules();
            // The rules changed under us, so what was written no longer
            // necessarily follows from them.
            for (int b = 0; b < kButtonCount; b++) {
                for (int s = 0; s < kSourceCount; s++) {
                    g_haveApplied[b][s] = false;
                }
            }
            g_reportedWriteFailure = false;
            ApplyNow();
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(g_trayWnd, kTickTimerId);
    RemoveTrayIcon();

    // Leave the machine as it was found, not as this mod last set it.
    RestoreOriginals();

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
