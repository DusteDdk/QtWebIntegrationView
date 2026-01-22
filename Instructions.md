# Role
You are an senior Software Engineer with special interest in C++, Qt and JavaScript.

# General direction
Building a generic framework for integration of UI components written in Angular (HTML/CSS/TypeScript) and an existing QT application to leverage the strong existing foundation of the QT application while making development of new UI featuers (entire panels,  windows and dialogues) to be developed by web developers.

# Philosophy
The Angular components are hosted inside a QWidget subclass called "WebHost" which acts as a generic bridge between "javascript land" and "c++ land".
The contents of the webview are going to be compiled into the Qt application and served directly from the application, not through http.
Interactions between the angular app and the host qt application will take place strictly through a DOM object "windows.HostApi" which exposes all the methods that allow bidirectional communication, including RPC, sync and async.

# Current state
- The application works and demonstrates a fairly familiar API on both the C++ and JavaScript side. "It works" but is not yet feature-complete.
- The webview shouldn't contain any code related to bootstrapping the dom object; this should be provided from the C++ side by the host view.
- The demo now injects HostApi from C++ and the web page waits for `HostApiReady`.
- Runtime root selection exists: `setRootDir(QString)` for local dev and `setRootQrc()` for packaged assets.
- HostApi versioning + schema metadata are exposed on the bridge and injected into `window.HostApi`.
- HostApi codegen scaffolding exists (C++ glue, schema JSON, `hostapi.d.ts`, Angular services).
- A sample Angular app lives in `QtWebIntegrationView/web-angular` and builds into `web/`.

# Steps to a finished product
1) **Runtime asset strategy**
   - Keep `setRootDir(QString)` for local Angular rebuilds without recompiling C++.
   - Add Qt resources (`.qrc`) for packaged builds and use `setRootQrc()` in production.
   - Optionally switch to `webhost://` scheme for tighter control (single source of truth for asset loading).
2) **HostApi bootstrap in C++**
   - Inject HostApi after web load (or via preload) from C++.
   - Web assets must not define HostApi; they only wait for `HostApiReady`.
3) **HostApi contract ownership**
   - Define which C++ classes are exposed to HostApi and which are not.
   - Use explicit markers (e.g., Q_CLASSINFO or macro) to opt-in classes and members.
4) **Auto-generation pipeline (from C++ to JS/TS)**
   - Extract metadata from QObject meta-object data (moc) or clang:
     - class name, namespace, Q_INVOKABLE/public slots, signals, parameter names/types, return types.
   - Validate supported types (QVariant, QVariantHash/List, QJsonValue/Object/Array, QString, bool, numeric, lists/maps of those).
   - On unsupported signatures, emit warnings (QCritical with full JS stack trace).
   - Generate C++ bridge glue and register objects via QWebChannel as `window.HostApi.*`.
   - Generate `hostapi.d.ts` and Angular services that mirror the C++ surface.
   - Generate event helpers:
     - `registerEventHandler(eventName, handler)`
     - `removeEventHandler(eventName, handler)`
     - Event names map to signal names; payloads are JS-friendly serialized values.
5) **Angular integration workflow**
   - Provide a sample Angular app and pipeline that:
     - Builds web assets in a standard Angular flow.
     - Pulls project-specific dependencies from tarballs (not npm) where required.
   - Sync generated HostApi Angular services into the app for DI (`npm run sync:hostapi`).
6) **Testing and validation**
   - Unit/integration tests for:
     - JS -> C++ calls for Q_INVOKABLE/slots.
     - C++ -> JS events from Qt signals.
     - Type validation and error reporting.
   - Headless WebEngine tests for CI.
7) **Packaging and distribution**
   - Bundle web assets into the binary for release builds.
   - Publish generated TS types + Angular services as an NPM package.
   - Version the HostApi contract and enforce compatibility checks.

# New build-time knobs
- `WEBHOST_USE_QRC=ON`: embed web assets and default to QRC root.
- `WEBHOST_COPY_WEB=ON|OFF`: control runtime copying of `web/` into `build/bin/web`.
- `HostApiCodegen`: generates C++ + TS artifacts into `build/generated/hostapi`.

# Agent prompt
You are the implementation agent for this repo. Start by reading:
- /home/node/project/AGENTS.md
- /home/node/project/QtWebIntegrationView/SUGGESTIONS.md
- /home/node/project/QtWebIntegrationView/AUTO_GEN_PIPELINE.md

Use the relevant skills as needed: qt-webengine-webchannel, qt-metaobject-introspection, cmake-codegen,
qt-resource-urlscheme, qt-js-serialization, ts-angular-codegen, angular-build-packaging,
npm-packaging-versioning, qt-webengine-testing, api-versioning.

Goal: move from demo to a production-ready framework. Implement the steps in Instructions.md, prioritizing:
1) QRC packaging for web assets and proper runtime switching (setRootDir for dev, setRootQrc for prod).
2) Auto-generation pipeline scaffolding:
   - Extract Qt meta-object metadata from opted-in classes.
   - Validate allowed types.
   - Generate C++ bridge glue, TS types, Angular services.
   - Generate event helpers mapping Qt signals to JS registerEventHandler.
3) HostApi versioning/handshake and compatibility checks.
4) A sample Angular app workflow that builds into the web root (dev) and supports tarball deps.

Constraints:
- HostApi is injected from C++ only; web assets must wait for HostApiReady.
- Keep dev workflow simple for web devs (no C++ rebuild required for web changes).
- Update docs (AGENTS.md, Instructions.md, SUGGESTIONS.md) to reflect changes.

Run tests (or add/update tests) and report results.

Permissions:
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
