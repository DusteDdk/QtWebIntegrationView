---
name: qt-metaobject-introspection
description: Use when extracting QMetaObject metadata to generate HostApi surfaces or validate Qt types.
---

# Qt Meta-Object Introspection

## When to use
- Auto-generating HostApi from C++ classes.
- Enumerating Q_INVOKABLE/public slots or signals.
- Mapping Qt types to JS/TS types.

## Workflow
1) **Select exposed classes**
   - Require explicit opt-in (e.g., Q_CLASSINFO or macro).
2) **Extract metadata**
   - Use `QObject::metaObject()`.
   - Iterate `QMetaObject::methodCount()` and `QMetaObject::method(i)`.
3) **Filter members**
   - Signals: `method.methodType() == QMetaMethod::Signal`.
   - Invokable: `method.methodType() == QMetaMethod::Method` or `Slot` and `method.access() == Public`.
4) **Capture signature**
   - `method.name()`, `method.parameterTypes()`, `method.parameterNames()`, `method.returnType()`.
5) **Validate types**
   - Only allow JSON/JS-friendly Qt types; reject or warn on unsupported ones.
6) **Emit model**
   - Produce a schema used for C++ glue + TS generation.

## Type rules (baseline)
- Allowed: QVariant, QVariantHash/List, QJsonValue/Object/Array, QString, bool, numeric, lists/maps of those.
- For other types, require an explicit serializer or exclude the member.

## Notes
- Use `QMetaType` to map type ids to names.
- Prefer `QMetaMethod::methodSignature()` for a stable key.
- Parameter names are only available if the class is compiled with proper moc metadata.

## Execution permissions
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
