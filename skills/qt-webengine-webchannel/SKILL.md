---
name: qt-webengine-webchannel
description: Use when embedding QWebEngineView and wiring QWebChannel/HostApi bootstrap between C++ and JS.
---

# Qt WebEngine + WebChannel Bridge

## When to use
- Embedding web UI in Qt (QWebEngineView/QWebEnginePage).
- Creating or updating the HostApi bridge between C++ and JS.
- Injecting bootstrap JS from C++.

## Workflow
1) **Create the web view stack**
   - Instantiate QWebEngineProfile, QWebEnginePage, and QWebEngineView.
   - Set security-related settings (local/remote access) based on the chosen root.
2) **Set up QWebChannel**
   - Create QWebChannel, register HostBridge (or generated objects).
   - Call `page->setWebChannel(channel)`.
3) **Inject HostApi**
   - On `loadFinished(ok == true)`, run JS that:
     - loads `qrc:///qtwebchannel/qwebchannel.js` if missing
     - constructs `window.HostApi`
     - emits `HostApiReady`
4) **Logging**
   - Override `javaScriptConsoleMessage` to forward console logs to Qt logs.

## Guardrails
- Only inject HostApi from C++; the web page should wait for `HostApiReady`.
- Ensure `qt.webChannelTransport` exists before creating QWebChannel in JS.
- If using QRC assets, disable local file access where possible.
- Re-inject HostApi on reload (loadFinished is the safe place).

## Pitfalls
- Injecting before page load can race with `qt.webChannelTransport`.
- Loading `qwebchannel.js` from a non-existent URL silently breaks HostApi.
- Leaving `LocalContentCanAccessRemoteUrls` enabled can widen attack surface.

## Minimal checklist
- [ ] WebChannel registered
- [ ] HostApi injected from C++
- [ ] HostApiReady dispatched
- [ ] Console logging enabled

## Execution permissions
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
