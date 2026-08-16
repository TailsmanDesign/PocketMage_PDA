---
type: reference
title: Reference
description: "Exact-use reference pages for PocketMage app authors and contributors."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/reference/"
path: /reference/
updated: 2026-08-16
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-16T21:58:43.074Z"
---
---
title: "Reference"
description: "Exact-use reference pages for PocketMage app authors and contributors."
---

# Reference

Use these pages when you need the exact API, state, or helper names. This section is the precise counterpoint to the guides: the guides explain how to build, the reference pages document exactly what to call.

## Pages

- [App API Reference](app-api.md) - app entry points, helpers, and shared patterns
- [PocketMage Library](pocketmage-library.md) - the shared firmware modules under `lib/PocketMage/`
- [System State Reference](system-state.md) - globals, settings, and runtime state
- [Command Manual](../command-manual/index.md) - keyboard mappings and built-in app commands
- [Scripting](../scripting/index.md) - Wrench examples and command reference

## When to use this section

Start here if you are writing an OTA app, touching shared app state, or looking for the current function names used by the firmware.

- **Writing an app** - read [App API](app-api.md) for the entry points, then [PocketMage Library](pocketmage-library.md) for the modules you can call.
- **Adding an app** - read [System State](system-state.md) to see the `AppState` enum and dispatch switch.
- **Automating** - read [Scripting](../scripting/index.md) for the Wrench bindings.
- **Using the device** - read the [Command Manual](../command-manual/index.md).

## Next steps

::: grids
::: grid
::: button "App API Reference" ./app-api.md icon:book
:::
::: grid
::: button "PocketMage Library" ./pocketmage-library.md icon:box
:::
::: grid
::: button "System State Reference" ./system-state.md icon:settings
:::
::: grid
::: button "Scripting" ../scripting/index.md icon:code
:::
:::
