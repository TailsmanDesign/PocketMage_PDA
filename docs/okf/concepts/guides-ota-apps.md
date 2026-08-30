---
type: guide
title: "OTA Apps"
description: "How third-party PocketMage apps work."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/guides/ota-apps/"
path: /guides/ota-apps/
updated: 2026-08-30
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-30T22:49:51.395Z"
---
---
title: "OTA Apps"
description: "How third-party PocketMage apps work."
---

# OTA Apps

OTA apps are separate programs that PocketMageOS loads into one of four OTA slots at runtime through the app loader. They let third parties ship apps without reflashing the OS. This page explains what an OTA app is, how it is built, and what it can rely on at runtime.

## What an OTA app is

An OTA app is the same three-function shape as a native app, but compiled standalone with the `OTA_APP` environment. The loader calls its entry points directly:

- `APP_INIT()`
- `processKB_APP()`
- `einkHandler_APP()`

## The template

Start from `Code/PocketMageOS/src/APP_TEMPLATE.cpp`. It shows the three entry points and the `#if OTA_APP` guard that keeps the code out of normal OS builds.

## Building an OTA app

Build with the `OTA_APP` environment from `Code/PocketMageOS/`:

```bash
pio run -e OTA_APP
```

That environment defines `OTA_APP_FLAG=1`, which:

- sets `OTA_APP == true` in `config.h`
- excludes the native OS app set from the build
- makes the template's entry points the program

See [Build Environments](../development/build-environments.md) for the details.

## Packaging and installing

1. Build the app with `pio run -e OTA_APP`.
2. Package the result as a `.tar` file.
3. Copy the `.tar` to the device, typically over USB.
4. Open the app loader on the device and use the swap action to install it into one of the four slots (`A` through `D`).

Installed apps are tracked in `OTA1_APP` through `OTA4_APP` in `globals.h`. Launch an installed app from the home screen by typing its slot letter, or from the loader.

## What an OTA app should assume

- It runs without the native PocketMageOS app set. Do not call functions from `OS_APPS/`.
- It should stay self-contained.
- It uses the PocketMage Library for hardware access, exactly like native apps.
- There is no version or ABI compatibility check between an app and the OS core it loads into. An app built against a different OS version loads without warning, so test against the OS version you target and keep the code resilient.

## Read next

- [Making Apps](making-apps.md)
- [PocketMage Library](../reference/pocketmage-library.md)
- [App API Reference](../reference/app-api.md)
- [Build Environments](../development/build-environments.md)
