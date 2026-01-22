---
name: qt-resource-urlscheme
description: Use when serving web assets via Qt resources or a custom webhost:// scheme.
---

# Qt Resources and Custom Schemes

## When to use
- Packaging web assets into the binary.
- Locking asset access to an internal scheme.
- Switching between dev (dir) and prod (qrc) roots.

## Workflow
1) **QRC path**
   - Add `web.qrc` to a target and include `/web/index.html`.
   - Load with `qrc:/web/index.html` (or a root like `qrc:/web`).
2) **Custom scheme (optional)**
   - Register `webhost://` via `QWebEngineUrlScheme`.
   - Implement `QWebEngineUrlSchemeHandler` for stricter control.
3) **Runtime switching**
   - Use `setRootDir()` for local dev rebuilds.
   - Use `setRootQrc()` for packaged builds.

## Guardrails
- If using QRC or custom scheme, consider disabling file URL access.
- Keep one authoritative source of assets per mode.

## Minimal checklist
- [ ] `.qrc` added to target
- [ ] `setRootQrc()` loads index.html
- [ ] Scheme handler blocks unknown URLs (if used)

## Execution permissions
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
