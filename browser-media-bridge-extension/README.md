# Browser Media Bridge — extension

The browser half of the `browser-media-bridge` Windhawk mod. It reports what
each tab is playing over a loopback WebSocket, and applies commands coming
back the other way.

Written for **Chrome and Edge** (identical — both are Chromium, Manifest V3).
See *Firefox* at the bottom.

## Install

Chrome will not permanently install an extension from outside its Store, so
this is loaded unpacked. That is normal for a personal extension; the cost is
a "disable developer mode extensions" nag on some startups.

1. Turn the `browser-media-bridge` mod on in Windhawk.
2. Open `chrome://extensions` (or `edge://extensions`).
3. Turn on **Developer mode**.
4. Click **Load unpacked** and pick this folder.

That is the whole of it — nothing to copy, nothing to paste. The extension
connects to the mod's default port by itself, and the mod keeps web pages out
by checking the handshake's `Origin` rather than by a shared token.

To check it worked: **Details** → **Extension options**. The card at the top
says whether the connection is up and how many tabs are reporting.

Keep the folder where it is — Chrome loads an unpacked extension from its path
every startup, so moving or deleting it uninstalls it.

## If it says not connected

Either the mod is off, or something else already holds port 49872. Set the
same port in both places: the mod's **Port** setting, and the box on the
options page. A token is only needed if you chose to set one on the mod's
side.

## What gets reported

Per tab: whether it is playing, muted, volume, position, duration, title,
artist and hostname. Title and artist come from
`navigator.mediaSession.metadata` when the page sets it — the same thing the
page hands to Windows — and fall back to the tab title.

Media is very often inside an iframe, so the content script runs in all
frames. The background worker sends one line per *tab*: of that tab's frames,
the playing one wins, then the longest, so an embedded player beats the page
around it.

## What it can and cannot control

Supported: `toggle`, `play`, `pause`, `mute`, `seek`, `volume` (all driven on
the tab's media element) and `focus` (switches to the tab).

**Not** supported: next and previous track. Those are the page's own
behaviour, reachable only through the media-key handling that the browser
keeps to itself; there is no general way to drive them from an extension. They
are not wired to anything rather than being buttons that silently do nothing.

`play()` can also be refused by the browser's autoplay policy if the tab has
had no user interaction. Nothing to be done about that from here.

## Privacy and security

- The only thing it talks to is `127.0.0.1` on the bridge's port. Nothing is
  sent anywhere else, and there is no telemetry.
- `<all_urls>` is required to see media on any site. The content script reads
  media element state, the page title and `mediaSession` metadata — not page
  content, not form fields, not cookies.
- What stops a random web page connecting to the mod's port is the `Origin`
  header, which the browser sets and page script cannot touch. The mod accepts
  an extension scheme or no `Origin` at all, and refuses everything else.

## Firefox

Firefox needs three changes:

- `background`: use `"scripts": ["background.js"]` instead of
  `"service_worker"`.
- The `"world": "MAIN"` content script is not supported; drop that entry.
  `page.js` is only an optimisation for titles, and `content.js` already falls
  back to the tab title without it.
- `browser_specific_settings.gecko.id` must be set to load it unpacked
  persistently; otherwise use `about:debugging` → **Load Temporary Add-on**,
  which lasts until the browser restarts.

The `chrome.*` calls all work in Firefox as-is.
