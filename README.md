# Potato

Potato is a lightweight, local-first macOS desktop pet for Apple Silicon.

It lives in the menu bar and provides a transparent primary-screen pet window,
directional drag animation, three-edge poses, click reactions, seasonal and
day/night resources, local bilingual click messages, optional activity-only
typing animation, and reduced-motion support. It does not monitor tasks, roam
on its own, use location or weather, or make network requests.

Architecture and privacy boundaries are recorded in `docs/PROJECT.md`.

## Development requirements

- Apple Silicon Mac running macOS 12 or newer
- Apple Clang with the macOS SDK
- CMake 3.24 or newer
- Qt 6.8.x for compatibility development; the acceptance packager pins 6.8.8
- Git LFS for checking out reviewed built-in image assets

Configure and test out of source:

```sh
cmake -S . -B build -DPOTATO_BUILD_TESTS=ON
cmake --build build -j 10                      # your core count, explicitly
cmake --build build --target check             # ctest across the cores
```

Always pass an explicit `-j <n>`. A bare `cmake --build -j` means *unlimited*
jobs under Make, which oversubscribes the machine badly enough that a clean
build takes several times longer than a capped one. The `check` target runs
CTest in parallel; `ctest --test-dir build --output-on-failure` still works when
you want it serial, and `-R <name>` selects one test.

Set `POTATO_QT_ROOT` when Qt is not installed in a standard CMake prefix.

The local development app is `build/Potato.app`. Launch it through Finder or:

```sh
open -n build/Potato.app
```

Qt 6.8.8 is a [commercial LTS patch](https://www.qt.io/blog/commercial-lts-qt-6.8.8-released)
and requires a valid Qt commercial account.
For the release-compatible local bundle, install it under
`.tools/Qt/6.8.8/macos` or point `POTATO_QT_ROOT` at an existing licensed
installation, then run `scripts/package-local.sh`. The script rejects any Qt
version mismatch, runs the complete test suite, deploys dynamic Qt frameworks,
copies licenses, creates an ad-hoc signature, and verifies an arm64-only app.
If the licensed installer keeps Qt notices elsewhere, point
`POTATO_QT_LICENSES` at that matching license directory.
Developers without that entitlement can continue local iteration with the
public Qt 6.8.4 sources used by this workspace, or another compatible Qt 6
release, but that output is not the Qt 6.8.8 acceptance artifact.

After packaging, verify the idle resource limits and network boundary with:

```sh
scripts/check-runtime.sh /absolute/path/to/Potato.app
```

It runs two phases — shipped defaults, and both appearance sliders at maximum —
and fails at 2% idle CPU on the defaults, 64 MiB peak physical footprint, or any
open network socket. CPU is a delta over a fixed window rather than `ps -o %cpu`,
which is a lifetime average and so flatters any build the longer it runs.
Footprint rather than resident size, because most of the reported RSS is Qt
framework text shared with every other Qt process. It uses and removes a
dedicated runtime-check preference/data profile, and does not inspect local pets
or modify the user's launch-at-login registration.
DMG creation, Developer ID signing, and notarization remain a later release
phase; the current deliverable is an ad-hoc-signed local `.app`.

The workspace's public-Qt compatibility acceptance produced the ignored local
artifact `dist/Potato.app` with source-built Qt 6.8.4. All 15 test executables
passed; the deployed bundle contains 21 arm64-only Mach-O files with a maximum
declared minimum OS of 12.0, a valid ad-hoc signature, WebP support, and no
development library paths. The runtime probe measured 1.42% idle CPU, 114944
KiB peak resident memory, and zero network sockets on the current Apple Silicon
development Mac. This validates local development, not the licensed Qt 6.8.8
release gate or the later required macOS 12 compatibility-machine check.

## Privacy

- Typing detection is off by default and receives activity-only callbacks with
  no character, key code, modifier, or foreground-application payload.
- Pet imports remain under `~/Library/Application Support/Potato/Pets`.
- Local pet sources under `local/pets/`, image-generation runs, QA media,
  imports, builds, and release output are ignored by Git.
- Release code has no online provider, telemetry, persistent application log,
  weather, location, or remote update path.

## Creating pet resources

The repository and application bundle contain an unmodified copy of the
Apache-2.0 Hatch Pet skill. See `docs/PET_AUTHORING.md` for Codex v2 creation,
Potato variants, edge animations, validation, and local privacy rules.

Only reviewed built-in Potato assets are tracked, through Git LFS. The bundled
skill is read-only application content and is never installed into `~/.codex`
automatically.
