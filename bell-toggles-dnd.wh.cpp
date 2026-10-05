// ==WindhawkMod==
// @id              bell-toggles-dnd
// @name            Bell icon toggles Do Not Disturb
// @description     Clicking the notification bell in the tray turns Do Not Disturb on or off instead of opening the notification centre
// @version         1.0.0
// @author          aariv
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lole32 -loleaut32 -lruntimeobject -lversion
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Bell icon toggles Do Not Disturb

Windows puts the bell and the clock in one button, so clicking either opens the
notification centre. This gives the bell its own job: click it and Do Not
Disturb turns on or off. The clock next to it still opens the notification
centre, so nothing is lost.

The bell glyph changes to its Do-Not-Disturb variant the moment it flips, so
the icon itself is the feedback.

## How it works

Windows has no public API for setting Do Not Disturb, but the switch inside the
notification centre goes through an undocumented COM interface,
`IQuietHoursSettings`, and "on" is simply the priority-only quiet hours
profile. This mod calls the same interface.

Reading the state is clean - the shell publishes it as a WNF notification -
so the mod never has to guess which way to flip.

## Notes

* Only Windows 11 has this bell.
* If the bell is hidden, turn it on in **Settings -> System -> Notifications**.
* The mod leaves every other tray icon alone.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- mouseButton: left
  $name: Which click toggles it
  $description: >-
    The other buttons keep their normal behaviour, so you can still open the
    notification centre from the bell if you leave this on middle or right.
  $options:
  - left: Left click
  - middle: Middle click
  - right: Right click
- alsoOpenCentre: false
  $name: Also open the notification centre
  $description: >-
    Off by default - the whole point is that the bell stops opening it. Turn it
    on if you want both to happen at once.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <atomic>
#include <functional>
#include <list>
#include <string>
#include <vector>

#undef GetCurrentTime

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/base.h>

using namespace winrt::Windows::UI::Xaml;

namespace input = winrt::Windows::UI::Xaml::Input;

// ---------------------------------------------------------------- settings --

enum class MouseButton { left, middle, right };

struct {
    MouseButton mouseButton;
    bool alsoOpenCentre;
} g_settings;

std::atomic<bool> g_systemTrayModuleHooked;
std::atomic<bool> g_unloading;

// ------------------------------------------------------------ do not disturb --

// The notification centre's own switch goes through this. It is undocumented,
// so it is looked up at runtime and the feature simply does nothing if a future
// Windows moves it.
struct IQuietHoursSettings : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE get_UserSelectedProfile(LPWSTR* id) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_UserSelectedProfile(LPCWSTR id) = 0;
};

constexpr GUID kCLSID_QuietHoursSettings = {
    0xf53321fa, 0x34f8, 0x4b7f,
    {0xb9, 0xa3, 0x36, 0x18, 0x77, 0xcb, 0x94, 0xcf}};
constexpr GUID kIID_IQuietHoursSettings = {
    0x6bff4732, 0x81ec, 0x4ffb,
    {0xae, 0x67, 0xb6, 0xc1, 0xbc, 0x29, 0x63, 0x1f}};

// The shell publishes the active quiet hours profile here; 0 means none.
constexpr ULONGLONG kWnfQuietHoursActiveProfile = 0x0D83063EA3BF1C75;

bool IsDoNotDisturbOn() {
    using NtQueryWnfStateData_t =
        LONG(NTAPI*)(const ULONGLONG* stateName, const void* typeId,
                     const void* explicitScope, ULONG* changeStamp,
                     void* buffer, ULONG* bufferSize);
    static const auto query = (NtQueryWnfStateData_t)GetProcAddress(
        GetModuleHandle(L"ntdll.dll"), "NtQueryWnfStateData");
    if (!query) {
        return false;
    }

    DWORD profile = 0;
    ULONG size = sizeof(profile);
    ULONG changeStamp = 0;
    return query(&kWnfQuietHoursActiveProfile, nullptr, nullptr, &changeStamp,
                 &profile, &size) >= 0 &&
           size == sizeof(profile) && profile != 0;
}

bool SetDoNotDisturb(bool on) {
    winrt::com_ptr<IQuietHoursSettings> settings;
    if (FAILED(CoCreateInstance(kCLSID_QuietHoursSettings, nullptr,
                                CLSCTX_LOCAL_SERVER, kIID_IQuietHoursSettings,
                                settings.put_void()))) {
        Wh_Log(L"The quiet hours settings aren't available");
        return false;
    }

    const HRESULT result = settings->put_UserSelectedProfile(
        on ? L"Microsoft.QuietHoursProfile.PriorityOnly"
           : L"Microsoft.QuietHoursProfile.Unrestricted");
    if (FAILED(result)) {
        Wh_Log(L"Switching \"Do not disturb\" failed: %08X", (unsigned)result);
    }
    return SUCCEEDED(result);
}

void ToggleDoNotDisturb() {
    const bool was = IsDoNotDisturbOn();
    const bool done = SetDoNotDisturb(!was);
    Wh_Log(L"Do Not Disturb was %s, set to %s: %s", was ? L"on" : L"off",
           was ? L"off" : L"on", done ? L"ok" : L"FAILED");
}

// -------------------------------------------------------------- tree walking --

FrameworkElement EnumChildElements(
    FrameworkElement element,
    std::function<bool(FrameworkElement)> enumCallback) {
    int childrenCount = Media::VisualTreeHelper::GetChildrenCount(element);

    for (int i = 0; i < childrenCount; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            continue;
        }
        if (enumCallback(child)) {
            return child;
        }
    }
    return nullptr;
}

FrameworkElement FindChildByName(FrameworkElement element, PCWSTR name) {
    return EnumChildElements(element, [name](FrameworkElement child) {
        return child.Name() == name;
    });
}

FrameworkElement FindChildByClassName(FrameworkElement element,
                                      PCWSTR className) {
    return EnumChildElements(element, [className](FrameworkElement child) {
        return winrt::get_class_name(child) == className;
    });
}

FrameworkElement EnumParentElements(
    FrameworkElement element,
    std::function<bool(FrameworkElement)> enumCallback) {
    auto parent = element;
    while (true) {
        parent = Media::VisualTreeHelper::GetParent(parent)
                     .try_as<FrameworkElement>();
        if (!parent) {
            return nullptr;
        }
        if (enumCallback(parent)) {
            return parent;
        }
    }
}

bool IsChildOfElementByName(FrameworkElement element, PCWSTR name) {
    return !!EnumParentElements(element, [name](FrameworkElement parent) {
        return parent.Name() == name;
    });
}

// Confirms the element really is the bell, rather than anything else that might
// end up under the notification centre button, by reading the glyph it draws.
bool LooksLikeTheBell(FrameworkElement iconView) {
    FrameworkElement child = FindChildByName(iconView, L"ContainerGrid");
    if (!child) {
        Wh_Log(L"  no ContainerGrid under the icon view");
        return false;
    }

    // ContentPresenter is only in the tree when the clock is hidden.
    if (FrameworkElement presenter = FindChildByName(child, L"ContentPresenter")) {
        child = presenter;
    }

    if (!(child = FindChildByName(child, L"ContentGrid")) ||
        !(child = FindChildByClassName(child, L"SystemTray.TextIconContent")) ||
        !(child = FindChildByName(child, L"ContainerGrid")) ||
        !(child = FindChildByName(child, L"Base")) ||
        !(child = FindChildByName(child, L"InnerTextBlock"))) {
        Wh_Log(L"  the tree under the icon view is not the expected shape");
        return false;
    }

    auto textBlock = child.try_as<Controls::TextBlock>();
    if (!textBlock) {
        return false;
    }

    auto text = textBlock.Text();
    Wh_Log(L"  glyph: U+%04X (length %d)", text.size() ? (unsigned)text[0] : 0,
           (int)text.size());
    if (text.size() != 1) {
        return false;
    }

    switch (text[0]) {
        case L'\uF2A3':  // Empty bell
        case L'\uF285':  // Empty bell, Do Not Disturb
        case L'\uF2A5':  // Full bell
        case L'\uF2A8':  // Full bell, Do Not Disturb
            return true;
        default:
            return false;
    }
}

// ---------------------------------------------------------------- the click --

struct AttachedHandler {
    FrameworkElement element = nullptr;
    RoutedEvent routedEvent = nullptr;
    winrt::Windows::Foundation::IInspectable handler;
};

std::list<AttachedHandler> g_attached;
std::list<winrt::event_revoker<IFrameworkElement>> g_loadedRevokers;

// Handling the pointer events is not enough on its own: the tray control
// raises Tapped separately afterwards, and that is what opens the notification
// centre. So a press opens a short window during which the next Tapped is
// swallowed - the same approach Separate System Tray Icons uses for its
// middle-click handling.
ULONGLONG g_suppressTapUntil = 0;
bool g_pressArmed = false;  // the press matched the configured button

// Short, because the taps land within a couple of milliseconds of the release
// - 13:21:48.862 released, 13:21:48.869 to .870 tapped - and a window any
// longer than it needs to be would start eating clicks on the clock beside it.
constexpr ULONGLONG kTapSuppressionMs = 400;

void SuppressTapsBriefly() {
    g_suppressTapUntil = GetTickCount64() + kTapSuppressionMs;
}

// Deliberately does not clear the window: every tap inside it is swallowed, not
// just the first. It expires on time instead.
bool TapIsSuppressed() {
    return g_suppressTapUntil && GetTickCount64() <= g_suppressTapUntil;
}

FrameworkElement GetParentElementByName(FrameworkElement element, PCWSTR name) {
    return EnumParentElements(element, [name](FrameworkElement parent) {
        return parent.Name() == name;
    });
}

void AddTrackedHandler(FrameworkElement element,
                       RoutedEvent routedEvent,
                       winrt::Windows::Foundation::IInspectable handler) {
    element.AddHandler(routedEvent, handler, true);
    g_attached.push_back({element, routedEvent, handler});
}

bool MatchesChosenButton(
    winrt::Windows::UI::Input::PointerPointProperties const& props) {
    switch (g_settings.mouseButton) {
        case MouseButton::middle:
            return props.IsMiddleButtonPressed();
        case MouseButton::right:
            return props.IsRightButtonPressed();
        case MouseButton::left:
        default:
            return props.IsLeftButtonPressed();
    }
}

// On release no button is down any more, so IsLeftButtonPressed and friends are
// all false. PointerUpdateKind is the one that says which button just came up.
bool ReleaseMatchesChosenButton(
    winrt::Windows::UI::Input::PointerPointProperties const& props) {
    using Kind = winrt::Windows::UI::Input::PointerUpdateKind;
    switch (props.PointerUpdateKind()) {
        case Kind::LeftButtonReleased:
            return g_settings.mouseButton == MouseButton::left;
        case Kind::MiddleButtonReleased:
            return g_settings.mouseButton == MouseButton::middle;
        case Kind::RightButtonReleased:
            return g_settings.mouseButton == MouseButton::right;
        default:
            // Touch, pen, or a control that does not report a kind. Treat it as
            // the configured button rather than doing nothing at all.
            Wh_Log(L"Release reported no button kind; treating it as a match");
            return true;
    }
}

// The button this sits inside consumes pointer events in its class handler, so
// the handlers are registered with handledEventsToo and capture the pointer:
// without a matching release the button never raises its own click, which is
// what keeps the notification centre shut.
void AttachToBell(FrameworkElement iconView) {
    auto pressedHandler = input::PointerEventHandler(
        [](winrt::Windows::Foundation::IInspectable const& sender,
           input::PointerRoutedEventArgs const& args) {
            if (g_unloading) {
                return;
            }
            auto element = sender.try_as<UIElement>();
            if (!element) {
                return;
            }
            auto point = args.GetCurrentPoint(element);
            auto props = point.Properties();
            if (!MatchesChosenButton(props)) {
                Wh_Log(L"Bell pressed with the wrong button "
                       L"(left=%d middle=%d right=%d, configured=%s) - ignoring",
                       (int)props.IsLeftButtonPressed(),
                       (int)props.IsMiddleButtonPressed(),
                       (int)props.IsRightButtonPressed(),
                       g_settings.mouseButton == MouseButton::left     ? L"left"
                       : g_settings.mouseButton == MouseButton::middle ? L"middle"
                                                                       : L"right");
                return;
            }
            g_pressArmed = true;
            Wh_Log(L"Bell pressed, opening the tap suppression window");
            element.CapturePointer(args.Pointer());
            if (!g_settings.alsoOpenCentre) {
                SuppressTapsBriefly();
                args.Handled(true);
            }
        });

    auto releasedHandler = input::PointerEventHandler(
        [](winrt::Windows::Foundation::IInspectable const& sender,
           input::PointerRoutedEventArgs const& args) {
            if (g_unloading) {
                return;
            }
            auto element = sender.try_as<UIElement>();
            if (!element) {
                return;
            }
            element.ReleasePointerCapture(args.Pointer());

            auto props = args.GetCurrentPoint(element).Properties();
            if (!g_pressArmed && !ReleaseMatchesChosenButton(props)) {
                Wh_Log(L"Bell released with a button other than the configured "
                       L"one - ignoring");
                return;
            }
            g_pressArmed = false;

            // Tapped is raised after this, so arming here is in time to swallow
            // it even when the press handler never ran.
            if (!g_settings.alsoOpenCentre) {
                SuppressTapsBriefly();
                args.Handled(true);
            }
            Wh_Log(L"Bell released, toggling");
            ToggleDoNotDisturb();
        });

    auto tappedHandler = input::TappedEventHandler(
        [](winrt::Windows::Foundation::IInspectable const&,
           input::TappedRoutedEventArgs const& args) {
            if (g_unloading || g_settings.alsoOpenCentre) {
                return;
            }
            if (TapIsSuppressed()) {
                Wh_Log(L"Tapped seen, suppression window live -> swallowed");
                args.Handled(true);
            } else {
                Wh_Log(L"Tapped seen, but no suppression window was open");
            }
        });

    auto doubleTappedHandler = input::DoubleTappedEventHandler(
        [](winrt::Windows::Foundation::IInspectable const&,
           input::DoubleTappedRoutedEventArgs const& args) {
            if (g_unloading || g_settings.alsoOpenCentre) {
                return;
            }
            Wh_Log(L"Swallowed a double tap");
            args.Handled(true);
        });

    auto cancelledHandler = input::PointerEventHandler(
        [](winrt::Windows::Foundation::IInspectable const&,
           input::PointerRoutedEventArgs const&) { g_pressArmed = false; });

    AddTrackedHandler(iconView, UIElement::PointerCanceledEvent(),
                      winrt::box_value(cancelledHandler));
    AddTrackedHandler(iconView, UIElement::PointerCaptureLostEvent(),
                      winrt::box_value(cancelledHandler));
    AddTrackedHandler(iconView, UIElement::PointerPressedEvent(),
                      winrt::box_value(pressedHandler));
    AddTrackedHandler(iconView, UIElement::PointerReleasedEvent(),
                      winrt::box_value(releasedHandler));
    AddTrackedHandler(iconView, UIElement::TappedEvent(),
                      winrt::box_value(tappedHandler));
    AddTrackedHandler(iconView, UIElement::DoubleTappedEvent(),
                      winrt::box_value(doubleTappedHandler));

    // The tap is raised by the button, not the icon, on some builds.
    if (auto button =
            GetParentElementByName(iconView, L"NotificationCenterButton")) {
        AddTrackedHandler(button, UIElement::TappedEvent(),
                          winrt::box_value(tappedHandler));
        AddTrackedHandler(button, UIElement::DoubleTappedEvent(),
                          winrt::box_value(doubleTappedHandler));
        Wh_Log(L"Attached to the bell and to the notification centre button");
    } else {
        Wh_Log(L"Attached to the bell only; the button was not found above it");
    }
}

void DetachAll() {
    for (auto& attached : g_attached) {
        if (!attached.element || !attached.routedEvent) {
            continue;
        }
        try {
            attached.element.RemoveHandler(attached.routedEvent,
                                           attached.handler);
        } catch (...) {
            // The element may already be gone with the taskbar it lived in.
        }
    }
    g_attached.clear();
    g_loadedRevokers.clear();
}

// ------------------------------------------------------------------- hooks --

using IconView_IconView_t = void*(WINAPI*)(void* pThis);
IconView_IconView_t IconView_IconView_Original;

void* WINAPI IconView_IconView_Hook(void* pThis) {
    Wh_Log(L"IconView constructed");
    void* ret = IconView_IconView_Original(pThis);

    FrameworkElement iconView = nullptr;
    ((IUnknown**)pThis)[1]->QueryInterface(winrt::guid_of<FrameworkElement>(),
                                           winrt::put_abi(iconView));
    if (!iconView) {
        return ret;
    }

    g_loadedRevokers.emplace_back();
    auto revokerIt = g_loadedRevokers.end();
    --revokerIt;

    *revokerIt = iconView.Loaded(
        winrt::auto_revoke_t{},
        [revokerIt](winrt::Windows::Foundation::IInspectable const& sender,
                    RoutedEventArgs const&) {
            g_loadedRevokers.erase(revokerIt);

            auto iconView = sender.try_as<FrameworkElement>();
            if (!iconView || g_unloading) {
                return;
            }

            auto className = winrt::get_class_name(iconView);
            Wh_Log(L"Icon view loaded: class=%s name=%s", className.c_str(),
                   iconView.Name().c_str());

            if (className != L"SystemTray.IconView" ||
                iconView.Name() != L"SystemTrayIcon") {
                return;
            }
            if (!IsChildOfElementByName(iconView, L"NotificationCenterButton")) {
                Wh_Log(L"  not under NotificationCenterButton, skipping");
                return;
            }
            Wh_Log(L"  under NotificationCenterButton");
            if (!LooksLikeTheBell(iconView)) {
                Wh_Log(L"  but the glyph is not one of the four bells, skipping");
                return;
            }
            AttachToBell(iconView);
        });

    return ret;
}

VS_FIXEDFILEINFO* GetModuleVersionInfo(HMODULE hModule, UINT* puPtrLen) {
    void* pFixedFileInfo = nullptr;
    UINT uPtrLen = 0;

    HRSRC hResource =
        FindResource(hModule, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    if (hResource) {
        if (HGLOBAL hGlobal = LoadResource(hModule, hResource)) {
            if (void* pData = LockResource(hGlobal)) {
                if (!VerQueryValue(pData, L"\\", &pFixedFileInfo, &uPtrLen) ||
                    uPtrLen == 0) {
                    pFixedFileInfo = nullptr;
                    uPtrLen = 0;
                }
            }
        }
    }

    if (puPtrLen) {
        *puPtrLen = uPtrLen;
    }
    return (VS_FIXEDFILEINFO*)pFixedFileInfo;
}

HMODULE GetSystemTrayModuleHandle() {
    HMODULE module = GetModuleHandle(L"SystemTray.dll");
    if (!module) {
        module = GetModuleHandle(L"Taskbar.View.dll");
        if (module) {
            // The first known build without SystemTray.dll is
            // Taskbar.View.dll 2604.8002.200.6000.
            VS_FIXEDFILEINFO* fixedFileInfo =
                GetModuleVersionInfo(module, nullptr);
            WORD moduleMajor =
                fixedFileInfo ? HIWORD(fixedFileInfo->dwFileVersionMS) : 0;
            if (!moduleMajor || moduleMajor >= 2604) {
                module = nullptr;
            }
        }
    }
    return module;
}

bool HookSystemTraySymbols(HMODULE module) {
    WindhawkUtils::SYMBOL_HOOK symbolHooks[] = {
        {
            {LR"(public: __cdecl winrt::SystemTray::implementation::IconView::IconView(void))"},
            &IconView_IconView_Original,
            IconView_IconView_Hook,
        },
    };

    if (!WindhawkUtils::HookSymbols(module, symbolHooks,
                                    ARRAYSIZE(symbolHooks))) {
        Wh_Log(L"HookSymbols failed - the IconView symbol was not found");
        return false;
    }
    Wh_Log(L"IconView hook installed");
    return true;
}

void HandleLoadedModuleIfSystemTray(HMODULE module, LPCWSTR lpLibFileName) {
    if (!g_systemTrayModuleHooked && GetSystemTrayModuleHandle() == module &&
        !g_systemTrayModuleHooked.exchange(true)) {
        Wh_Log(L"Loaded %s", lpLibFileName);
        if (HookSystemTraySymbols(module)) {
            Wh_ApplyHookOperations();
        }
    }
}

using LoadLibraryExW_t = decltype(&LoadLibraryExW);
LoadLibraryExW_t LoadLibraryExW_Original;

HMODULE WINAPI LoadLibraryExW_Hook(LPCWSTR lpLibFileName,
                                   HANDLE hFile,
                                   DWORD dwFlags) {
    HMODULE module = LoadLibraryExW_Original(lpLibFileName, hFile, dwFlags);
    if (module) {
        HandleLoadedModuleIfSystemTray(module, lpLibFileName);
    }
    return module;
}

// ------------------------------------------------------------- mod plumbing --

void LoadSettings() {
    PCWSTR button = Wh_GetStringSetting(L"mouseButton");
    g_settings.mouseButton = MouseButton::left;
    if (button) {
        if (wcscmp(button, L"middle") == 0) {
            g_settings.mouseButton = MouseButton::middle;
        } else if (wcscmp(button, L"right") == 0) {
            g_settings.mouseButton = MouseButton::right;
        }
    }
    Wh_FreeStringSetting(button);

    g_settings.alsoOpenCentre = Wh_GetIntSetting(L"alsoOpenCentre") != 0;

    Wh_Log(L"Settings: button=%s alsoOpenCentre=%d",
           g_settings.mouseButton == MouseButton::left     ? L"left"
           : g_settings.mouseButton == MouseButton::middle ? L"middle"
                                                           : L"right",
           (int)g_settings.alsoOpenCentre);
}

BOOL Wh_ModInit() {
    Wh_Log(L">");

    LoadSettings();

    if (HMODULE systemTrayModule = GetSystemTrayModuleHandle()) {
        g_systemTrayModuleHooked = true;
        if (!HookSystemTraySymbols(systemTrayModule)) {
            return FALSE;
        }
    } else {
        Wh_Log(L"System tray module not loaded yet");

        HMODULE kernelBaseModule = GetModuleHandle(L"kernelbase.dll");
        auto pKernelBaseLoadLibraryExW =
            (decltype(&LoadLibraryExW))GetProcAddress(kernelBaseModule,
                                                      "LoadLibraryExW");
        WindhawkUtils::SetFunctionHook(pKernelBaseLoadLibraryExW,
                                       LoadLibraryExW_Hook,
                                       &LoadLibraryExW_Original);
    }

    return TRUE;
}

void Wh_ModSettingsChanged() {
    LoadSettings();
}

void Wh_ModUninit() {
    Wh_Log(L">");
    g_unloading = true;
    DetachAll();
}
