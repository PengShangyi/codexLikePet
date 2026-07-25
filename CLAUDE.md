# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Potato is a local-first macOS menu-bar desktop pet for Apple Silicon (macOS 12+),
written in C++20 with Qt 6.8-compatible APIs and narrowly-scoped Objective-C++
(`.mm`) adapters for macOS-only services. It renders one transparent pet window on
the primary display with drag/edge-snap/click animations, seasonal + day/night
resource variants, reduced-motion support, and opt-in activity-only typing detection.

Hard product/privacy boundaries (see `docs/PROJECT.md`, the authoritative contract):
no task monitoring, no telemetry, no network access in release builds, no persistent
logs, no weather/location, no autonomous roaming, no multi-display behavior. Keyboard
monitoring is opt-in and keeps only a rolling window of monotonic timestamps in
memory — key values must never reach models, logs, settings, tests, or files.

## Build, test, run

```sh
cmake -S . -B build -DPOTATO_BUILD_TESTS=ON   # tests ON by default
cmake --build build
ctest --test-dir build --output-on-failure
open -n build/Potato.app                       # local dev app
```

Run one test by CTest name (each test target is a separate executable):

```sh
ctest --test-dir build -R potato_behavior_test --output-on-failure
./build/tests/potato_behavior_test             # or run the binary directly
```

Set `POTATO_QT_ROOT` if Qt is not on the default CMake prefix path. Builds are
arm64-only and out-of-source; generated files are never committed.

## Release packaging (requires licensed Qt 6.8.8)

`scripts/package-local.sh` is the acceptance path: it rejects any Qt version other
than 6.8.8, runs the full test suite, deploys Qt frameworks via `macdeployqt`, copies
licenses, ad-hoc signs, and verifies an arm64-only bundle. Configure with
`-DPOTATO_REQUIRE_EXACT_QT=ON` to enforce the exact Qt pin. Local iteration with a
newer Qt 6 is fine but is **not** the release artifact.

`scripts/check-runtime.sh /abs/path/to/Potato.app` verifies idle limits — it fails at
2% CPU, 160 MiB resident memory, or any open network socket.

## Architecture

`AppController` (`src/app/`) owns the application lifecycle and is the single wiring
point: it constructs and connects otherwise-isolated services, the tray menu, and the
pet window. Platform adapters never own settings or pet resources — they are injected.

The design centers on **single-authority services** with injectable boundaries so each
has a deterministic test double. Rendering code receives already-resolved state and
never invents transitions:

- **`BehaviorController`** (`src/pet/`) — sole behavior-priority authority. Decides
  which `BehaviorState` is active; `PetWindow`/`AnimationPlayer` just render it.
- **`EnvironmentResolver`** (`src/environment/`) — sole season/day-night authority,
  driven by an injectable `EnvironmentClock`, with fallback logic when a variant is
  missing. Produces the variant key (e.g. `spring-day`) used to pick atlas/clips.
- **`AnimationPlayer`** (`src/pet/`) — plays either a Codex v2 atlas row **or** one
  validated Potato clip. Reduced motion freezes either source on its first frame.
- **Resource pipeline** (`src/resources/`) — `PetPackageValidator` owns contract
  validation, `ArchiveExtractor` owns hostile-archive boundaries (zip via vendored
  miniz), `PetStore` owns staged atomic install; `PetLibrary` lists pets and
  `PetPackageImporter` drives import. Imported pets live in
  `~/Library/Application Support/Potato/Pets`, outside the repo.
- **Input** (`src/input/`) — `InputActivitySource` is the boundary
  (`MacInputActivitySource.mm`); `TypingActivityDetector` derives activity frequency
  from timestamps only.
- **Boundaries with test doubles**: `InputActivitySource`, `SystemActivitySource`
  (`src/platform/`), `LoginItemController` (`src/login/`, macOS Service Management —
  no custom LaunchAgent), and `QuoteProvider` (`src/quotes/`, local-only).

Runtime resource discipline (enforced by design): decode lazily, retain at most two
atlas cache entries (`AtlasCache`), clear extension clips on environment/pet change,
stop pet/environment/input timers while hidden or asleep, recompute primary-screen
placement (`WindowPlacement`) after display change and wake.

## Pet resource contract

Codex v2 pet atlases (`potato.json` manifest + spritesheet) are the base contract.
Potato-specific extensions — `variants` (season/day-night spritesheets) and
`variantClips` (per-variant edge animations with `durationsMs`) — live **beside**, not
inside, the v2-compatible fields. Built-in assets are under `assets/Pets/`, tracked via
Git LFS, and copied into the app bundle's `Contents/Resources/Pets` at build time.

Authoring workflow, validation rules, and the bundled Apache-2.0 Hatch Pet skill are
documented in `docs/PET_AUTHORING.md`. The bundled skill is read-only app content and
is never installed into `~/.codex`.

## Conventions

- Objective-C++ (`.mm`) is used only for macOS system adapters; keep Qt/C++ logic out
  of platform files and behind the C++ boundary interface (e.g. `*ActivitySource.h`).
- Every service that touches a system boundary has a pure-C++ interface header plus a
  `Mac*` implementation, so tests link the logic without the system dependency. Follow
  this pattern when adding new platform integration.
- Each test in `tests/CMakeLists.txt` is its own `qt_add_executable` linking only the
  sources under test — add new tests the same way rather than into a shared binary.
- Warnings are errors-adjacent: everything builds with `-Wall -Wextra -Wpedantic`.
