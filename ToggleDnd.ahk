; Toggle Windows 11 "Do not disturb"                          AutoHotkey v2
;
; Windows exposes no API for setting Do Not Disturb. Reading it is clean - the
; shell publishes a WNF state - but the only supported way to CHANGE it is the
; button in the Notification Center. So this script drives that real button and
; then reads the WNF state back to confirm it actually flipped.
;
; First run: it does not know how many Tab presses reach the button on your
; build, so it finds out once, tells you, and remembers. Every run after that
; is instant.
;
; Hotkey: Win+Shift+D. Change it at the bottom.

#Requires AutoHotkey v2.0
#SingleInstance Force

; The shell publishes DND state here. Verified against the Dynamic Island mod,
; which subscribes to this same state to show its DND indicator.
WNF_QUIET_HOURS_ACTIVE_PROFILE_CHANGED := 0xD83063EA3BF1C75

TabCountFile := A_ScriptDir "\ToggleDnd.tabs.txt"
MaxTabs := 14

; Returns 1 = on, 0 = off, -1 = could not read.
DndState() {
    global WNF_QUIET_HOURS_ACTIVE_PROFILE_CHANGED
    stateName := Buffer(8, 0)
    NumPut("UInt64", WNF_QUIET_HOURS_ACTIVE_PROFILE_CHANGED, stateName)
    stamp := Buffer(4, 0)
    value := Buffer(4, 0)
    size  := Buffer(4, 0)
    NumPut("UInt", 4, size)

    status := DllCall("ntdll\NtQueryWnfStateData"
        , "Ptr", stateName, "Ptr", 0, "Ptr", 0
        , "Ptr", stamp, "Ptr", value, "Ptr", size, "Int")
    if (status != 0)
        return -1
    return NumGet(value, 0, "Int") ? 1 : 0
}

OpenNotificationCentre() {
    Send "#n"
    Sleep 420              ; the flyout animates in; clicking early misses it
}

CloseNotificationCentre() {
    Send "{Esc}"
    Sleep 160
}

; Walks Shift+Tab backwards from where focus lands - the DND button sits near
; the top of the panel, so backwards reaches it in fewer steps than forwards.
PressButtonAfter(tabs) {
    Loop tabs
        Send "+{Tab}"
    Sleep 60
    Send "{Enter}"
    Sleep 300
}

LoadTabCount() {
    global TabCountFile
    if !FileExist(TabCountFile)
        return 0
    try return Integer(Trim(FileRead(TabCountFile)))
    return 0
}

SaveTabCount(n) {
    global TabCountFile
    try FileDelete TabCountFile
    try FileAppend String(n), TabCountFile
}

; Tries 1..MaxTabs until the WNF state actually changes. Each miss is undone by
; closing the panel, so nothing else gets clicked by accident.
DiscoverTabCount() {
    global MaxTabs
    before := DndState()
    if (before = -1) {
        MsgBox "Could not read the Do Not Disturb state.`n`n"
             . "NtQueryWnfStateData failed, which usually means this build "
             . "moved the state. Nothing was changed.", "Toggle DND", 48
        return 0
    }

    Loop MaxTabs {
        n := A_Index
        OpenNotificationCentre()
        PressButtonAfter(n)
        CloseNotificationCentre()

        if (DndState() != before) {
            SaveTabCount(n)
            MsgBox "Found it: " n " press" (n = 1 ? "" : "es") " of Shift+Tab.`n`n"
                 . "Saved, so from now on Win+Shift+D toggles straight away.",
                   "Toggle DND", 64
            return n
        }
    }

    MsgBox "Could not find the Do not disturb button by keyboard in "
         . MaxTabs " steps.`n`n"
         . "Open the Notification Center with Win+N and check the button is "
         . "there. If it is, raise MaxTabs at the top of this script.",
           "Toggle DND", 48
    return 0
}

ToggleDnd() {
    tabs := LoadTabCount()
    if (tabs = 0) {
        DiscoverTabCount()
        return
    }

    before := DndState()
    OpenNotificationCentre()
    PressButtonAfter(tabs)
    CloseNotificationCentre()

    after := DndState()
    if (before != -1 && after = before) {
        ; The layout moved - a Windows update, or the panel opened differently.
        ; Re-learn rather than silently doing nothing.
        SaveTabCount(0)
        DiscoverTabCount()
        return
    }

    ToolTip "Do not disturb: " (after = 1 ? "on" : "off")
    SetTimer () => ToolTip(), -1200
}

#+d::ToggleDnd()
