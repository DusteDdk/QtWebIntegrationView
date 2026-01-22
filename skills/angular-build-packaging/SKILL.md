---
name: angular-build-packaging
description: Use when wiring Angular builds to the Qt web asset pipeline (dev vs prod).
---

# Angular Build + Asset Packaging

## When to use
- Wiring Angular builds to `web/` for local dev.
- Packaging assets for QRC/webhost scheme.

## Workflow
1) **Dev mode**
   - Use `ng build --watch` targeting the `web/` output dir.
   - Set base href to `./` so assets load under file/qrc.
2) **Prod mode**
   - Build optimized assets into `web/` for packaging.
   - Ensure no HostApi bootstrap JS in the app; wait for `HostApiReady`.
3) **Qt integration**
   - Use `setRootDir()` in dev, `setRootQrc()` in production.

## Guardrails
- Avoid absolute URLs in asset references.
- Avoid WebChannel bootstrap in the web app.

## Minimal checklist
- [ ] Build output goes to Qt web root
- [ ] Base href set to relative
- [ ] HostApiReady listener in app

## Execution permissions
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
