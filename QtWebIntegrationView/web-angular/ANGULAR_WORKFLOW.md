# Angular Workflow (HostApi)

## Dev mode (local rebuilds)
- Build into the Qt web root so the C++ app does not need rebuilding.
- From `web-angular`:
  - `npm run watch`
- Run the Qt app with `setRootDir()` (default in dev).
- After regenerating HostApi, sync Angular services: `npm run sync:hostapi`.

## Prod mode (packaged assets)
- Build optimized assets into `QtWebIntegrationView/web`:
  - `npm run build:prod`
- Configure CMake with `-DWEBHOST_USE_QRC=ON` and rebuild so assets are embedded.
- `web.qrc` is generated at build time to include all files under `web/`.

## Tarball dependencies
- Generated HostApi packages can be installed as tarballs (see `build/generated/hostapi/npm/`).
- After codegen, run `npm pack` in that folder to refresh the tarball used by `web-angular`.

## HostApi
- Do not define `window.HostApi` in the web app.
- Wait for `HostApiReady` before calling into the host.

## Tests
- `npm test` (set `CHROME_BIN=/usr/bin/chromium` if Chrome isn't detected).
