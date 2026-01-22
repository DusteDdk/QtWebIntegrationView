---
name: cmake-codegen
description: Use when wiring code generation into CMake builds (custom commands, byproducts, dependencies).
---

# CMake Codegen Pipeline

## When to use
- Generating C++/TS code from metadata during builds.
- Adding codegen dependencies to Qt targets.

## Workflow
1) **Define outputs**
   - Choose a `generated/` directory in the build or source tree.
2) **Add custom command**
   - `add_custom_command(OUTPUT ... COMMAND <generator> DEPENDS <inputs>)`.
3) **Create a target**
   - `add_custom_target(HostApiGen DEPENDS <outputs>)`.
4) **Wire dependencies**
   - `add_dependencies(AppTarget HostApiGen)`.
   - Add generated sources via `target_sources()`.
5) **Track byproducts**
   - Use `BYPRODUCTS` to ensure Ninja/Make see all outputs.

## Guardrails
- Inputs must be explicit so changes trigger regeneration.
- Generated files should live in a predictable path and be ignored by VCS if built output.
- Avoid regenerating on every build by keeping dependencies accurate.

## Template (pseudo)
- add_custom_command OUTPUT: generated cpp/h + ts
- add_custom_target HostApiGen
- target_sources(WebHost ... generated cpp)
- add_dependencies(WebHost HostApiGen)

## Execution permissions
- Agents may install packages with apt-get and npm when needed.
- Agents may run any available Linux command, including sudo, to accomplish tasks.
