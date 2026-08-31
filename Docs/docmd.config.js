export default {
  title: "PocketMage",
  url: "https://talismandesign.github.io/PocketMage_PDA/docs",
  logo: { alt: "PocketMage", href: "./" },
  favicon: "/assets/favicon.png",
  theme: {
    name: "ruby",
    defaultMode: "system",
    enableModeToggle: true,
    positionMode: "top",
    codeHighlight: true,
    customCss: ["/assets/css/theme.css"],
    copyWidgets: {
      enabled: true,
      raw: true,
      context: true,
    },
  },
  layout: {
    footer: {
      style: "complete",
      description: "A clamshell PDA powered by an ESP32-S3 with E-Ink and OLED displays.",
      branding: true,
      columns: [
        {
          title: "Resources",
          links: [
            { text: "Command Manual", url: "./command-manual/" },
            { text: "FAQ", url: "./faq/" },
            { text: "Build Environments", url: "./development/build-environments" },
          ],
        },
        {
          title: "Community",
          links: [
            { text: "GitHub", url: "https://github.com/TalismanDesign/PocketMage_PDA" },
            { text: "Discord", url: "https://discord.gg/KSCapSf4XH" },
            { text: "Website", url: "https://pocketmage.org/" },
          ],
        },
      ],
    },
  },
  plugins: {
    search: {
      semantic: true,
      showConfidence: true,
    },
    seo: {
      defaultDescription:
        "PocketMage is an open-source clamshell PDA powered by an ESP32-S3 with E-Ink and OLED displays. Notes, calendar, tasks, and more.",
      openGraph: { defaultImage: "/assets/images/og-image.png" },
      twitter: { cardType: "summary_large_image" },
    },
    sitemap: {
      defaultChangefreq: "weekly",
      defaultPriority: 0.8,
    },
    mermaid: {},
    git: {},
    llms: {
      fullContext: true,
    },
  },
  search: true,
  minify: true,
  autoTitleFromH1: true,
  copyCode: true,
  pageNavigation: true,
  navigation: [
    { title: "Home", path: "/", icon: "home" },
    {
      title: "Getting Started",
      icon: "rocket",
      collapsible: true,
      path: "/getting-started/index",
      children: [
        { title: "What is PocketMage", path: "/getting-started/what-is-pocketmage", icon: "info" },
        { title: "Build Environments", path: "/development/build-environments", icon: "settings" },
      ],
    },
    {
      title: "PocketMageOS",
      icon: "cpu",
      collapsible: true,
      path: "/development/index",
      children: [
        { title: "Features", path: "/features/index", icon: "star" },
      ],
    },
    {
      title: "Guides",
      icon: "book-open",
      collapsible: true,
      path: "/guides/index",
      children: [
        { title: "Making Apps", path: "/guides/making-apps", icon: "code" },
        { title: "Native Apps", path: "/guides/native-apps", icon: "terminal" },
        { title: "OTA Apps", path: "/guides/ota-apps", icon: "package" },
      ],
    },
    {
      title: "Reference",
      icon: "book",
      collapsible: true,
      path: "/reference/index",
      children: [
        { title: "App API", path: "/reference/app-api", icon: "code" },
        { title: "PocketMage Library", path: "/reference/pocketmage-library", icon: "box" },
        { title: "System State", path: "/reference/system-state", icon: "settings" },
      ],
    },
    { title: "Commands", path: "/command-manual/index", icon: "keyboard" },
    { title: "FAQ", path: "/faq/index", icon: "help-circle" },
    {
      title: "Tutorials",
      icon: "folder-open",
      collapsible: true,
      path: "/tutorials/index",
      children: [
        { title: "Format MicroSD Card", path: "/tutorials/format-micro-sd", icon: "card-sd" },
      ],
    },
    {
      title: "Scripting",
      icon: "terminal",
      collapsible: true,
      path: "/scripting/index",
      children: [
        { title: "Basic Input/Output", path: "/scripting/example-c", icon: "terminal" },
        { title: "E-Ink Drawing", path: "/scripting/ink-c", icon: "image" },
        { title: "OLED Drawing", path: "/scripting/oled-c", icon: "monitor" },
        { title: "Full Command Reference", path: "/scripting/fullPotionCommandList", icon: "book" },
      ],
    },
    {
      title: "GitHub",
      path: "https://github.com/TalismanDesign/PocketMage_PDA",
      icon: "github",
      external: true,
    },
  ],
  footer: "Built with [docmd](https://docmd.io). [View on GitHub](https://github.com/TalismanDesign/PocketMage_PDA).",
  editLink: {
    enabled: true,
    baseUrl: "https://github.com/TalismanDesign/PocketMage_PDA/edit/main/Docs/docs",
    text: "Edit this page",
  },
};
