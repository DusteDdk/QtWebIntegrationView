---
name: npm-packaging-versioning
description: Use when packaging generated TS/Angular artifacts and managing HostApi versions.
---

# NPM Packaging + Versioning

## When to use
- Publishing generated TS types and Angular services.
- Keeping HostApi versions in sync across C++ and JS.

## Workflow
1) **Package layout**
   - Include `package.json`, `hostapi.d.ts`, generated services, and README (optional).
2) **Versioning**
   - Use semver; bump on HostApi changes.
   - Add a HostApi version constant in C++ and expose to JS.
3) **Distribution**
   - `npm pack` for tarball distribution or publish to registry.

## Guardrails
- Only ship generated files, not build intermediates.
- Keep version in a single source of truth and propagate.

## Execution permissions
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
