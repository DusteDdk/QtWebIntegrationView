---
name: api-versioning
description: Use when defining HostApi compatibility, versioning, and migration strategy.
---

# HostApi Versioning

## When to use
- Managing breaking vs non-breaking API changes.
- Coordinating C++/JS version compatibility.

## Workflow
1) **Define version source**
   - Keep a single source of truth (e.g., C++ constant).
2) **Expose version**
   - Publish as `HostApi.version` or `HostApi.getVersion()`.
3) **Compatibility rules**
   - Use semver and document breaking changes.
   - Gate new methods behind feature flags if needed.
4) **Handshake**
   - On bootstrap, emit version info and log mismatches.

## Guardrails
- Avoid silent breaking changes.
- Keep old fields for one deprecation cycle if possible.

## Execution permissions
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
