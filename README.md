# Potato

Potato is a lightweight, local-first macOS desktop pet for Apple Silicon.

The project is under active development. Its architecture, privacy boundaries,
and resource compatibility contract are recorded in `docs/PROJECT.md`.

## Development requirements

- Apple Silicon Mac running macOS 12 or newer
- Apple Clang with the macOS SDK
- CMake 3.24 or newer
- Qt 6.8.x for release compatibility (newer Qt may be used for local iteration)

Configure and test out of source:

```sh
cmake -S . -B build -DPOTATO_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Set `POTATO_QT_ROOT` when Qt is not installed in a standard CMake prefix.
