---
name: qt-js-serialization
description: Use when mapping Qt types to JSON/JS payloads and back for HostApi.
---

# Qt <-> JS Serialization

## When to use
- Defining allowed types for HostApi methods/signals.
- Converting QVariant/QJsonValue to JS-friendly payloads.

## Workflow
1) **Define allowed types**
   - QVariant, QVariantHash/List, QJsonValue/Object/Array, QString, bool, numeric, lists/maps of those.
2) **Convert outbound values**
   - Use QJsonValue/QJsonDocument to serialize payloads.
   - Map `undefined` to `null` when emitting JS.
3) **Convert inbound values**
   - Accept QJsonValue or QVariant from WebChannel.
   - Validate payload types and reject unsupported fields.

## Pitfalls
- QVariantMap vs QVariantHash ordering differences.
- QDateTime/QByteArray need explicit serialization (string/base64).
- `undefined` is not JSON; normalize to null.

## Minimal checklist
- [ ] Type whitelist enforced
- [ ] Deterministic JSON serialization
- [ ] Clear errors for unsupported types

## Execution permissions
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
