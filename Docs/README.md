# PocketMage Documentation

The documentation site for the PocketMage PDA project.

**Live docs: <https://talismandesign.github.io/PocketMage_PDA/docs/>**

---

## What's in here

The `docs/` folder contains all the source Markdown files that make up the documentation site. Sections include:

- **Getting Started** - what PocketMage is, setup, and first steps
- **PocketMageOS** - the operating system and runtime model
- **Guides** - making apps, native apps, OTA apps, and scripting
- **Reference** - app API, system state, and the PocketMage library
- **Commands** - keymaps and built-in command reference
- **Features** - what PocketMage can do today
- **FAQ** - common questions about the device, building, flashing, and troubleshooting
- **Tutorials** - SD card formatting and setup walkthroughs

The site is built using [docmd](https://github.com/mgks/docmd) and configured via `docmd.config.js`.

---

## Local development

You'll need Node.js 18+ installed.

Install dependencies:

```
npm install
```

Start the local dev server with live reload:

```
npm run dev
```

Build the static site to the `site/` output folder:

```
npm run build
```

Preview the built site locally:

```
npm run preview
```

---

## Contributing to the docs

Documentation source files live in `docs/` as standard Markdown with YAML front matter for title and description metadata.

To add or edit a page:

1. Create or modify a `.md` file in the appropriate subfolder under `docs/`.
2. If you are adding a new page, add it to the `navigation` array in `docmd.config.js` so it appears in the sidebar.
3. Run `npm run dev` to preview your changes locally before submitting.

Keep the writing short and direct. Use the overview pages to route readers into the exact guide or reference page they need.

---

## Project links

- Main repo: <https://github.com/TalismanDesign/PocketMage_PDA>
- Discord: <https://discord.gg/KSCapSf4XH>
- Website: <https://pocketmage.org/>
