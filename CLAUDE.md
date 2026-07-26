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
cmake --build build -j 10
cmake --build build --target check             # ctest across the cores; ~4s
open -n build/Potato.app                       # local dev app
```

Pass an explicit `-j <n>`. `cmake --build -j` with no number means *unlimited*
jobs under Make, which oversubscribes the machine badly enough to make a clean
build several times slower and to make timings meaningless.

`CMAKE_BUILD_TYPE` defaults to `RelWithDebInfo` when you do not choose one, so a
dev build is worth profiling. Release stays the packaging path.

Run one test by CTest name:

```sh
ctest --test-dir build -R potato_behavior_test --output-on-failure
```

Most test classes share a binary with the others in their link group, so the
CTest name and the executable name are not the same thing. To run one class
directly, name it:

```sh
./build/tests/potato_core_test --class SettingsTest   # one class
./build/tests/potato_core_test                        # every class in the binary
```

Set `POTATO_QT_ROOT` if Qt is not on the default CMake prefix path. Builds are
arm64-only and out-of-source; generated files are never committed.

## Memory lifetime

Two gates, because neither covers the other half:

```sh
cmake -S . -B build-asan -DPOTATO_SANITIZE=ON && ctest --test-dir build-asan
scripts/check-leaks.sh                          # must stay at zero
```

`POTATO_SANITIZE=ON` builds with ASan and UBSan and catches use-after-free — the
failure mode for the frame views described under Conventions. Note its reach:
Qt is a prebuilt uninstrumented framework, so a dangling read performed *inside*
`QImage::pixelColor` is invisible to it. A lifetime test has to touch the buffer
from our own code to be checked.

`scripts/check-leaks.sh` drives the platform's `leaks(1)`, because LeakSanitizer
is not supported on Darwin/arm64. Every binary records zero in
`scripts/leak-baseline.txt`; a non-zero entry needs a reason in the commit that
introduces it. Do not run it against a sanitizer build — ASan replaces the
allocator it inspects, and the script refuses.

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

The settings, about, and welcome windows are built on **first use**, not at startup;
most sessions never open any of them. `SettingsViewState` (`src/app/`) absorbs the
state `AppController` pushes at the settings window in the meantime and replays it on
attach, so the call sites stay free of null checks. It deliberately records no image
handles — a proxy that cached a preview atlas would pin 14 MB to avoid building some
widgets, which is the point inverted — so `settingsWindow()` re-pushes those from the
package and atlas the controller already owns.

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
  `TypingAnimationDriver` sits beside it and owns the keystroke-driven typing
  animation (hold the clip at rest, advance one press per key, relax after a pause).
- **Resource pipeline** (`src/resources/`) — `PetPackageValidator` owns contract
  validation, `ArchiveExtractor` owns hostile-archive boundaries (zip via vendored
  miniz), `PetStore` owns staged atomic install; `PetLibrary` lists pets and
  `PetPackageImporter` drives import. Imported pets live in
  `~/Library/Application Support/Potato/Pets`, outside the repo.
- **Shared rule modules** — `PackagePolicy` (`src/resources/`) holds the filesystem
  safety rules (portable relative paths, root escape, file-type allowlist, pet id)
  and `ClipContract` (`src/pet/`) holds the clip geometry and duration bounds. Both
  the validator and the runtime loaders call these rather than restating them; a
  rule must never be written out twice.
- **`Theme` + the settings kit** (`src/ui/`) — `Theme` is the single home for every
  color, metric, and font, and `Theme::styleSheet()` is the only `setStyleSheet`
  call site in the app. It defines light and dark outright instead of deriving from
  `QGuiApplication::palette()`, because `QStyleHints::colorSchemeChanged` fires
  *before* Qt swaps that palette — deriving from it painted dark text on dark cards.
  `SettingsCard`/`SettingsRow`/`SettingsPage`/`ValueSlider`/`InlineBanner`/
  `Disclosure` compose the settings pages; a `SettingsRow` structurally requires a
  `TextKey`, so no row can exist without a translatable label or an accessible name.
  The stylesheet styles **containers only** — every native control (`QComboBox`,
  `QSlider`, `QCheckBox`, `QTimeEdit`, `QPushButton`) is deliberately left unstyled
  so `QMacStyle` keeps drawing it and it follows `NSAppearance` for free. Every
  selector is qualified, because `QFileDialog`/`QMessageBox` are parented to the
  settings window and inherit its sheet.
- **Input** (`src/input/`) — `InputActivitySource` is the boundary
  (`MacInputActivitySource.mm`); `TypingActivityDetector` derives activity frequency
  from timestamps only.
- **Boundaries with test doubles**: `InputActivitySource`, `SystemActivitySource`
  (`src/platform/`), `LoginItemController` (`src/login/`, macOS Service Management —
  no custom LaunchAgent), and `QuoteProvider` (`src/quotes/`, local-only).

Runtime resource discipline (enforced by design): decode lazily, retain at most two
atlas cache entries (`AtlasCache`) and a bounded clip cache (`ClipCache`), clear
extension clips on environment/pet change, stop pet/environment/input timers while
hidden or asleep, recompute primary-screen placement (`WindowPlacement`) after display
change and wake, build the settings/about/welcome windows only when asked for, and hand
out frames as views rather than copies (see Conventions).

**Validation depth.** Decoding every atlas and clip is the entire cost of package
validation: for the built-in pet, 23 ms per atlas against 0.18 ms for the occupancy
scan that follows one. It runs at the trust boundary only — `PetPackageImporter` and
`PetStore`'s post-copy recheck use `ValidationDepth::Full`; `PetLibrary` lists with
`ValidationDepth::Metadata`. Both depths enforce every *safety* rule (directory
envelope, symlinks, executable bits, path safety, unreferenced files, clip geometry
from image headers); Full adds only the pixel-level checks. Anything that slips past
listing still surfaces at load through `PetAtlas`/`AnimationClip::load`. Do not move
deep validation back onto the launch path.

Those pixel checks are *recorded* by the sequential walk and dispatched together at the
end, on a local `QThreadPool` — the files are independent and read-only, and import
pays for the whole set twice (source, then the post-copy recheck). Issues are appended
in walk order, not completion order, so which decode finishes first cannot change the
report. Whether a check runs at all is structural: the walk is handed a sink to record
into, and at `Metadata` depth there is nowhere to put a decode job.

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
- Test *binaries* are grouped by link set; test *entries* are not. Classes sharing a
  link set share an executable (`potato_core_test`, `potato_geometry_test`,
  `potato_ui_test`, `potato_package_test`, `potato_animation_test`), but each keeps its
  own CTest entry dispatched with `--class`, so filtering and `ctest -j` parallelism are
  per class. A new test joins the group whose link set it already needs: define the
  class in its own file, end it with a `QObject *create<Class>()` factory instead of
  `QTEST_MAIN`, and register it in the group's `tests/groups/*_main.cpp` and its
  `potato_add_grouped_tests` list. Only give a test its own binary when it needs
  something no group has — `potato_pet_overlay_test` is separate because it must *not*
  run offscreen.
- Low-level modules come from the shared static libraries in the root `CMakeLists.txt`
  (`potato_settings_core`, `potato_theme`, `potato_ui`, `potato_environment`,
  `potato_geometry`, `potato_atlas`, `potato_policy`, `potato_package`,
  `potato_overlay`, `potato_app`); link those instead of relisting their sources, and
  list any other source directly. `potato_theme` is Gui-only so the tray icon, the pet's
  fallback drawing, and the speech bubble can share the brand palette without
  acquiring a Widgets and `Localization` dependency; `potato_ui` adds the widgets.
  Modules that pull in a system framework stay out of the libraries so a test can still
  link the pure C++ half of a boundary on its own. `potato_app` is the one exception —
  it is the whole application layer including the `.mm` adapters, and it exists because
  `Potato` and `potato_app_controller_test` were compiling the same thirty sources
  twice. Those two are its only consumers; do not link it from a narrower test.
- Precompiled headers come in three tiers (`POTATO_PCH_CORE` / `_GUI` / `_WIDGETS`).
  Take the narrowest one a target can: handing `potato_theme` a Widgets header to
  precompile would give it a Widgets include path and dissolve the boundary above.
- Warnings are errors-adjacent: everything builds with `-Wall -Wextra -Wpedantic`.
- Settings live behind `AppSettings` — including the pet window position and the
  settings window geometry. Never reach for a bare `QSettings()`; that bypasses the
  injected store and makes tests write to real macOS preference domains.
- Colors, metrics, and fonts live in `Theme`. Do not call `setStyleSheet` outside
  `Theme::styleSheet()`, and do not add a QSS rule that matches `PetPreviewWidget` —
  a match makes `QStyleSheetStyle` set `WA_StyledBackground` on it, which paints a
  background before `paintEvent` and silently undoes its opaque-paint optimization.
- Prefer a distinct type over a `bool` for a new flag parameter next to existing
  numeric ones (see `PetGesture::DragLock`). Adding `bool` there compiled silently
  and reinterpreted every existing call, because `int` converts to `bool` without a
  warning.
- **Ownership.** QObjects use Qt parent/child; `std::unique_ptr` owns the top-level
  windows and the non-QObject services; `QSharedPointer` owns decoded atlases and
  clips, which are shared between the caches, the player, and the preview. There is
  no bare `delete` in the codebase and no reason to add one.
- **Frames are views, not copies.** Anything that runs per animation frame — or worse,
  per paint — takes `PetAtlas::frameView()` / `AnimationClip::frameView()`, which point
  into the owner's pixels. `frame()` still copies 156KB and is for callers that need an
  independent image. A view keeps its own pixels alive (it retains the owning `QImage`),
  so it survives cache eviction; writing to one detaches and gives the allocation back.
  Reverting a hot path to `frame()` is a regression, not a simplification.
- Atlases and clips are stored `ARGB32_Premultiplied`, the raster engine's native
  format. Storing anything else means `drawImage` converts on every paint.
- `PetWindow` caches the current frame pre-scaled to the window. Any new input to that
  scale — a new frame, a different filter — must invalidate `m_scaled`, or the pet
  freezes on one image while everything else reports that it is animating.
