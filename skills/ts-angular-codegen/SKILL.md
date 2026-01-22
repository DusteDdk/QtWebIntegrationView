---
name: ts-angular-codegen
description: Use when generating TypeScript types and Angular services from HostApi metadata.
---

# TS + Angular Codegen

## When to use
- Generating hostapi.d.ts and Angular services.
- Keeping JS/TS in sync with C++ HostApi.

## Workflow
1) **Model**
   - Start from the C++ metadata model (classes, methods, signals, types).
2) **Generate types**
   - Emit interfaces for each HostApi member object.
   - Emit a root `HostApi` interface and `window.HostApi` typing.
3) **Generate services**
   - Provide Angular injectable services that call `window.HostApi.<member>`.
   - Include event helpers for signals.
4) **Package**
   - Emit into an NPM-ready folder (types + services).

## Guardrails
- Preserve parameter names for ergonomic TS signatures.
- Signal handlers should reflect payload types.
- Keep HostApiReady as the bootstrap signal.

## Output checklist
- [ ] hostapi.d.ts
- [ ] Angular services per class
- [ ] Global `window.HostApi` typing

## Execution permissions
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
