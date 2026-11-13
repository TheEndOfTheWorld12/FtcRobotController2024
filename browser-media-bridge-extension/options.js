const dot = document.getElementById("dot");
const state = document.getElementById("state");
const detail = document.getElementById("detail");
const portField = document.getElementById("port");
const tokenField = document.getElementById("token");
const saved = document.getElementById("saved");

chrome.storage.local.get(["port", "token"]).then((stored) => {
  if (stored && stored.port) portField.value = stored.port;
  if (stored && stored.token) tokenField.value = stored.token;
});

function refresh() {
  chrome.runtime.sendMessage({ kind: "bmb-status" }).then((status) => {
    if (!status) throw new Error("no reply");
    dot.className = "dot " + (status.open ? "up" : "down");
    state.textContent = status.open ? "Connected to the bridge"
                                    : "Not connected";
    const where = status.url.replace(/\?token=.*$/, "");
    detail.textContent = status.open
      ? status.tabCount + (status.tabCount === 1 ? " tab" : " tabs") +
        " reporting, " + status.playing + " playing  ·  " + where
      : "Nothing is listening on " + where;
  }).catch(() => {
    // The worker was asleep; sending the message wakes it, so the next tick
    // usually succeeds.
    dot.className = "dot down";
    state.textContent = "Starting…";
    detail.textContent = "";
  });
}

document.getElementById("save").addEventListener("click", async () => {
  const port = parseInt(portField.value, 10);
  if (portField.value && (!isFinite(port) || port < 1024 || port > 65535)) {
    saved.textContent = "Port must be between 1024 and 65535.";
    return;
  }
  await chrome.storage.local.set({
    port: portField.value ? port : null,
    token: tokenField.value.trim()
  });
  saved.textContent = "Saved. Reconnecting…";
  setTimeout(refresh, 600);
});

refresh();
setInterval(refresh, 2000);
