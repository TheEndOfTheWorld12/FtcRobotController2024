// Runs once per frame, in the isolated world.
//
// Watches this frame's <audio>/<video> and reports the interesting one to the
// background worker. Media is very often inside an iframe (an embedded
// player), which is why this runs in all frames; the background picks one
// report per tab.
(function () {
  var metadata = null;

  window.addEventListener("message", function (event) {
    if (event.source !== window) return;
    var payload = event.data;
    if (!payload || payload.__bmb_metadata !== true) return;
    metadata = payload.data;
  });

  // The element worth reporting: something actually playing, else the longest
  // one, so a page full of muted preview clips does not win over the player.
  function pickElement() {
    var all = Array.prototype.slice.call(
      document.querySelectorAll("audio, video")
    );
    var playing = all.filter(function (el) {
      return !el.paused && !el.ended && el.readyState > 2;
    });
    var pool = playing.length ? playing : all;
    if (!pool.length) return null;
    pool.sort(function (a, b) {
      return (b.duration || 0) - (a.duration || 0);
    });
    return pool[0];
  }

  // Tabs and newlines are the wire protocol's separators, so no field may
  // contain one.
  function clean(text) {
    return String(text == null ? "" : text)
      .replace(/[\t\r\n]+/g, " ")
      .trim()
      .slice(0, 120);
  }

  function describe() {
    var el = pickElement();
    if (!el) return null;
    var title = (metadata && metadata.title) || document.title || "";
    var artist = (metadata && metadata.artist) || "";
    return {
      playing: !el.paused && !el.ended && el.readyState > 2,
      muted: !!el.muted,
      volume: Math.round((el.volume == null ? 1 : el.volume) * 100),
      position: isFinite(el.currentTime) ? el.currentTime : 0,
      duration: isFinite(el.duration) ? el.duration : 0,
      title: clean(title),
      artist: clean(artist),
      site: clean(location.hostname)
    };
  }

  var lastKey = "";
  function report(force) {
    var state = describe();
    // A frame with no media at all reports once, so the background can drop
    // any stale entry for it.
    var key = state
      ? [state.playing, state.muted, state.volume, Math.floor(state.position),
         Math.floor(state.duration), state.title, state.artist].join("|")
      : "none";
    if (!force && key === lastKey) return;
    lastKey = key;
    try {
      chrome.runtime.sendMessage({ kind: "bmb-state", state: state });
    } catch (e) {
      // The worker was asleep or the extension reloaded; the next tick retries.
    }
  }

  // Commands arrive from the mod via the background worker. Only things a
  // media element can actually do: next and previous track are the page's own
  // business and there is no general way to drive them from outside, so they
  // are not handled here rather than silently doing nothing.
  chrome.runtime.onMessage.addListener(function (message, sender, respond) {
    if (!message || message.kind !== "bmb-command") return;
    var el = pickElement();
    if (!el) {
      respond && respond({ ok: false });
      return;
    }
    try {
      switch (message.verb) {
        case "toggle": el.paused ? el.play() : el.pause(); break;
        case "play": el.play(); break;
        case "pause": el.pause(); break;
        case "mute": el.muted = !el.muted; break;
        case "seek":
          if (isFinite(el.duration) && el.duration > 0) {
            el.currentTime = Math.max(
              0, Math.min(el.duration, message.value || 0));
          }
          break;
        case "volume":
          el.volume = Math.max(0, Math.min(100, message.value || 0)) / 100;
          break;
      }
    } catch (e) {
      // Autoplay policy can refuse play(); nothing useful to do about it here.
    }
    report(true);
    respond && respond({ ok: true });
  });

  document.addEventListener("play", function () { report(false); }, true);
  document.addEventListener("pause", function () { report(false); }, true);
  document.addEventListener("volumechange", function () { report(false); }, true);
  setInterval(function () { report(false); }, 1000);
  report(true);
})();
