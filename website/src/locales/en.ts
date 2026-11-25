export const en = {
  meta: {
    title: "Sekiro Mod Manager — Zero-Copy Desktop Mod Manager for Sekiro",
  },
  nav: {
    skip: "Skip to main content",
    showcase: "Preview",
    features: "Features",
    download: "Download",
    packs: "Starter Packs",
    faq: "FAQ",
    language: "Switch language",
    menu: "Open navigation menu",
    close: "Close navigation menu",
  },
  hero: {
    eyebrow: "Windows 10 / 11 x64 · Native C++17 / Qt 6 · MIT Open Source",
    title: "Zero-Copy Desktop Mod Manager for Sekiro",
    subtitle:
      "Stage mods cleanly in isolation and project active files into mods/ via NTFS hard links—zero extra disk space, clear conflict arbitration, and one-click pristine rollback.",
    primaryCta: "Download for Windows",
    secondaryCta: "Explore Starter Packs",
    githubCta: "View on GitHub",
  },
  showcase: {
    caption: "Sekiro Mod Manager · Desktop Workbench Screenshots",
    downloadLink: "Download now",
    slides: [
      {
        id: "armoury",
        label: "Mod Library",
        image: "/assets/app/armoury-en.webp",
        summary: "Priority ordering (lower number wins), per-mod and per-asset toggles, category filters, and conflict detail drawer.",
      },
      {
        id: "hub",
        label: "Launch Hub",
        image: "/assets/app/hub-en.webp",
        summary: "At-a-glance deployment telemetry, saved disk space, and 5-point pre-launch environment verification.",
      },
      {
        id: "packs",
        label: "Mod Packs",
        image: "/assets/app/packs-en.webp",
        summary: "Snapshot enabled sets and priority order as reusable presets or portable .smmpack bundles.",
      },
      {
        id: "settings",
        label: "Settings",
        image: "/assets/app/settings-en.webp",
        summary: "Auto-detect Sekiro install paths with same-volume NTFS hard-link hints, dark/light themes, and live bilingual UI.",
      },
    ],
  },
  features: {
    eyebrow: "CORE CAPABILITIES",
    title: "Built for reversible, transparent Sekiro modding",
    subtitle: "Every step from archive import to game launch is designed to stay zero-copy, deterministic, and easy to roll back.",
    items: [
      {
        id: "hardlink",
        badge: "NTFS HARD LINKS",
        title: "Zero-Copy Deployment & Pristine Rollback",
        description:
          "Uses CreateHardLinkW on the same NTFS volume for instant deployment with zero duplicate storage (automatic copy fallback across drives). Every link is tracked in .smm_manifest.json for clean restore.",
      },
      {
        id: "normalize",
        badge: "ZIP / 7Z / RAR",
        title: "Smart Layout Normalization",
        description:
          "Accepts arbitrary Nexus or community folder layouts, unpacks nested archives up to 6 levels deep, routes loose assets to parts/, chr/, param/, menu/font/, and strips unpacking clutter.",
      },
      {
        id: "conflict",
        badge: "3-TIER ARBITRATION",
        title: "Semantic Conflict Arbitration",
        description:
          "Classifies overlaps into Critical (balance tables), Warning (exclusive player/weapon slots), and Info. Priority order decides every winner, with per-file toggles inside the detail drawer.",
      },
      {
        id: "doctor",
        badge: "ENVIRONMENT DOCTOR",
        title: "5-Point Environment Diagnostics",
        description:
          "Dedicated diagnostics page checking sekiro.exe, ModEngine dinput8.dll, modengine.ini, staging volume, and deployment parity with one-click repair. Never bundles or patches game binaries.",
      },
    ],
  },
  workflow: {
    eyebrow: "WORKFLOW",
    title: "Three steps from archive to Ashina",
    steps: [
      {
        step: "01",
        title: "Import mods or .smmpack",
        description: "Drop .zip / .7z / .rar archives, folders, or .smmpack bundles. SMM normalizes paths automatically.",
      },
      {
        step: "02",
        title: "Order priorities & inspect conflicts",
        description: "Toggle mods or individual files and adjust priority order; every shared-file override is visible upfront.",
      },
      {
        step: "03",
        title: "Deploy, launch, or restore",
        description: "Pass the 5-point health check, hard-link into mods/, and play—or click Restore anytime to return to vanilla.",
      },
    ],
  },
  download: {
    eyebrow: "DOWNLOAD CENTER",
    title: "Download Sekiro Mod Manager",
    subtitle: "Choose between our Cloudflare CDN mirror proxy or direct GitHub Releases—both serve byte-identical builds with matching SHA-256 digests.",
    appTitle: "Sekiro Mod Manager · Windows 10 / 11 x64",
    appDesc: "Native C++17 / Qt 6 desktop app. Requests UAC elevation once at startup so Steam honours ModEngine's local dinput8.dll.",
    sourceLabel: "Download source",
    sourceMirror: "CDN Mirror Proxy",
    sourceGithub: "GitHub Direct",
    sourceNote: "The CDN mirror is an allowlisted streaming proxy running on our Cloudflare Worker directly against GitHub Releases.",
    loading: "Checking latest release…",
    latest: "Latest stable",
    unavailable: "Could not load release metadata. Download directly from GitHub Releases.",
    retry: "Retry",
    allReleases: "All GitHub Releases",
    setupTitle: "Setup Installer (.exe)",
    setupBadge: "Recommended",
    setupDesc: "Standard Windows installer with Start Menu / Desktop shortcuts, clean upgrades, and full uninstaller.",
    portableTitle: "Portable Bundle (.zip)",
    portableBadge: "No install",
    portableDesc: "Extract anywhere and launch. Ships pruned Qt 6 and VC++ runtime DLLs with zero system registration.",
    downloadBtn: "Download",
    hashCopy: "Copy SHA-256",
    hashCopied: "Copied",
    hashFailed: "Copy failed",
    modEngineNote:
      "Note: SMM never bundles or redistributes ModEngine (dinput8.dll). Download an unmodified copy from Nexus Mods #6 before deploying.",
    modEngineLink: "Get ModEngine on Nexus Mods",
  },
  packs: {
    eyebrow: "COMMUNITY SHOWCASE · .SMMPACK",
    title: "Community Starter Packs",
    subtitle:
      "Three optional community mod combinations packaged as .smmpack archives so new users can test one-click import, priority switching, and clean rollback right away.",
    routeLabel: "R2 route",
    routeCdn: "Optimized CDN",
    routeOrigin: "R2 Origin",
    includedMods: "Included mods & sources",
    readyHint: "How to use: Download a .smmpack file → open SMM's Mod Packs page → click Import Pack to load mods and preset ordering automatically.",
    downloadPack: "Download .smmpack",
    pendingBtn: "Uploading to R2",
    pendingNote: "R2 direct link coming soon",
    fileLabel: "Filename",
    disclaimerTitle: "Third-Party Mod Ownership & Notice-and-Takedown Policy",
    disclaimerBody:
      "These optional .smmpack starter packs combine community-created mods strictly for non-commercial evaluation and onboarding. They are hosted externally on object storage and are NEVER included in the SMM Git repository or desktop releases. All mod assets remain the sole property of their respective original creators under their original terms (with upstream Nexus Mods links listed in each card); SMM's MIT license does not apply to third-party mod content.",
    takedownLink: "Author attribution update / immediate removal request (GitHub Issues)",
  },
  faq: {
    eyebrow: "FAQ",
    title: "Frequently Asked Questions",
    subtitle: "Quick answers before you start modding.",
    items: [
      {
        q: "Why keep the staging folder on the same drive as Sekiro?",
        a: "NTFS hard links only work within the same volume (e.g., both on D:), making multi-gigabyte deployments instant with zero extra space. Across different drives, SMM automatically falls back to file copying with a diagnostic hint.",
      },
      {
        q: "Why doesn't SMM bundle ModEngine (dinput8.dll)?",
        a: "ModEngine reserves all rights and does not grant permission for third-party managers to redistribute the DLL. SMM detects, validates, and patches modengine.ini while leaving you to obtain an unmodified dinput8.dll directly from Nexus Mods #6.",
      },
      {
        q: "Why does SMM ask for administrator (UAC) elevation on startup?",
        a: "In an unelevated Steam launch chain, Windows' PreferSystem32 DLL policy can cause Sekiro to load the system DINPUT8.dll instead of ModEngine's local hook. Running elevated eliminates this silent failure mode and permits writing to Program Files installs.",
      },
      {
        q: "How do the community .smmpack starter packs work?",
        a: "Import any .smmpack from SMM's Mod Packs page to stage its mods and register a one-click preset without losing your existing mods. Every community mod remains owned by its original author.",
      },
      {
        q: "How do I return Sekiro to a completely vanilla state?",
        a: "Click Restore on the Launch Hub. SMM removes only the files and empty directories recorded in .smm_manifest.json, leaving original game files and saves untouched.",
      },
    ],
  },
  footer: {
    description: "Native C++17 / Qt 6 zero-copy desktop mod manager for Sekiro: Shadows Die Twice.",
    siteHeading: "Navigation",
    projectHeading: "Project & Docs",
    project: "GitHub Repository",
    releases: "Releases",
    docs: "Documentation",
    blog: "Author's Blog",
    license: "MIT License",
    notices: "Third-Party Notices",
    webLicenses: "Website Dependency Licenses",
    disclaimer:
      "Independent open-source project not affiliated with FromSoftware, Inc. or Activision. Third-party mods belong to their respective creators.",
  },
} as const;
