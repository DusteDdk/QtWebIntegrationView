# AGENTS.md

## Purpose
This repo is a Qt6 + WebEngine demo of a `WebHost` QWidget that embeds a web UI and bridges it to C++ via Qt WebChannel. The long-term goal is to host Angular-built panels/dialogs inside a Qt app and communicate strictly through `window.HostApi`.

## High-level layout
- `QtWebIntegrationView/WebHost/`: Static library owning the embedded web view and the C++ <-> JS bridge.
- `QtWebIntegrationView/app/`: Demo Qt application that hosts `WebHost` and wires UI actions.
- `QtWebIntegrationView/web/`: Static web assets (plain HTML + JS demo).
- `QtWebIntegrationView/web-angular/`: Sample Angular app that builds into `web/`.
- `QtWebIntegrationView/hostapi/`: Opt-in HostApi classes + version/event definitions.
- `QtWebIntegrationView/tools/HostApiGenerator/`: Qt-based code generator (C++ glue + TS/Angular).
- `QtWebIntegrationView/build/generated/hostapi/`: Generated glue + schema + TS artifacts.
- `QtWebIntegrationView/tests/`: QtTest-based integration tests for the HostApi JS plumbing.
- `QtWebIntegrationView/scripts/install_deps.sh`: Ubuntu packages for Qt6 + build deps.
- `QtWebIntegrationView/build/`: Generated build output (bin + copied web assets).

## WebHost lifecycle (C++ -> JS)
1) `WebHost` ctor registers `QJsonValue` meta type and calls `initialize()`.
2) `initialize()` sets `m_validEventTypes` (`actionOne`, `actionTwo`), resolves the web root, creates:
   - `QWebEngineProfile`, `QWebEnginePage`, `QWebEngineView`
   - `WebRootInterceptor` (blocks local file requests outside the root)
   - `QWebChannel` with `HostBridge` registered as `HostBridge`
3) `loadFinished(ok == true)`:
   - `applyWindowBackground()` updates `--host-window-color` and body background
   - `injectHostApiBootstrap()` loads `qrc:///qtwebchannel/qwebchannel.js` if needed, constructs `window.HostApi`, and dispatches `HostApiReady`
4) `loadRoot()` chooses `file://.../index.html` (dir mode) or `qrc:/web/index.html` (qrc mode).

## HostBridge / HostApi contract (current)
- `HostBridge` (private class in `WebHost.cpp`)
  - `Q_INVOKABLE`:
    - `sendData(QVariant)` -> emits `signalSendData(QJsonValue)`
    - `setOutput(QString)` -> emits `signalSetOutput(QString)`
    - `requestInput()` -> emits `signalGetInput(uuid)` and returns the uuid
  - Signal: `inputProvided(uuid, input)` used by JS to resolve `getInput()`
  - Property: `validEventTypes` used by JS to validate event names
  - Property: `hostApiVersion` + `hostApiSchema` (generated metadata)
- JS `window.HostApi` (injected from C++ only):
  - `sendData(payload)`
  - `setOutput(text)`
  - `getInput(): Promise<string>` (resolves on `inputProvided`)
  - `addEventListener(eventType, cb)` / `removeEventListener(eventType, cb)` (throws if eventType not allowed)
  - `validEventTypes` (array)
  - `__dispatchEvent(eventType, payload)` (used by C++ `slotTriggerEvent`)
  - `version`, `schema`
  - Generated member objects (e.g., `example`) with `registerEventHandler`/`removeEventHandler`
  - `HostApiReady` event fires when ready

## Root selection & loading
- `setRootDir(path)`: resolves to `path` if it exists, otherwise falls back to `applicationDirPath()/web`.
- `setRootQrc()`: loads `qrc:/web/index.html` (backed by `web/web.qrc`).
- `WebRootInterceptor` blocks local `file://` requests that resolve outside the configured root.

## Security / settings
- `LocalContentCanAccessFileUrls = true`, `LocalContentCanAccessRemoteUrls = false`.
- `webhost://` scheme is registered but not used when loading.

## Build & run (Ubuntu example)
- Install deps: `QtWebIntegrationView/scripts/install_deps.sh`
- Configure/build:
  - `cmake -S QtWebIntegrationView -B QtWebIntegrationView/build -G Ninja`
  - `cmake --build QtWebIntegrationView/build`
- Run app: `QtWebIntegrationView/build/bin/QtWebIntegrationView`
- `copy_web` CMake target copies `web/` to `build/bin/web`.
 - `WEBHOST_USE_QRC=ON` embeds assets and defaults to `setRootQrc()`.
 - `HostApiCodegen` generates C++/TS artifacts into `build/generated/hostapi`.

## Tests
- Build and run:
  - `cmake --build QtWebIntegrationView/build --target WebHostTests`
  - `ctest --test-dir QtWebIntegrationView/build`
- Headless flags set in `tests/test_webhost.cpp` (`QTWEBENGINE_*`, offscreen platform).
- Current tests cover:
  - `addEventListener` / `removeEventListener`
  - invalid event type error handling

## Debug tips
- JS console output is forwarded via `WebHostPage::javaScriptConsoleMessage()`.
- Load start/finish and blocked file requests are logged from `WebHost`.

## Known gaps vs. stated goals
- `webhost://` scheme is registered but unused in load paths.
- Angular sample app lives in `QtWebIntegrationView/web-angular` (workflow in `web-angular/ANGULAR_WORKFLOW.md`).
