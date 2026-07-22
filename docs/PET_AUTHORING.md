# Authoring pets for Potato

Potato accepts an ordinary Codex v2 pet and adds optional, separate resources
for seasons, day/night appearances, click/typing overrides, and edge poses.
The original `pet.json` and `spritesheet.webp` remain Codex-compatible.

## Use the bundled Hatch Pet skill

The unmodified skill is available at `third_party/hatch-pet` in the repository
and at `Potato.app/Contents/Resources/Skills/hatch-pet` in a built app. To use it
with Codex, copy that directory to `${CODEX_HOME:-$HOME/.codex}/skills/hatch-pet`
and restart or reload Codex skills. Potato never performs this copy itself.

Follow the skill's `SKILL.md` workflow and all of its deterministic and visual
QA gates. A newly created base atlas must be transparent WebP or PNG, exactly
1536×2288, and use `spriteVersionNumber: 2`.

## Directory layout

```text
my-pet/
├── pet.json
├── potato.json                 # optional Potato extensions
├── spritesheet.webp            # default or one mapped environment
├── variants/
│   ├── spring-night.webp
│   └── winter-day.webp
└── clips/
    ├── edge-left.webp
    ├── edge-right.webp
    └── edge-bottom.webp
```

`pet.json` keeps the Codex contract:

```json
{
  "id": "my-pet",
  "displayName": "My Pet",
  "description": "A short description.",
  "spriteVersionNumber": 2,
  "spritesheetPath": "spritesheet.webp"
}
```

`potato.json` schema version 1 adds explicit relative paths:

```json
{
  "schemaVersion": 1,
  "renderMode": "smooth",
  "variants": {
    "spring-day": "spritesheet.webp",
    "spring-night": "variants/spring-night.webp",
    "winter-day": "variants/winter-day.webp"
  },
  "clips": {
    "edge-left": {
      "path": "clips/edge-left.webp",
      "durationsMs": [150, 150, 240]
    }
  },
  "variantClips": {
    "winter-day": {
      "edge-left": {
        "path": "variants/winter-day-edge-left.webp",
        "durationsMs": [150, 150, 240]
      }
    }
  }
}
```

Valid variant keys are `spring`, `summer`, `autumn`, `winter`, `day`, `night`,
and each season combined with `day` or `night`. Potato selects a combined key,
then its season, then its phase, and finally the base spritesheet.

Valid clip keys are `click`, `typing`, `edge-left`, `edge-right`, and
`edge-bottom`. A clip is one transparent horizontal strip, 208 pixels high and
between one and eight 192-pixel-wide frames. It must provide one duration per
frame, each between 50 and 2000 milliseconds. The first frame is also the
representative still used when reduced motion is enabled. For edge art,
`edge-left` and `edge-right` should visibly lean or peek inward from their named
screen boundary; `edge-bottom` should read as a planted or prone bottom-edge
pose.

When an override is absent, Potato uses the Codex v2 `waving` row for clicks,
the `running` row for typing, and the 090°, 270°, or 000° look cell for the
left, right, or bottom edge respectively.

## Packaging and privacy

Import the directory directly during development, or ZIP its contents with
`pet.json` at the archive root and rename the archive to `.potatopet`. Archives
must contain only JSON, PNG, WebP, Markdown, text, and license files. Potato
rejects links, executable files, path traversal, oversized archives, and invalid
atlases before copying anything to Application Support. Manifest JSON is capped
at 1 MiB. Image files must be referenced by `pet.json` or `potato.json`; standard
README, notice, and license text may remain unreferenced, but arbitrary extra
files are rejected. Paths are portable relative paths and therefore cannot use
backslashes, drive prefixes, control characters, or absolute locations.

Local imports, Hatch Pet generation runs, QA media, decoded rows, and previews
belong outside Git or under the repository's ignored `local/`, `pet-runs/`, and
`qa/` directories. Only reviewed built-in release assets belong in Git LFS.
