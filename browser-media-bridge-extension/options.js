const field = document.getElementById("url");
const status = document.getElementById("status");

chrome.storage.local.get("url").then((stored) => {
  field.value = (stored && stored.url) || "";
});

document.getElementById("save").addEventListener("click", async () => {
  const url = field.value.trim();
  if (url && !/^ws:\/\/127\.0\.0\.1:\d+\/\?token=.+$/.test(url)) {
    status.textContent = "That does not look like the tray icon's URL.";
    return;
  }
  await chrome.storage.local.set({ url });
  status.textContent = url ? "Saved. Connecting..." : "Cleared.";
});
