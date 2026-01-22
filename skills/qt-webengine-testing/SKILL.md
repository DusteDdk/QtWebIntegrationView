---
name: qt-webengine-testing
description: Use when writing headless Qt WebEngine tests for HostApi flows.
---

# Qt WebEngine Testing

## When to use
- Writing integration tests for HostApi behavior.
- Running WebEngine in CI/headless environments.

## Workflow
1) **Headless flags**
   - Set `QT_QPA_PLATFORM=offscreen`.
   - Set `QTWEBENGINE_*` flags (no-sandbox, headless).
2) **Load coordination**
   - Wait for `QWebEngineView::loadFinished` via QSignalSpy.
3) **JS execution**
   - Use `runJavaScript` with an event loop to get synchronous results.
4) **HostApi checks**
   - Poll for `window.HostApi` or wait for `HostApiReady`.

## Guardrails
- Always set `Qt::AA_ShareOpenGLContexts` before QApplication.
- Avoid flakey waits; use timers + polling with a cap.

## Execution permissions
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
