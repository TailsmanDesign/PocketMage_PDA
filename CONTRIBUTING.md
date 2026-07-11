# Contributing to PocketMage

Thanks for your interest in contributing! This guide covers everything you need to get started.

---

## Table of contents

1. [Getting the code](#getting-the-code)
2. [Building the firmware](#building-the-firmware)
3. [Testing without hardware](#testing-without-hardware)
4. [Coding conventions](#coding-conventions)
5. [Submitting changes](#submitting-changes)
6. [Third-party apps (Bazaar)](#third-party-apps-bazaar)
7. [AI-assisted contributions](#ai-assisted-contributions)
8. [Community](#community)

---

## Getting the code

```bash
git clone https://github.com/TailsmanDesign/PocketMage_PDA.git
```

Main firmware source lives under `Code/PocketMage_V3/`. Hardware design files (KiCad schematics/PCBs) are under `Resources/PCB/`.

---

## Building the firmware

PocketMage uses [PlatformIO](https://platformio.org/). From `Code/PocketMage_V3/`:

```bash
pio run -e PM_PRODUCTION   # Production hardware (N16R2, Quad PSRAM)
pio run -e PM_BETA         # Beta hardware (N16R8, PSRAM disabled)
pio run -e OTA_APP         # Universal OTA app build (hardware-agnostic)
```

CI (`build-firmware.yml`) builds `PM_PRODUCTION` and `PM_BETA` on every push — run the same commands locally before opening a PR.

**Minimum requirements:** PlatformIO Core (via `pip install platformio`), ESP32 platform support (installed automatically on first build).

---

## Testing without hardware

There is currently **no automated test suite** — `src/readme.md` references a `pio test -e native` environment, but no `[env:native]` exists in `platformio.ini`. Treat that reference as aspirational, not a real capability, until someone adds it.

Manual verification against real hardware (or a build log review, for changes with no runtime effect) is the only current path — say what you tested in your PR description.

---

## Coding conventions

- **Language:** C++ (Arduino framework, ESP32-S3 target).
- **Pin/peripheral config:** centralised in `include/config.h` — add new pin definitions there, not scattered across source files.
- **App dispatch:** built-in apps are wired into a `switch (CurrentAppState)` in `PocketMageV3.cpp` (`applicationEinkHandler()`/`processKB()`), plus the `AppState` enum in `globals.h`. Follow the existing pattern (see `// ADD APP CASES HERE`) when adding a new built-in app.
- **Shared state:** `CurrentAppState` and similar globals are read from one FreeRTOS task (E-Ink handler, core 0) and written from another (`processKB()`, core 1) with no explicit synchronisation today — be aware of this if your change touches app-state transitions.

---

## Submitting changes

1. **Open or comment on an issue first.** This avoids duplicated effort and lets the maintainer give early feedback on the approach.
2. **Check for open PRs** on the same topic before starting — search by title as well as issue cross-references.
3. **Fork the repository** and create a branch from `main`.
4. **Keep PRs small and focused** — one concern per PR makes review much faster.
5. **Describe what and why** in the PR body. Include how you tested it (which hardware revision, if applicable).

---

## Third-party apps (Bazaar)

PocketMage supports loading third-party "Bazaar" apps via `APPLOADER.cpp`. The only current guide is `src/readme.md` plus a video walkthrough — start from `APP_TEMPLATE.cpp` for the required function stubs (`APP_INIT`, `processKB_APP`, `einkHandler_APP`).

Note: there is currently no version/ABI compatibility check between a Bazaar app and the OS core it's loaded into — an app built against a different OS version will load without warning. Worth bearing in mind if your app relies on specifics of `globals.h` or pin layout that might change between OS releases.

---

## AI-assisted contributions

AI tools are welcome for drafting code or documentation, but **every submission must be reviewed and understood by the person submitting it** — you should be able to explain how your own code works, not just that an assistant produced it. If a contribution was substantially generated or guided by an AI tool, please say so briefly in the PR description — for example: *"Drafted with an AI assistant, reviewed and tested manually."* This helps the maintainer understand the provenance of the code and focus review effort appropriately.

---

## Community

See the [docs site](https://tailsmandesign.github.io/PocketMage_PDA/docs/) for the FAQ, tutorials, and command manual.
