# Auto-Generation Pipeline (Draft)

## Goals
- Generate a stable HostApi contract from C++ classes.
- Emit TypeScript types + Angular services that mirror the C++ surface.
- Provide consistent event handling for Qt signals (JS-friendly payloads).
- Keep HostApi bootstrapping in C++ so web assets remain framework-only.

## Current scaffolding (implemented)
- `hostapi/` holds opt-in classes, version, and event types.
- `tools/HostApiGenerator/` builds a Qt-based generator that introspects QMetaObject data.
- Generated outputs land in `build/generated/hostapi` (C++ glue + schema + TS/Angular).

## Inputs
- C++ classes intended for exposure (QObject-derived, with Q_OBJECT).
- Public slots / Q_INVOKABLE methods for callable RPC.
- Public Qt signals for event emission.
- Optional annotations (macros or Q_CLASSINFO) for naming, visibility, and type hints.

## Compatibility rules (JS-friendly)
- Allowed signal/method types: QVariant, QVariantHash, QVariantList, QJsonValue, QJsonObject,
  QJsonArray, QString, bool, integral/float types, and lists/maps of those.
- Unsupported types require a custom serializer or must be excluded.

## Pipeline overview
1) **Annotate**
   - Add marker to opt-in classes that should become part of the HostApi, all public members (methods, variables, slots, signals) which are viable for translation between QT and JavaScript/TypeScript.
   - Example: Class ExampleApiMemberClass : public QWidget { Q_OBJECT public: ExampleApiMemberClass( QObject* parent );/* Obviously cannot be constructed in Js */ signals: void exampleEvent(Qstring); /* obviously can be, and results in window.HostApi.ExampleApiMemberClass.registerEventHandler('exampleEvent', handler(<string>)); being available*/ public slots: void setName(QString name, QString domain); /* obviously can be, and results in window.HostApi.ExampleApiMemberClass.setName(string: name, string: domain); being available, including parameter types and names )/* };
2) **Extract metadata**
   - Use Qt meta-object data (moc) or a clang-based tool to parse classes.
   - Collect: class name, namespace, invokable methods, signals, parameter/return types.
3) **Validate**
   - Enforce the JS-friendly type whitelist.
   - Emit warnings for unsupported signatures. (QCritical dialog with full javascript stack trace)
4) **Generate C++ bridge glue**
   - Create QObject wrappers if needed (for grouping/namespacing).
   - Expose generated objects via QWebChannel as HostApi.* members.
5) **Generate JS/TS artifacts**
   - `hostapi.d.ts` for static typing.
   - Angular services per exposed class, with methods that call through HostApi.
6) **Generate event helpers**
   - For each class with signals, generate a `registerEventHandler(eventName, handler)` method
     on that HostApi member object.
   - Event name defaults to signal name (e.g., `currentUserInfoUpdated`).
7) **Bootstrap in C++**
   - Inject a generated HostApi bootstrap script into the web view after load
     (or via a preload script).
   - The web page should not define HostApi manually.
8) **Example Angular Component**
   - An example usage of the WebHost class where an example angular app can be built with typical angular pipelines (it pulls project-specific dependencies from tarballs, not from npm)


## Event handling contract
- Each generated HostApi member object exposes:
  - `registerEventHandler(eventName, handler)`
  - `removeEventHandler(eventName, handler)`
- The mapping ties directly to Qt signal names for that class, with payloads serialized
  through the agreed JS-friendly types.
- Example (conceptual):
  - C++ signal: `void currentUserInfoUpdated(UserInfo info);`
  - JS: `window.HostApi.users.registerEventHandler('currentUserInfoUpdated', handler);`

## Outputs
- Generated C++ glue code (compiled into the app).
- Generated JS/TS assets (versioned and published as an NPM package).
- Optional JSON schema for tooling and validation, (this should not be needed as the typescript types included with the NPM module should include all required information)

## Implementation notes
- Start with Qt meta-object data to avoid a clang dependency.
- Use CMake custom commands to run the generator and add outputs to the build.
- Keep generated files in a dedicated directory (e.g., `generated/`).
- Add unit tests around a small sample class set to verify:
  - Method calls map correctly to C++ slots/Q_INVOKABLEs.
  - Signals appear as JS events and carry expected payloads.

## Work notes
- Run unit tests, fix any issues, repeat until test pass.
