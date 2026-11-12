// Runs in the page's own world.
//
// navigator.mediaSession.metadata is what the page hands to Windows, so it is
// by far the best title and artist available -- better than document.title,
// which on a music site is usually the site name. A content script in the
// isolated world shares the DOM but not reliably this object, so this tiny
// script lives in the MAIN world and relays it across. If the isolated world
// turns out to see mediaSession fine on your browser, this is harmless
// duplication rather than a problem.
(function () {
  function snapshot() {
    try {
      var m = navigator.mediaSession && navigator.mediaSession.metadata;
      if (!m) return null;
      return { title: m.title || "", artist: m.artist || "", album: m.album || "" };
    } catch (e) {
      return null;
    }
  }

  var lastSent = "";
  function tick() {
    var data = snapshot();
    var key = data ? data.title + "\u0000" + data.artist : "";
    if (key !== lastSent) {
      lastSent = key;
      // Same-window postMessage; content.js filters on the marker.
      window.postMessage({ __bmb_metadata: true, data: data }, "*");
    }
  }

  setInterval(tick, 1000);
  tick();
})();
