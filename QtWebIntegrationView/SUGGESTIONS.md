# Suggestions for Moving Forward

Status update: Phase 1-3 scaffolding is now in place (QRC packaging, versioning handshake, HostApi codegen). Angular sample app now lives in `web-angular`; remaining phases focus on expanding API surface and CI hardening.

## Phase 1: Packaged assets with runtime switching
- Keep HostApi bootstrapping in C++ (web assets only wait for `HostApiReady`).
- Switch from `file://` loading to packaged assets (Qt resources or a custom `webhost://` scheme).
- Add a `web.qrc` to include `web/index.html` and static assets.
- Keep `setRootDir()` for local Angular rebuilds and `setRootQrc()` for packaged builds.
- Add a CMake option (e.g., `WEBHOST_USE_QRC`) to toggle default root at build time.
- When using QRC or a custom scheme, disable `LocalContentCanAccessFileUrls`.

## Phase 2: HostApi contract + versioning
- Add a `HostApi.version` string and a handshake event (e.g., `HostApiReady` with version).
- Have the host log or reject mismatched versions (and expose a clear error event to JS).
- Define a single source of truth for valid event types and API members (generated, not hard-coded).

## Phase 3: Auto-generation scaffolding
- Add an opt-in marker (macro or `Q_CLASSINFO`) for HostApi-exposed classes.
- Extract metadata via `QMetaObject` (class name, signals, slots, invokables, param names/types).
- Validate against the JS-friendly whitelist and emit warnings on unsupported types.
- Generate:
  - C++ registration glue for QWebChannel
  - `hostapi.d.ts` types
  - Angular services per exposed class
  - `registerEventHandler` helpers for each signal

## Phase 4: Eventing + RPC ergonomics
- Consider a typed request/response RPC layer (ids, errors, timeouts) over ad hoc events.
- Add structured error reporting both directions (JS <-> C++).
- Ensure generated event helpers map directly to Qt signal names and payloads.

## Phase 5: Angular workflow
- Provide a sample Angular app with a clear build output directory.
- Dev: `setRootDir()` points at the Angular `dist/` folder for hot rebuilds.
- Prod: copy Angular build outputs into QRC at build time (and remove runtime copy).
- Document tarball-based dependency workflow for Angular if npm is restricted.

## Phase 6: Packaging + security
- Embed web assets into the binary for release builds.
- Restrict all URL access to the internal scheme/root (QRC or `webhost://`).
- Keep `WebRootInterceptor` (or the scheme handler) as the single gatekeeper.

## Phase 7: Testing + CI
- Expand headless WebEngine tests to cover:
  - `sendData`, `setOutput`, `getInput` flows
  - signal payload serialization
  - invalid event type behavior
  - version mismatch handling
- Add a basic CI job that runs `WebHostTests` with the headless flags.
