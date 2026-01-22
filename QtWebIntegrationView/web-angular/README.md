# WebHost Angular Sample

This sample Angular app builds into `QtWebIntegrationView/web` so the Qt demo can load it.

## Setup
1. Build HostApi codegen (generates the local NPM package used below):
   - `cmake -S .. -B ../build -G Ninja`
   - `cmake --build ../build`
   - `npm pack` (from `../build/generated/hostapi/npm`)
2. Sync generated Angular services into the app:
   - `npm run sync:hostapi`
3. Install dependencies:
   - `npm install`

## Dev build (watch)
- `npm run watch`

## Production build
- `npm run build:prod`

## Tests
- `CHROME_BIN=/usr/bin/chromium npm test`

Notes:
- The app syncs generated services into `src/generated` for Angular DI.
- The web app does not define `window.HostApi`; it waits for `HostApiReady`.
