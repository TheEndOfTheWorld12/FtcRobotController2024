// ==WindhawkMod==
// @id              browser-media-bridge
// @name            Browser Media Bridge
// @description     Makes each browser tab's media visible to other mods, which Windows on its own cannot do
// @version         1.0
// @author          aariv
// @include         windhawk.exe
// @architecture    x86-64
// @compilerOptions -lws2_32 -lbcrypt -lcrypt32 -luser32 -lshell32 -ladvapi32 -lgdi32 -lgdiplus
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Browser Media Bridge

Windows cannot see your browser tabs individually. Chromium's own
`system_media_controls` README says it outright:

> Normally, SMC is a singleton and all Chrome tabs and windows share an
> instance.

So `GlobalSystemMediaTransportControlsSessionManager` reports **one** session
for the whole browser, no matter how many tabs are playing. Only installed web
apps (dPWAs) get their own. That is a limit of the API, not of any mod: no
amount of work on the Windows side can separate tabs that the OS never
separated.

A browser extension can. This mod is the other half of that: it listens on a
loopback socket, the extension reports each tab, and the mod publishes the
result to shared memory where any other mod can read it.

```
  content script  ->  background worker  ==websocket==>  this mod  ->  shared
  (one per tab)       (one connection)    127.0.0.1       memory  ->  TopBar,
                                                                      etc.
```

## Setting it up

1. Turn this mod on. A tray icon appears.
2. Right-click the tray icon and choose **Copy connection URL**.
3. Load the extension from `browser-media-bridge-extension/` (its `README.md`
   has the unpacked-install steps), open its options, and paste the URL in.
4. The tray icon turns solid once the extension connects, and its tooltip
   lists the tabs.

The tray menu lists every reporting tab; clicking one toggles play/pause in
that tab, which is there to prove the reverse channel end to end.

## Why a socket rather than native messaging

Chrome's native messaging launches a **separate executable** and talks to it
over stdin/stdout. A Windhawk mod is a DLL inside a host process, so that
would mean a broker process plus a registry manifest to point at it. A
loopback socket needs neither, and it is bidirectional, so the mod can push
commands back down the same connection.

## Security

This is a listening socket, so it is worth being plain about the threats.

- It binds **127.0.0.1 only**, never `0.0.0.0`, so nothing off this machine
  can reach it.
- **A web page can also reach a loopback port.** CORS stops a page reading the
  reply, but the request still arrives. So the connection URL carries a random
  token, generated on first run, and a connection without it is closed
  immediately.
- The protocol carries media state and media commands and nothing else. There
  is no verb that touches the filesystem, runs anything, or is forwarded
  anywhere. The worst a leaked token buys is pausing your music and reading
  what is playing.
- Turn **Allow control** off to make the socket read-only.

## Settings

* **Port** - which loopback port to listen on.
* **Token** - leave blank and one is generated on first run. Changing it
  disconnects the extension until the new URL is pasted in again.
* **Allow control** - whether commands may be sent back to the browser.
* **Show tray icon** - the icon and its menu.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- port: 49872
  $name: Port
  $description: The loopback port to listen on. Change it only if something else already uses this one.
- token: ""
  $name: Token
  $description: Shared secret the extension must present. Leave blank to have one generated on first run.
- allowControl: true
  $name: Allow control
  $description: Let other mods send play, pause, next, previous, seek, volume and focus commands to a tab. Turn off to make the bridge read-only.
- showTrayIcon: true
  $name: Show tray icon
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <windows.h>
// Deliberately NOT winsock2.h. Windhawk force-includes windhawk_api.h, which
// includes windows.h, which pulls in winsock.h (1.1) first -- so a later
// winsock2.h warns "Please include winsock2.h before windows.h" and there is
// no way to get ahead of it from inside a mod. Everything this mod uses
// (socket, bind, listen, select, accept, recv, send, setsockopt, SO_RCVTIMEO,
// WSAGetLastError) is Winsock 1.1, already declared by windows.h, and
// -lws2_32 exports all of it. So the include is dropped and the warning with
// it.
#include <bcrypt.h>
#include <wincrypt.h>
#include <shellapi.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// Shared state
//
// The socket threads write it and the UI thread reads it, so everything in
// here goes through g_stateMutex.

struct TabInfo {
    long long id = 0;
    bool playing = false;
    bool muted = false;
    int volume = 100;         // 0-100
    double position = 0.0;    // seconds
    double duration = 0.0;    // seconds, 0 when unknown (a live stream)
    std::wstring title;
    std::wstring artist;
    std::wstring site;        // hostname, so two tabs on one site are telling
};

std::mutex g_stateMutex;
std::vector<TabInfo> g_tabs;
bool g_extensionConnected = false;
unsigned long long g_lastReportTick = 0;

struct {
    int port = 49872;
    std::wstring token;
    bool allowControl = true;
    bool showTrayIcon = true;
} g_settings;

////////////////////////////////////////////////////////////////////////////////
// Published snapshot
//
// Other mods read this rather than opening a socket of their own: one owner of
// the port, and a reader needs about forty lines. The layout is fixed and
// versioned; a reader that does not recognise kSnapshotVersion should back off
// rather than guess.

constexpr wchar_t kSnapshotName[] = L"Local\\WindhawkBrowserMediaBridge";
constexpr unsigned int kSnapshotVersion = 1;
constexpr int kMaxTabs = 24;
constexpr int kTextChars = 128;

struct SnapshotTab {
    long long id;
    int playing;
    int muted;
    int volume;
    double position;
    double duration;
    wchar_t title[kTextChars];
    wchar_t artist[kTextChars];
    wchar_t site[kTextChars];
};

struct Snapshot {
    unsigned int version;
    unsigned int sequence;       // bumped after each write; odd means mid-write
    unsigned int tabCount;
    unsigned int connected;
    unsigned long long tick;     // GetTickCount64 of the last report
    // Commands travel the other way in the same section. A writer fills the
    // fields, then bumps commandSequence last.
    unsigned int commandSequence;
    long long commandTab;
    int commandValue;
    wchar_t commandVerb[32];
    SnapshotTab tabs[kMaxTabs];
};

HANDLE g_snapshotMapping = nullptr;
Snapshot* g_snapshot = nullptr;
unsigned int g_lastSeenCommandSequence = 0;

void CopyFixed(wchar_t* dest, size_t destChars, const std::wstring& text) {
    if (!destChars) {
        return;
    }
    size_t n = text.size();
    if (n > destChars - 1) {
        n = destChars - 1;
    }
    memcpy(dest, text.c_str(), n * sizeof(wchar_t));
    dest[n] = L'\0';
}

bool CreateSnapshot() {
    g_snapshotMapping =
        CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                           sizeof(Snapshot), kSnapshotName);
    if (!g_snapshotMapping) {
        Wh_Log(L"CreateFileMapping failed (%u)", GetLastError());
        return false;
    }
    g_snapshot = (Snapshot*)MapViewOfFile(g_snapshotMapping, FILE_MAP_WRITE, 0,
                                          0, sizeof(Snapshot));
    if (!g_snapshot) {
        Wh_Log(L"MapViewOfFile failed (%u)", GetLastError());
        CloseHandle(g_snapshotMapping);
        g_snapshotMapping = nullptr;
        return false;
    }
    // A second instance would find the section already populated, so only the
    // fields this instance owns are reset.
    g_snapshot->version = kSnapshotVersion;
    g_snapshot->tabCount = 0;
    g_snapshot->connected = 0;
    g_snapshot->tick = GetTickCount64();
    g_lastSeenCommandSequence = g_snapshot->commandSequence;
    return true;
}

void DestroySnapshot() {
    if (g_snapshot) {
        g_snapshot->connected = 0;
        g_snapshot->tabCount = 0;
        UnmapViewOfFile(g_snapshot);
        g_snapshot = nullptr;
    }
    if (g_snapshotMapping) {
        CloseHandle(g_snapshotMapping);
        g_snapshotMapping = nullptr;
    }
}

// Call with g_stateMutex held.
void PublishSnapshotLocked() {
    if (!g_snapshot) {
        return;
    }
    // Odd sequence while writing, so a reader that samples it before and after
    // can tell it got a consistent copy without taking a lock.
    g_snapshot->sequence++;
    MemoryBarrier();

    unsigned int count = (unsigned int)g_tabs.size();
    if (count > kMaxTabs) {
        count = kMaxTabs;
    }
    for (unsigned int i = 0; i < count; i++) {
        const TabInfo& src = g_tabs[i];
        SnapshotTab& dst = g_snapshot->tabs[i];
        dst.id = src.id;
        dst.playing = src.playing ? 1 : 0;
        dst.muted = src.muted ? 1 : 0;
        dst.volume = src.volume;
        dst.position = src.position;
        dst.duration = src.duration;
        CopyFixed(dst.title, kTextChars, src.title);
        CopyFixed(dst.artist, kTextChars, src.artist);
        CopyFixed(dst.site, kTextChars, src.site);
    }
    g_snapshot->tabCount = count;
    g_snapshot->connected = g_extensionConnected ? 1 : 0;
    g_snapshot->tick = g_lastReportTick;

    MemoryBarrier();
    g_snapshot->sequence++;
}

////////////////////////////////////////////////////////////////////////////////
// Text helpers

std::string ToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return std::string();
    }
    int needed = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(),
                                     nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return std::string();
    }
    std::string out((size_t)needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), out.data(),
                        needed, nullptr, nullptr);
    return out;
}

std::wstring FromUtf8(const char* data, size_t length) {
    if (!length) {
        return std::wstring();
    }
    int needed =
        MultiByteToWideChar(CP_UTF8, 0, data, (int)length, nullptr, 0);
    if (needed <= 0) {
        return std::wstring();
    }
    std::wstring out((size_t)needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, data, (int)length, out.data(), needed);
    return out;
}

std::string TrimAscii(const std::string& text) {
    size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return std::string();
    }
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

////////////////////////////////////////////////////////////////////////////////
// WebSocket
//
// Only what this protocol needs: the opening handshake, text frames, ping and
// close. No extensions, no fragmentation on the way out, no binary frames.

bool Sha1(const void* data, size_t length, unsigned char out[20]) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM,
                                                    nullptr, 0))) {
        return false;
    }
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool ok = BCRYPT_SUCCESS(
        BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0));
    if (ok) {
        ok = BCRYPT_SUCCESS(BCryptHashData(hash, (PUCHAR)data, (ULONG)length, 0));
    }
    if (ok) {
        ok = BCRYPT_SUCCESS(BCryptFinishHash(hash, out, 20, 0));
    }
    if (hash) {
        BCryptDestroyHash(hash);
    }
    BCryptCloseAlgorithmProvider(alg, 0);
    return ok;
}

std::string Base64(const unsigned char* data, size_t length) {
    DWORD chars = 0;
    if (!CryptBinaryToStringA(data, (DWORD)length,
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                              nullptr, &chars) ||
        !chars) {
        return std::string();
    }
    std::string out((size_t)chars, '\0');
    if (!CryptBinaryToStringA(data, (DWORD)length,
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                              out.data(), &chars)) {
        return std::string();
    }
    out.resize(strlen(out.c_str()));
    return out;
}

std::string AcceptKey(const std::string& clientKey) {
    static const char kMagic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = clientKey + kMagic;
    unsigned char digest[20];
    if (!Sha1(combined.data(), combined.size(), digest)) {
        return std::string();
    }
    return Base64(digest, sizeof(digest));
}

// recv returns whatever has arrived, which is not necessarily what was asked
// for; every header and frame field here needs all of it.
bool RecvExact(SOCKET sock, void* buffer, size_t length) {
    char* at = (char*)buffer;
    size_t left = length;
    while (left) {
        int got = recv(sock, at, (int)left, 0);
        if (got <= 0) {
            return false;
        }
        at += got;
        left -= (size_t)got;
    }
    return true;
}

bool SendAll(SOCKET sock, const void* buffer, size_t length) {
    const char* at = (const char*)buffer;
    size_t left = length;
    while (left) {
        int sent = send(sock, at, (int)left, 0);
        if (sent <= 0) {
            return false;
        }
        at += sent;
        left -= (size_t)sent;
    }
    return true;
}

bool SendTextFrame(SOCKET sock, const std::string& payload) {
    std::vector<unsigned char> frame;
    frame.push_back(0x81);  // FIN + text
    const size_t n = payload.size();
    if (n < 126) {
        frame.push_back((unsigned char)n);
    } else if (n <= 0xFFFF) {
        frame.push_back(126);
        frame.push_back((unsigned char)((n >> 8) & 0xFF));
        frame.push_back((unsigned char)(n & 0xFF));
    } else {
        frame.push_back(127);
        for (int shift = 56; shift >= 0; shift -= 8) {
            frame.push_back((unsigned char)((n >> shift) & 0xFF));
        }
    }
    // A server must not mask what it sends.
    frame.insert(frame.end(), payload.begin(), payload.end());
    return SendAll(sock, frame.data(), frame.size());
}

enum class FrameKind { Text, Ping, Pong, Close, Other, Error };

FrameKind ReadFrame(SOCKET sock, std::string* payloadOut) {
    payloadOut->clear();
    unsigned char head[2];
    if (!RecvExact(sock, head, 2)) {
        return FrameKind::Error;
    }
    const unsigned char opcode = head[0] & 0x0F;
    const bool masked = (head[1] & 0x80) != 0;
    unsigned long long length = head[1] & 0x7F;
    if (length == 126) {
        unsigned char ext[2];
        if (!RecvExact(sock, ext, 2)) {
            return FrameKind::Error;
        }
        length = ((unsigned long long)ext[0] << 8) | ext[1];
    } else if (length == 127) {
        unsigned char ext[8];
        if (!RecvExact(sock, ext, 8)) {
            return FrameKind::Error;
        }
        length = 0;
        for (int i = 0; i < 8; i++) {
            length = (length << 8) | ext[i];
        }
    }
    // A report of two dozen tabs is a few kilobytes; anything far past that is
    // not this protocol, so refuse rather than allocate for it.
    if (length > 1024 * 1024) {
        return FrameKind::Error;
    }
    unsigned char mask[4] = {0, 0, 0, 0};
    if (masked && !RecvExact(sock, mask, 4)) {
        return FrameKind::Error;
    }
    payloadOut->resize((size_t)length);
    if (length && !RecvExact(sock, payloadOut->data(), (size_t)length)) {
        return FrameKind::Error;
    }
    if (masked) {
        for (size_t i = 0; i < payloadOut->size(); i++) {
            (*payloadOut)[i] = (char)((unsigned char)(*payloadOut)[i] ^ mask[i % 4]);
        }
    }
    switch (opcode) {
        case 0x1: return FrameKind::Text;
        case 0x8: return FrameKind::Close;
        case 0x9: return FrameKind::Ping;
        case 0xA: return FrameKind::Pong;
        default:  return FrameKind::Other;
    }
}

bool SendPong(SOCKET sock, const std::string& payload) {
    std::vector<unsigned char> frame;
    frame.push_back(0x8A);
    frame.push_back((unsigned char)(payload.size() & 0x7F));
    frame.insert(frame.end(), payload.begin(), payload.end());
    return SendAll(sock, frame.data(), frame.size());
}

////////////////////////////////////////////////////////////////////////////////
// The wire protocol
//
// Tab-separated lines inside text frames. Deliberately not JSON: both ends are
// written here, the shape is fixed, and a split on '\t' has nowhere to go
// wrong. The extension strips tabs and newlines out of titles before sending,
// so a field can never contain a separator.
//
//   extension -> mod   TAB <id> <playing> <muted> <volume> <position>
//                          <duration> <title> <artist> <site>
//                      END                      (ends a batch)
//   mod -> extension   CMD <verb> <id> <value>
//
// Verbs: toggle, play, pause, mute, seek, volume, focus. There is
// deliberately no next/prev: those are the page's own behaviour and an
// extension has no general way to drive them, so promising them here would
// mean buttons that silently do nothing.

std::vector<std::string> SplitTabs(const std::string& line) {
    std::vector<std::string> fields;
    size_t start = 0;
    for (;;) {
        size_t tab = line.find('\t', start);
        if (tab == std::string::npos) {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, tab - start));
        start = tab + 1;
    }
    return fields;
}

double ParseDouble(const std::string& text) {
    try {
        return std::stod(text);
    } catch (...) {
        return 0.0;
    }
}

long long ParseLongLong(const std::string& text) {
    try {
        return std::stoll(text);
    } catch (...) {
        return 0;
    }
}

int ParseInt(const std::string& text, int fallback = 0) {
    try {
        return std::stoi(text);
    } catch (...) {
        return fallback;
    }
}

void HandleReport(const std::string& payload) {
    std::vector<TabInfo> incoming;
    size_t start = 0;
    bool sawEnd = false;
    while (start <= payload.size()) {
        size_t newline = payload.find('\n', start);
        std::string line = payload.substr(
            start, newline == std::string::npos ? std::string::npos
                                                : newline - start);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line == "END") {
            sawEnd = true;
        } else if (line.compare(0, 4, "TAB\t") == 0) {
            auto f = SplitTabs(line);
            if (f.size() >= 10) {
                TabInfo tab;
                tab.id = ParseLongLong(f[1]);
                tab.playing = ParseInt(f[2]) != 0;
                tab.muted = ParseInt(f[3]) != 0;
                tab.volume = std::clamp(ParseInt(f[4], 100), 0, 100);
                tab.position = ParseDouble(f[5]);
                tab.duration = ParseDouble(f[6]);
                tab.title = FromUtf8(f[7].data(), f[7].size());
                tab.artist = FromUtf8(f[8].data(), f[8].size());
                tab.site = FromUtf8(f[9].data(), f[9].size());
                if (tab.id != 0) {
                    incoming.push_back(std::move(tab));
                }
            }
        }
        if (newline == std::string::npos) {
            break;
        }
        start = newline + 1;
    }

    // A batch without its END marker is a partial read, not an empty browser;
    // dropping the previous state for it would make the tab list flicker.
    if (!sawEnd) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_stateMutex);
    g_tabs = std::move(incoming);
    g_extensionConnected = true;
    g_lastReportTick = GetTickCount64();
    PublishSnapshotLocked();
}

std::string BuildCommand(const std::wstring& verb, long long tab, int value) {
    std::string out = "CMD\t";
    out += ToUtf8(verb);
    out += "\t";
    out += std::to_string(tab);
    out += "\t";
    out += std::to_string(value);
    out += "\n";
    return out;
}

////////////////////////////////////////////////////////////////////////////////
// The server

SOCKET g_listenSocket = INVALID_SOCKET;
SOCKET g_clientSocket = INVALID_SOCKET;
std::mutex g_clientMutex;
HANDLE g_stopEvent = nullptr;
HANDLE g_serverThread = nullptr;

// Queued by the UI thread (tray menu) or by another mod through the snapshot's
// command slot, and drained by the connection thread.
std::vector<std::string> g_outgoing;
std::mutex g_outgoingMutex;

void QueueCommand(const std::wstring& verb, long long tab, int value) {
    if (!g_settings.allowControl) {
        Wh_Log(L"Control is turned off; dropping '%s'", verb.c_str());
        return;
    }
    std::lock_guard<std::mutex> lock(g_outgoingMutex);
    g_outgoing.push_back(BuildCommand(verb, tab, value));
}

std::wstring GenerateToken() {
    unsigned char bytes[16] = {};
    if (!BCRYPT_SUCCESS(BCryptGenRandom(nullptr, bytes, sizeof(bytes),
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG))) {
        // Not reached in practice, and a predictable token is worse than none,
        // so refuse rather than fall back to something guessable.
        return std::wstring();
    }
    static const wchar_t kHex[] = L"0123456789abcdef";
    std::wstring out;
    out.reserve(sizeof(bytes) * 2);
    for (unsigned char b : bytes) {
        out.push_back(kHex[b >> 4]);
        out.push_back(kHex[b & 0x0F]);
    }
    return out;
}

std::wstring ConnectionUrl() {
    std::wstring url = L"ws://127.0.0.1:" + std::to_wstring(g_settings.port) +
                       L"/?token=" + g_settings.token;
    return url;
}

// Pulls the token out of the request line's query string.
std::string TokenFromRequestLine(const std::string& requestLine) {
    size_t query = requestLine.find("token=");
    if (query == std::string::npos) {
        return std::string();
    }
    size_t start = query + 6;
    size_t end = start;
    while (end < requestLine.size() && requestLine[end] != '&' &&
           requestLine[end] != ' ' && requestLine[end] != '\r') {
        end++;
    }
    return requestLine.substr(start, end - start);
}

// Constant-time compare, so a wrong token cannot be narrowed down by timing
// how long the refusal took.
bool TokenMatches(const std::string& presented) {
    const std::string expected = ToUtf8(g_settings.token);
    if (expected.empty() || presented.size() != expected.size()) {
        return false;
    }
    unsigned char diff = 0;
    for (size_t i = 0; i < expected.size(); i++) {
        diff |= (unsigned char)(expected[i] ^ presented[i]);
    }
    return diff == 0;
}

void CloseClient() {
    std::lock_guard<std::mutex> lock(g_clientMutex);
    if (g_clientSocket != INVALID_SOCKET) {
        closesocket(g_clientSocket);
        g_clientSocket = INVALID_SOCKET;
    }
}

void MarkDisconnected() {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    g_extensionConnected = false;
    g_tabs.clear();
    PublishSnapshotLocked();
}

// Reads the HTTP request, checks the token, and replies 101. Returns false
// when the connection should be dropped.
bool PerformHandshake(SOCKET sock) {
    std::string request;
    char chunk[1024];
    // Headers end at a blank line. A client that never sends one gets dropped
    // by the receive timeout set on the socket.
    while (request.find("\r\n\r\n") == std::string::npos) {
        int got = recv(sock, chunk, sizeof(chunk), 0);
        if (got <= 0) {
            return false;
        }
        request.append(chunk, (size_t)got);
        if (request.size() > 16 * 1024) {
            return false;
        }
    }

    size_t lineEnd = request.find("\r\n");
    const std::string requestLine = request.substr(0, lineEnd);

    if (!TokenMatches(TokenFromRequestLine(requestLine))) {
        Wh_Log(L"Rejected a connection with a missing or wrong token");
        const char* refusal =
            "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        SendAll(sock, refusal, strlen(refusal));
        return false;
    }

    // Header names are case-insensitive, so search a lower-cased copy and take
    // the value from the original.
    std::string lower = request;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return (char)tolower(c); });
    size_t keyAt = lower.find("sec-websocket-key:");
    if (keyAt == std::string::npos) {
        Wh_Log(L"Connection was not a WebSocket upgrade");
        return false;
    }
    size_t valueStart = keyAt + strlen("sec-websocket-key:");
    size_t valueEnd = request.find("\r\n", valueStart);
    const std::string clientKey =
        TrimAscii(request.substr(valueStart, valueEnd - valueStart));

    const std::string accept = AcceptKey(clientKey);
    if (accept.empty()) {
        return false;
    }

    std::string response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
    return SendAll(sock, response.data(), response.size());
}

void ServeClient(SOCKET sock) {
    if (!PerformHandshake(sock)) {
        return;
    }
    Wh_Log(L"Extension connected");

    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_extensionConnected = true;
        g_lastReportTick = GetTickCount64();
        PublishSnapshotLocked();
    }

    // Reads block, so the receive timeout doubles as the tick that flushes
    // queued commands and notices the stop event.
    DWORD timeout = 200;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout,
               sizeof(timeout));

    for (;;) {
        if (WaitForSingleObject(g_stopEvent, 0) == WAIT_OBJECT_0) {
            break;
        }

        // Anything another mod asked for through the snapshot's command slot.
        if (g_snapshot) {
            unsigned int seq = g_snapshot->commandSequence;
            if (seq != g_lastSeenCommandSequence) {
                g_lastSeenCommandSequence = seq;
                wchar_t verb[32] = {};
                wcsncpy_s(verb, g_snapshot->commandVerb, ARRAYSIZE(verb) - 1);
                QueueCommand(verb, g_snapshot->commandTab,
                             g_snapshot->commandValue);
            }
        }

        std::vector<std::string> pending;
        {
            std::lock_guard<std::mutex> lock(g_outgoingMutex);
            pending.swap(g_outgoing);
        }
        for (const auto& command : pending) {
            if (!SendTextFrame(sock, command)) {
                Wh_Log(L"Send failed; dropping the connection");
                return;
            }
        }

        std::string payload;
        FrameKind kind = ReadFrame(sock, &payload);
        if (kind == FrameKind::Error) {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT) {
                continue;  // the 200 ms tick, not a failure
            }
            break;
        }
        if (kind == FrameKind::Close) {
            break;
        }
        if (kind == FrameKind::Ping) {
            SendPong(sock, payload);
            continue;
        }
        if (kind == FrameKind::Text) {
            HandleReport(payload);
        }
    }

    Wh_Log(L"Extension disconnected");
}

DWORD WINAPI ServerThreadProc(LPVOID) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Wh_Log(L"WSAStartup failed");
        return 1;
    }

    g_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenSocket == INVALID_SOCKET) {
        Wh_Log(L"socket() failed (%d)", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    // Loopback only. Never INADDR_ANY: nothing off this machine has any
    // business reaching this.
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons((unsigned short)g_settings.port);

    if (bind(g_listenSocket, (sockaddr*)&address, sizeof(address)) ==
        SOCKET_ERROR) {
        Wh_Log(L"Could not bind 127.0.0.1:%d (%d) - is the port already in use?",
               g_settings.port, WSAGetLastError());
        closesocket(g_listenSocket);
        g_listenSocket = INVALID_SOCKET;
        WSACleanup();
        return 1;
    }
    if (listen(g_listenSocket, 1) == SOCKET_ERROR) {
        Wh_Log(L"listen() failed (%d)", WSAGetLastError());
        closesocket(g_listenSocket);
        g_listenSocket = INVALID_SOCKET;
        WSACleanup();
        return 1;
    }

    Wh_Log(L"Listening on 127.0.0.1:%d", g_settings.port);

    while (WaitForSingleObject(g_stopEvent, 0) != WAIT_OBJECT_0) {
        // select rather than a blocking accept, so the stop event is noticed
        // without having to connect to the port to break it.
        fd_set readable;
        FD_ZERO(&readable);
        FD_SET(g_listenSocket, &readable);
        timeval wait = {0, 200 * 1000};
        int ready = select(0, &readable, nullptr, nullptr, &wait);
        if (ready == SOCKET_ERROR) {
            break;
        }
        if (ready == 0) {
            continue;
        }

        SOCKET client = accept(g_listenSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            continue;
        }

        // The handshake has to finish promptly; a connection that opens and
        // says nothing would otherwise hold the single slot forever.
        DWORD handshakeTimeout = 5000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO,
                   (const char*)&handshakeTimeout, sizeof(handshakeTimeout));

        {
            std::lock_guard<std::mutex> lock(g_clientMutex);
            g_clientSocket = client;
        }
        ServeClient(client);
        CloseClient();
        MarkDisconnected();
    }

    if (g_listenSocket != INVALID_SOCKET) {
        closesocket(g_listenSocket);
        g_listenSocket = INVALID_SOCKET;
    }
    WSACleanup();
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Tray icon
//
// The point of the icon is that this mod is otherwise completely invisible:
// without it, a wrong port or an unpasted token looks exactly like a mod that
// is not running.

constexpr UINT WM_APP_TRAY = WM_APP + 1;
constexpr UINT WM_APP_QUIT = WM_APP + 2;
constexpr UINT WM_APP_SETTINGS = WM_APP + 3;
constexpr UINT kTrayIconId = 1;
constexpr UINT_PTR kTickTimerId = 1;
constexpr UINT kTickIntervalMs = 1000;
constexpr UINT kCmdCopyUrl = 100;
constexpr UINT kCmdTabBase = 200;

HINSTANCE g_hInst = nullptr;
HWND g_trayWnd = nullptr;
HICON g_trayIcon = nullptr;
bool g_trayIconAdded = false;
UINT g_taskbarCreatedMessage = 0;
HANDLE g_uiThread = nullptr;
DWORD g_uiThreadId = 0;
HANDLE g_readyEvent = nullptr;
ULONG_PTR g_gdiplusToken = 0;
std::vector<TabInfo> g_menuTabs;

// A report older than this means the extension is attached but has gone quiet,
// which is worth showing differently from "never connected".
constexpr unsigned long long kStaleAfterMs = 10000;

bool ExtensionLive() {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (!g_extensionConnected) {
        return false;
    }
    return GetTickCount64() - g_lastReportTick < kStaleAfterMs;
}

HICON CreateStateIcon(int size, bool connected, bool anyPlaying) {
    HDC screenDC = GetDC(nullptr);
    if (!screenDC) {
        return nullptr;
    }
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = size;
    info.bmiHeader.biHeight = -size;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP colorBitmap = CreateDIBSection(screenDC, &info, DIB_RGB_COLORS,
                                           &bits, nullptr, 0);
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

        // Solid once the extension is reporting, hollow while it is not;
        // green when something is playing.
        Gdiplus::Color color = connected
            ? (anyPlaying ? Gdiplus::Color(255, 52, 199, 89)
                          : Gdiplus::Color(255, 240, 240, 240))
            : Gdiplus::Color(255, 150, 150, 150);
        float inset = size / 8.0f;
        Gdiplus::RectF disc(inset, inset, size - inset * 2, size - inset * 2);
        if (connected) {
            Gdiplus::SolidBrush brush(color);
            graphics.FillEllipse(&brush, disc);
        } else {
            float thickness = size / 8.0f;
            if (thickness < 1.5f) {
                thickness = 1.5f;
            }
            Gdiplus::Pen pen(color, thickness);
            Gdiplus::RectF ring(disc.X + thickness / 2, disc.Y + thickness / 2,
                                disc.Width - thickness, disc.Height - thickness);
            graphics.DrawEllipse(&pen, ring);
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

std::wstring DescribeState() {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (!g_extensionConnected) {
        return L"Waiting for the extension on port " +
               std::to_wstring(g_settings.port);
    }
    if (GetTickCount64() - g_lastReportTick >= kStaleAfterMs) {
        return L"Connected, but no report for a while";
    }
    if (g_tabs.empty()) {
        return L"Connected. No tab is playing media.";
    }
    std::wstring text = std::to_wstring(g_tabs.size()) +
                        (g_tabs.size() == 1 ? L" tab" : L" tabs");
    for (const auto& tab : g_tabs) {
        text += L"\r\n";
        text += tab.playing ? L"> " : L"|| ";
        text += tab.title.empty() ? tab.site : tab.title;
    }
    return text;
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

    bool connected = ExtensionLive();
    bool anyPlaying = false;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        for (const auto& tab : g_tabs) {
            if (tab.playing) {
                anyPlaying = true;
                break;
            }
        }
    }

    HICON icon = CreateStateIcon(16, connected, anyPlaying);
    if (!icon) {
        return;
    }

    std::wstring tip = L"Browser Media Bridge\r\n" + DescribeState();
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

void CopyTextToClipboard(const std::wstring& text) {
    if (!OpenClipboard(g_trayWnd)) {
        return;
    }
    EmptyClipboard();
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (handle) {
        void* target = GlobalLock(handle);
        if (target) {
            memcpy(target, text.c_str(), bytes);
            GlobalUnlock(handle);
            SetClipboardData(CF_UNICODETEXT, handle);
        } else {
            GlobalFree(handle);
        }
    }
    CloseClipboard();
}

void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    AppendMenuW(menu, MF_STRING, kCmdCopyUrl, L"Copy connection URL");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_menuTabs = g_tabs;
    }

    if (!ExtensionLive()) {
        AppendMenuW(menu, MF_STRING | MF_DISABLED, 0,
                    L"Extension not connected");
    } else if (g_menuTabs.empty()) {
        AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, L"No tab is playing");
    } else {
        for (size_t i = 0; i < g_menuTabs.size() && i < 32; i++) {
            const TabInfo& tab = g_menuTabs[i];
            std::wstring label = tab.playing ? L"Pause  " : L"Play  ";
            std::wstring name = tab.title.empty() ? tab.site : tab.title;
            if (name.size() > 48) {
                name.resize(45);
                name += L"...";
            }
            label += name;
            if (!tab.site.empty() && !tab.title.empty()) {
                label += L"  (" + tab.site + L")";
            }
            AppendMenuW(menu,
                        MF_STRING | (g_settings.allowControl ? 0 : MF_DISABLED),
                        kCmdTabBase + (UINT)i, label.c_str());
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
        case WM_COMMAND: {
            UINT id = LOWORD(wParam);
            if (id == kCmdCopyUrl) {
                CopyTextToClipboard(ConnectionUrl());
                return 0;
            }
            if (id >= kCmdTabBase && id < kCmdTabBase + g_menuTabs.size()) {
                const TabInfo& tab = g_menuTabs[id - kCmdTabBase];
                QueueCommand(L"toggle", tab.id, 0);
                return 0;
            }
            return 0;
        }
        case WM_TIMER:
            if (wParam == kTickTimerId) {
                UpdateTrayIcon();
                return 0;
            }
            break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// The mod's lifecycle

void LoadSettings() {
    g_settings.port = Wh_GetIntSetting(L"port");
    if (g_settings.port < 1024 || g_settings.port > 65535) {
        g_settings.port = 49872;
    }

    PCWSTR token = Wh_GetStringSetting(L"token");
    g_settings.token = token ? token : L"";
    Wh_FreeStringSetting(token);
    // Trim, so a stray space pasted in with the token does not silently stop
    // every connection from matching.
    while (!g_settings.token.empty() &&
           (g_settings.token.front() == L' ' || g_settings.token.front() == L'\t')) {
        g_settings.token.erase(0, 1);
    }
    while (!g_settings.token.empty() &&
           (g_settings.token.back() == L' ' || g_settings.token.back() == L'\t')) {
        g_settings.token.pop_back();
    }
    if (g_settings.token.empty()) {
        g_settings.token = GenerateToken();
        if (!g_settings.token.empty()) {
            // Written back so it survives restarts and the user can read it
            // out of the mod's settings as well as the tray menu.
            Wh_SetStringValue(L"token", g_settings.token.c_str());
            Wh_Log(L"Generated a connection token");
        } else {
            Wh_Log(L"Could not generate a token; refusing all connections");
        }
    }

    g_settings.allowControl = Wh_GetIntSetting(L"allowControl") != 0;
    g_settings.showTrayIcon = Wh_GetIntSetting(L"showTrayIcon") != 0;
}

DWORD WINAPI UiThreadProc(LPVOID) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

    WNDCLASSEXW windowClass = {sizeof(windowClass)};
    windowClass.lpfnWndProc = TrayWndProc;
    windowClass.hInstance = g_hInst;
    windowClass.lpszClassName = L"WH_BrowserMediaBridge";
    RegisterClassExW(&windowClass);

    // Not message-only: the tray menu has to be able to take the foreground.
    g_trayWnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"WH_BrowserMediaBridge", L"",
                                WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, g_hInst,
                                nullptr);
    if (!g_trayWnd) {
        Wh_Log(L"could not create the tray window");
        SetEvent(g_readyEvent);
        return 1;
    }

    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    UpdateTrayIcon();
    SetTimer(g_trayWnd, kTickTimerId, kTickIntervalMs, nullptr);
    SetEvent(g_readyEvent);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.hwnd == nullptr && msg.message == WM_APP_QUIT) {
            PostQuitMessage(0);
            continue;
        }
        if (msg.hwnd == nullptr && msg.message == WM_APP_SETTINGS) {
            // The port may have moved, so the listener is restarted rather
            // than left on the old one.
            LoadSettings();
            UpdateTrayIcon();
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(g_trayWnd, kTickTimerId);
    RemoveTrayIcon();
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

    if (!CreateSnapshot()) {
        return FALSE;
    }

    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_stopEvent || !g_readyEvent) {
        return FALSE;
    }

    g_uiThread = CreateThread(nullptr, 0, UiThreadProc, nullptr, 0,
                              &g_uiThreadId);
    if (!g_uiThread) {
        return FALSE;
    }
    WaitForSingleObject(g_readyEvent, 5000);

    g_serverThread = CreateThread(nullptr, 0, ServerThreadProc, nullptr, 0,
                                  nullptr);
    if (!g_serverThread) {
        Wh_Log(L"could not start the server thread");
        return FALSE;
    }
    return TRUE;
}

void WhTool_ModSettingsChanged() {
    if (g_uiThreadId) {
        PostThreadMessageW(g_uiThreadId, WM_APP_SETTINGS, 0, 0);
    }
}

void WhTool_ModUninit() {
    if (g_stopEvent) {
        SetEvent(g_stopEvent);
    }
    // Closing both sockets is what unblocks the server thread's select and any
    // in-progress recv; waiting on the timeouts alone would take longer.
    CloseClient();
    if (g_listenSocket != INVALID_SOCKET) {
        closesocket(g_listenSocket);
        g_listenSocket = INVALID_SOCKET;
    }
    if (g_serverThread) {
        if (WaitForSingleObject(g_serverThread, 5000) != WAIT_OBJECT_0) {
            Wh_Log(L"server thread did not exit in time");
        }
        CloseHandle(g_serverThread);
        g_serverThread = nullptr;
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

    DestroySnapshot();

    if (g_stopEvent) {
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
    }
    if (g_readyEvent) {
        CloseHandle(g_readyEvent);
        g_readyEvent = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////
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
