// The single WebSocket to the Windhawk mod, and the per-tab state behind it.
//
// MV3 service workers are killed when idle, which would quietly end the whole
// feature. WebSocket activity resets that idle timer, so the live socket is
// what keeps this alive; the heartbeat below guarantees there is always
// activity well inside the window even when nothing is playing.

const RECONNECT_MIN_MS = 1000;
const RECONNECT_MAX_MS = 30000;
const HEARTBEAT_MS = 20000;

let socket = null;
let reconnectDelay = RECONNECT_MIN_MS;
let reconnectTimer = null;
let heartbeatTimer = null;

// tabId -> { frames: Map(frameId -> state), updated: number }
const tabs = new Map();

async function connectionUrl() {
  const stored = await chrome.storage.local.get("url");
  return (stored && stored.url) || "";
}

function clearTimers() {
  if (heartbeatTimer) { clearInterval(heartbeatTimer); heartbeatTimer = null; }
  if (reconnectTimer) { clearTimeout(reconnectTimer); reconnectTimer = null; }
}

function scheduleReconnect() {
  if (reconnectTimer) return;
  reconnectTimer = setTimeout(() => {
    reconnectTimer = null;
    connect();
  }, reconnectDelay);
  // Backing off matters: the mod may simply be turned off, and a tight retry
  // loop would keep the worker awake forever for nothing.
  reconnectDelay = Math.min(reconnectDelay * 2, RECONNECT_MAX_MS);
}

async function connect() {
  if (socket && (socket.readyState === WebSocket.OPEN ||
                 socket.readyState === WebSocket.CONNECTING)) {
    return;
  }
  const url = await connectionUrl();
  if (!url) return;  // nothing configured yet

  try {
    socket = new WebSocket(url);
  } catch (e) {
    scheduleReconnect();
    return;
  }

  socket.onopen = () => {
    reconnectDelay = RECONNECT_MIN_MS;
    sendReport();
    clearTimers();
    heartbeatTimer = setInterval(sendReport, HEARTBEAT_MS);
  };

  socket.onmessage = (event) => {
    const lines = String(event.data).split("\n");
    for (const line of lines) {
      const f = line.split("\t");
      if (f[0] !== "CMD") continue;
      const verb = f[1];
      const tabId = parseInt(f[2], 10);
      const value = parseFloat(f[3]);
      if (!isFinite(tabId)) continue;
      if (verb === "focus") {
        chrome.tabs.update(tabId, { active: true }).catch(() => {});
        chrome.tabs.get(tabId)
          .then((t) => chrome.windows.update(t.windowId, { focused: true }))
          .catch(() => {});
        continue;
      }
      chrome.tabs.sendMessage(
        tabId, { kind: "bmb-command", verb, value }
      ).catch(() => {});
    }
  };

  socket.onclose = () => { socket = null; clearTimers(); scheduleReconnect(); };
  socket.onerror = () => { /* onclose follows */ };
}

// One report line per tab. Of a tab's frames, the playing one wins, then the
// longest -- an embedded player should beat the page it sits in.
function bestFrameState(frames) {
  let best = null;
  for (const state of frames.values()) {
    if (!state) continue;
    if (!best) { best = state; continue; }
    if (state.playing !== best.playing) {
      if (state.playing) best = state;
      continue;
    }
    if ((state.duration || 0) > (best.duration || 0)) best = state;
  }
  return best;
}

function sendReport() {
  if (!socket || socket.readyState !== WebSocket.OPEN) return;
  let text = "";
  for (const [tabId, entry] of tabs) {
    const state = bestFrameState(entry.frames);
    if (!state) continue;
    text += [
      "TAB", tabId,
      state.playing ? 1 : 0,
      state.muted ? 1 : 0,
      state.volume,
      state.position.toFixed(1),
      state.duration.toFixed(1),
      state.title, state.artist, state.site
    ].join("\t") + "\n";
  }
  // The mod treats a batch without END as a partial read and keeps its
  // previous state, so this marker is what makes an empty report mean
  // "nothing is playing" rather than "the message got cut".
  text += "END\n";
  try {
    socket.send(text);
  } catch (e) {
    socket = null;
    scheduleReconnect();
  }
}

chrome.runtime.onMessage.addListener((message, sender) => {
  if (!message || message.kind !== "bmb-state") return;
  if (!sender.tab) return;
  const tabId = sender.tab.id;
  const frameId = sender.frameId || 0;

  if (!message.state) {
    const entry = tabs.get(tabId);
    if (entry) {
      entry.frames.delete(frameId);
      if (entry.frames.size === 0) tabs.delete(tabId);
    }
  } else {
    let entry = tabs.get(tabId);
    if (!entry) { entry = { frames: new Map() }; tabs.set(tabId, entry); }
    entry.frames.set(frameId, message.state);
  }
  sendReport();
});

chrome.tabs.onRemoved.addListener((tabId) => {
  if (tabs.delete(tabId)) sendReport();
});

// A service worker can be started fresh at any time, so connecting is wired to
// every entry point rather than to one of them.
chrome.runtime.onStartup.addListener(connect);
chrome.runtime.onInstalled.addListener(connect);
chrome.storage.onChanged.addListener((changes) => {
  if (!changes.url) return;
  // A new URL means a new token or port; drop the old socket for it.
  if (socket) { try { socket.close(); } catch (e) {} socket = null; }
  reconnectDelay = RECONNECT_MIN_MS;
  connect();
});
connect();
