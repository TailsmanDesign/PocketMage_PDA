---
title: "Build Environments & PlatformIO Setup"
description: "How to set up PlatformIO and build PocketMage firmware on Linux, macOS, and Windows."
---

# Build Environments & PlatformIO Setup

PocketMage firmware is built with [PlatformIO](https://platformio.org/). The build configuration lives in `Code/PocketMageOS/platformio.ini` and defines three environments that produce different firmware images from the same source.

## The three build environments

| Environment | Target | Purpose |
| --- | --- | --- |
| `PM_PRODUCTION` | Production hardware (N16R2, Quad PSRAM enabled) | The stable OS image flashed to production devices |
| `PM_BETA` | Beta hardware (N16R8, PSRAM forced disabled) | A test build with the latest changes |
| `OTA_APP` | Hardware-agnostic | A single third-party app built to be sideloaded |

The three environments share a common `[common]` section and differ only in build flags. The flags set three things: which hardware revision to target, whether the build is an OS or an OTA app, and the version string.

Running `pio run` with no arguments builds all three. Build one explicitly with `pio run -e <name>`.

### What the OTA_APP flag does

The `OTA_APP` environment defines `OTA_APP_FLAG=1`. In `config.h` this becomes `OTA_APP == true`, which excludes the native OS app set from the build. The three entry points from [APP_TEMPLATE.cpp](https://github.com/TalismanDesign/PocketMage_PDA/blob/main/Code/PocketMageOS/src/APP_TEMPLATE.cpp) - `APP_INIT()`, `processKB_APP()`, `einkHandler_APP()` - become the program. The same source tree therefore builds either a full OS or a standalone app depending on the environment you choose.

## Prerequisites

- [VS Code](https://code.visualstudio.com/) installed
- Python 3 installed
- PlatformIO Core (installed automatically by the VS Code extension, or via `pip install platformio`)
- The PocketMage source code, cloned from [GitHub](https://github.com/TalismanDesign/PocketMage_PDA)

The ESP32 platform support downloads automatically on the first build, so the first build is noticeably slower.

## Setup

### Linux

1. Install `python3-venv` if it is missing:

   ```bash
   sudo apt install python3-venv
   ```

2. Install the [PlatformIO IDE](https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode) extension in VS Code.
3. Open the PocketMage source folder in VS Code.
4. When PlatformIO asks for a project folder, choose `Code/PocketMageOS/`.
5. Build from the PlatformIO toolbar or the command palette.

### macOS

Use the same steps as Linux. No extra setup is required.

### Windows

1. Install [Git](https://git-scm.com/) to clone the repository.
2. Follow the same PlatformIO IDE steps as Linux.
3. Build from the PlatformIO toolbar in VS Code.

## Command line

From `Code/PocketMageOS/`:

```bash
pio run -e PM_PRODUCTION   # production firmware
pio run -e PM_BETA         # beta firmware
pio run -e OTA_APP         # a third-party OTA app
```

## OTA apps

OTA apps use the `OTA_APP` environment, which switches the build into app mode as described above. The app is packaged as a `.tar` file and installed on the device through the app loader. See [OTA Apps](../guides/ota-apps.md) and the [app loader section](../command-manual/index.md) of the command manual.

For a full walkthrough, watch the [OTA app development video](https://www.youtube.com/watch?v=3Ytc-3-BbMM).

## Testing

There is no automated test suite for the firmware. Verify changes against real hardware, or review the build log for changes with no runtime effect. CI builds `PM_PRODUCTION` and `PM_BETA` on every push, so a clean build locally is the minimum bar before opening a pull request.

## Next steps

::: grids
::: grid
::: button "PocketMageOS overview" ./index.md icon:cpu
:::
::: grid
::: button "Making Apps" ../guides/making-apps.md icon:code
:::
::: grid
::: button "OTA Apps" ../guides/ota-apps.md icon:package
:::
::: grid
::: button "Firmware FAQ" ../faq/index.md icon:help-circle
:::
:::
