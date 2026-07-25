# Potato project contract

Potato is a menu-bar macOS desktop pet for Apple Silicon and macOS 12 or newer.
The application is implemented with C++20, Qt 6.8-compatible APIs, and narrowly
scoped Objective-C++ adapters for macOS-only services.

## Product boundaries

- One transparent pet window on the primary display only.
- Dragging, left/right/bottom snapping, contextual animation, click reactions,
  local season/day variants, reduced motion, and optional activity-only typing
  detection.
- Codex v2 pet atlases remain the base resource contract. Potato extensions live
  beside, rather than inside, the compatible manifest and atlas.
- No task monitoring, telemetry, online quote provider, weather, location,
  autonomous roaming, click-through mode, or multi-display behavior in v1.

## Privacy and filesystem rules

- Imported pets belong in the user's Application Support directory, outside the
  repository. Development imports under `local/` are ignored.
- Keyboard monitoring is opt-in and retains only a short rolling window of
  monotonic activity timestamps in memory, from which activity frequency is
  derived. Key values must never enter models, logs, settings, tests, or files.
- Release builds do not write persistent logs and do not initiate network access.
- Generated image runs and QA artifacts stay ignored; only approved built-in
  release assets are tracked through Git LFS.

## Engineering rules

- Build out of source. Generated files are never committed.
- Third-party code is vendored in-tree under `third_party/`, never fetched at
  configure time: a clean build must succeed with no network access.
- Keep runtime dependencies limited to Qt and explicitly vendored, licensed
  components. Python is tooling-only and is not embedded in the app.
- Every milestone includes focused automated tests and one cohesive commit.
- The first release artifact is a local arm64 app bundle. Signing, notarization,
  and DMG publication are a later release gate.

## Runtime ownership

- `AppController` owns application lifecycle and connects otherwise isolated
  services; platform adapters do not own settings or pet resources.
- `BehaviorController` is the sole behavior-priority authority. Rendering code
  receives a resolved state and never invents transitions.
- `EnvironmentResolver` is the sole season/day-night fallback authority and is
  driven through an injectable `EnvironmentClock`.
- `AnimationPlayer` accepts either a v2 atlas row or one validated Potato clip.
  Reduced motion freezes either source on its representative first frame.
- `PetPackageValidator`, `ArchiveExtractor`, and `PetStore` respectively own
  contract validation, hostile archive boundaries, and staged atomic install.
  The filesystem safety rules they share live once in `PackagePolicy`, and the
  clip geometry and duration bounds live once in `ClipContract`; neither rule set
  may be restated at a second call site.
- `InputActivitySource`, `SystemActivitySource`, `LoginItemController`, and
  `QuoteProvider` are replaceable boundaries with deterministic test doubles.

The application decodes resources lazily, retains at most two atlas cache
entries and a bounded clip cache, clears extension clips on environment or pet
changes, stops pet, environment, and input timers while hidden or asleep, and
recomputes primary screen placement after display changes and wake.

Pixel-level package validation happens at the trust boundary — import, and the
recheck of the staged copy — never on the launch path. Listing installed pets
uses the metadata depth, which still enforces every safety rule (directory
envelope, symbolic links, executable bits, path safety, unreferenced files, clip
geometry read from image headers) and omits only the decode. A resource that
fails to decode is reported when it is loaded.

## Persistent locations

- Preferences: native `QSettings` domain for `com.peng.Potato`.
- Imported pets: `~/Library/Application Support/Potato/Pets`.
- Login state: Apple Service Management only; no custom LaunchAgent.
- Repository-local tools: `.tools/` (ignored).
- Built-in pets and bundled authoring skill: read-only app resources.
