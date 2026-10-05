; Toggle Windows 11 "Do not disturb"                          AutoHotkey v2
;
; Reads the state from the WNF notification Windows publishes, and sets it
; through IQuietHoursSettings - the same undocumented interface the Windows
; notification centre itself uses. No UI is opened and nothing flickers; the
; toggle is one COM call.
;
; "Do not disturb on" is the priority-only quiet hours profile, which is what
; the switch in the notification centre selects.
;
; Hotkey: Win+Shift+D. Change it at the bottom.

#Requires AutoHotkey v2.0
#SingleInstance Force

; {F53321FA-34F8-4B7F-B9A3-361877CB94CF}
CLSID_QuietHoursSettings := "{F53321FA-34F8-4B7F-B9A3-361877CB94CF}"
; {6BFF4732-81EC-4FFB-AE67-B6C1BC29631F}
IID_IQuietHoursSettings  := "{6BFF4732-81EC-4FFB-AE67-B6C1BC29631F}"

PROFILE_ON  := "Microsoft.QuietHoursProfile.PriorityOnly"
PROFILE_OFF := "Microsoft.QuietHoursProfile.Unrestricted"

CLSCTX_LOCAL_SERVER := 4

; Vtable slots after IUnknown's three:
VT_GET_PROFILE := 3
VT_PUT_PROFILE := 4

GuidBuffer(text) {
    buf := Buffer(16, 0)
    if (DllCall("ole32\CLSIDFromString", "WStr", text, "Ptr", buf, "Int") != 0)
        throw Error("Malformed GUID: " text)
    return buf
}

; Returns an IQuietHoursSettings pointer, or 0. Release it with ObjRelease.
QuietHoursSettings() {
    global CLSID_QuietHoursSettings, IID_IQuietHoursSettings, CLSCTX_LOCAL_SERVER
    DllCall("ole32\CoInitializeEx", "Ptr", 0, "UInt", 2)   ; APARTMENTTHREADED
    ptr := 0
    hr := DllCall("ole32\CoCreateInstance"
        , "Ptr",  GuidBuffer(CLSID_QuietHoursSettings)
        , "Ptr",  0
        , "UInt", CLSCTX_LOCAL_SERVER
        , "Ptr",  GuidBuffer(IID_IQuietHoursSettings)
        , "Ptr*", &ptr
        , "Int")
    return (hr = 0) ? ptr : 0
}

; 1 = on, 0 = off, -1 = could not read.
; The shell publishes the active quiet-hours profile here: 0 means none.
DndState() {
    stateName := Buffer(8, 0)
    NumPut("UInt64", 0xD83063EA3BF1C75, stateName)
    stamp := Buffer(4, 0)
    value := Buffer(4, 0)
    size  := Buffer(4, 0)
    NumPut("UInt", 4, size)

    status := DllCall("ntdll\NtQueryWnfStateData"
        , "Ptr", stateName, "Ptr", 0, "Ptr", 0
        , "Ptr", stamp, "Ptr", value, "Ptr", size, "Int")
    if (status < 0)
        return -1
    return NumGet(value, 0, "Int") ? 1 : 0
}

SetDnd(on) {
    global PROFILE_ON, PROFILE_OFF, VT_PUT_PROFILE
    ptr := QuietHoursSettings()
    if (!ptr)
        return false
    hr := ComCall(VT_PUT_PROFILE, ptr, "WStr", on ? PROFILE_ON : PROFILE_OFF, "Int")
    ObjRelease(ptr)
    return hr >= 0
}

ToggleDnd() {
    before := DndState()
    if (before = -1) {
        MsgBox "Could not read the Do Not Disturb state.`n`n"
             . "NtQueryWnfStateData failed, so nothing was changed.",
               "Toggle DND", 48
        return
    }

    want := !before
    if (!SetDnd(want)) {
        MsgBox "The quiet hours settings were not available.`n`n"
             . "CoCreateInstance on IQuietHoursSettings failed - this is an "
             . "undocumented interface, so a Windows update may have moved it.",
               "Toggle DND", 48
        return
    }

    ; Confirm against the state Windows actually publishes, rather than
    ; trusting the call's return value.
    Sleep 120
    after := DndState()
    ToolTip "Do not disturb: " (after = 1 ? "on" : "off")
    SetTimer () => ToolTip(), -1200
}

#+d::ToggleDnd()
