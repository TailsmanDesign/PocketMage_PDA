---
type: concept
title: "PocketMage FAQ"
description: "Frequently asked questions about the PocketMage PDA device."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/faq/"
path: /faq/
updated: 2026-08-04
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-04T02:13:37.649Z"
---
---
title: "PocketMage FAQ"
description: "Frequently asked questions about the PocketMage PDA device."
---

# PocketMage FAQ

## Helpful links

- [PocketMage.org](https://pocketmage.org/)
- [PocketMage on GitHub](https://github.com/TailsmanDesign/PocketMage_PDA)
- [PocketMage on Discord](https://discord.gg/KSCapSf4XH)
- [Ashtf on YouTube](https://www.youtube.com/@ashtf)
- [Ashtf on Patreon](https://patreon.com/Ashtf)

## General information

### What is PocketMage?

PocketMage is a hackable, open-source PDA focused on productivity. Use it to take notes, keep track of tasks, journal, schedule your week, and more. It is a clamshell design with a clear shell, a full tactile thumb keyboard, a large E-Ink main display, and a smaller OLED secondary display. All hardware and software is open source, which makes it ideal for hacking and add-ons.

### How much is the kit?

The final production kit is not released yet and the price is not final. The target is around $150 for a full kit.

### How hard is it to build the kit?

The kit is designed so anyone can build it. The only tool you need is one allen key, and there is no soldering, programming, or electronics involved in the build process.

### Will I be able to get PocketMage if I live outside the USA?

Yes. We plan to ship worldwide.

### When can I order?

There is no set release date yet. Once the beta production run is complete and the software matures, we will move to final production.

### How can I get notified when the kit releases?

Visit [PocketMage.org](https://pocketmage.org/) and click the "Join the Mailing List!" button.

### How can I keep up with the project?

Subscribe to [Ashtf's YouTube channel](https://www.youtube.com/@ashtf) for video updates, [Ashtf's Patreon](https://patreon.com/Ashtf) for exclusive weekly written updates, and the [PocketMage Discord server](https://discord.gg/KSCapSf4XH) for community updates.

## Building and operating the PocketMage

### Where is the build guide?

The video build guide is [here](https://youtu.be/2aNcC0qCK1o?si=HKqUIyK0ZPkjqzQI).

### Is there a guide on the key operations?

Yes. Use the [Command Manual](../command-manual/index.md).

### How do I flash the firmware?

1. Carefully remove the battery (pull on the plastic connector).
2. Ensure the DIP switch for programming mode is ON.
3. Hold the power button while plugging the PocketMage into your computer. Nothing should happen.
4. Open [The Web Flasher](https://tailsmandesign.github.io/PocketMage_PDA/) in a Chrome-based browser and select your firmware version. The most recent stable build is recommended.
5. If you want to keep your saved settings, check the "Preserve user data during installation" box.
6. Hit the flash button and select the PocketMage from the list of USB devices.
7. When flashing is done, unplug the PocketMage and re-insert the battery.

PocketMage should boot up automatically.

### What SD card can I use?

Use a microSD card formatted as FAT32 with an MBR partition table. Cards up to 32 GB are recommended; 8 GB is a good sweet spot because PocketMage files are small. The smaller the card, the faster the device. Cards larger than 32 GB must be explicitly forced to FAT32 + MBR and are not recommended. See [Format a MicroSD Card](../tutorials/format-micro-sd.md) for the exact steps.

### Where are the CAD files?

CAD files are available only to beta testers until the final production release. If you are a beta tester, find the password-protected archive [here](https://github.com/TailsmanDesign/PocketMage_PDA/tree/main/Resources/CAD) and the password in the beta Discord channel.

### Where can I get an extra battery?

International kits do not come with a battery because of shipping regulations. Battery links for common countries are [here](https://pocketmage.org/get/).

### What is the programming mode switch?

The programming mode switch connects the USB port directly to the microcontroller, bypassing the USB and battery charge controller. You can keep it on and use the device normally, but charging is limited to 500 mA instead of fast charging.

## Build and app docs

For the current setup path, start here:

- [Build Environments](../development/build-environments.md)
- [App API Reference](../reference/app-api.md)
- [System State Reference](../reference/system-state.md)

## Common issues

### My dictionary isn't working

The English dictionary is not installed out of the box and must be downloaded to the SD card. Find the dictionary files [here](https://github.com/TailsmanDesign/PocketMage_PDA/tree/main/Resources/Assets/Dictionary%20(OPTED)). Connect the PocketMage to your computer with the USB app and drop all the `.txt` files into the dictionary folder.

### I can't flash the firmware

Ensure the battery is unplugged and the programming mode DIP switch is ON. See the flashing guide above.

### My SD card isn't working

First, ensure the card is 32 GB or smaller (8 GB is recommended). Next, ensure it is formatted as FAT32 with an MBR partition table. It will not work otherwise. See [Format a MicroSD Card](../tutorials/format-micro-sd.md).

### I found a software bug

Report the bug in detail on the [GitHub issues page](https://github.com/TailsmanDesign/PocketMage_PDA/issues), including steps to reproduce.
