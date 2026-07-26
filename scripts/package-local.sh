#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
QT_VERSION="6.8.8"
QT_ROOT="${POTATO_QT_ROOT:-$ROOT_DIR/.tools/Qt/$QT_VERSION/macos}"
QT_LICENSES="${POTATO_QT_LICENSES:-}"
BUILD_DIR="${POTATO_BUILD_DIR:-$ROOT_DIR/build-release-$QT_VERSION}"
DIST_DIR="${POTATO_DIST_DIR:-$ROOT_DIR/dist}"
# Bounded deliberately. `cmake --build --parallel` with no number hands Make a
# bare -j, which places no limit at all on concurrent jobs -- and this script
# always builds from scratch with -O2 and links every target with LTO, so an
# unbounded wave oversubscribes memory and lands slower than a capped one while
# pinning every core.
JOBS="${POTATO_JOBS:-$(sysctl -n hw.ncpu)}"

if [[ "$(uname -m)" != "arm64" ]]; then
    print -u2 "Potato packaging requires an Apple Silicon Mac."
    exit 1
fi
if [[ ! -x "$QT_ROOT/bin/qmake" || ! -x "$QT_ROOT/bin/macdeployqt" ]]; then
    print -u2 "Qt tools were not found under $QT_ROOT"
    print -u2 "Set POTATO_QT_ROOT to a Qt $QT_VERSION macOS installation."
    exit 1
fi

ACTUAL_QT_VERSION="$($QT_ROOT/bin/qmake -query QT_VERSION)"
if [[ "$ACTUAL_QT_VERSION" != "$QT_VERSION" ]]; then
    print -u2 "Expected Qt $QT_VERSION but found $ACTUAL_QT_VERSION at $QT_ROOT"
    exit 1
fi

cmake -E remove_directory "$BUILD_DIR"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$QT_ROOT" \
    -DPOTATO_REQUIRE_EXACT_QT=ON \
    -DPOTATO_BUILD_TESTS=ON
cmake --build "$BUILD_DIR" --parallel "$JOBS"
# Serial on purpose, unlike the `check` target developers use: this is the
# release gate, and parallel load is the one thing that can perturb the suite's
# qWait/QTRY_* timing assertions. It costs seconds against a multi-minute LTO
# build.
ctest --test-dir "$BUILD_DIR" --output-on-failure

APP="$BUILD_DIR/Potato.app"
"$QT_ROOT/bin/macdeployqt" "$APP" -always-overwrite

for required in \
    "$APP/Contents/Library/LoginItems/PotatoLoginHelper.app/Contents/MacOS/PotatoLoginHelper" \
    "$APP/Contents/Resources/Pets/potato/pet.json" \
    "$APP/Contents/Resources/Pets/potato/potato.json" \
    "$APP/Contents/Resources/Skills/hatch-pet/SKILL.md" \
    "$APP/Contents/Resources/Licenses/miniz/LICENSE" \
    "$APP/Contents/Resources/Licenses/hatch-pet/LICENSE.txt" \
    "$APP/Contents/Resources/THIRD_PARTY_NOTICES.md"; do
    if [[ ! -f "$required" ]]; then
        print -u2 "Required bundle resource is missing: $required"
        exit 1
    fi
done

if ! find "$APP/Contents/PlugIns/imageformats" -maxdepth 1 -type f \
        -iname '*webp*.dylib' -print -quit 2>/dev/null | grep -q .; then
    print -u2 "macdeployqt did not deploy Potato's required WebP image plugin."
    exit 1
fi
if [[ "$(plutil -extract LSUIElement raw -o - "$APP/Contents/Info.plist")" != "true" ]]; then
    print -u2 "The packaged app is not configured as a Dock-less menu-bar app."
    exit 1
fi
if [[ "$(plutil -extract LSMinimumSystemVersion raw -o - "$APP/Contents/Info.plist")" != "12.0" ]]; then
    print -u2 "The packaged app does not declare macOS 12.0 as its minimum version."
    exit 1
fi

if [[ -z "$QT_LICENSES" ]]; then
    for candidate in \
        "$QT_ROOT/LICENSES" \
        "$QT_ROOT/../LICENSES" \
        "$QT_ROOT/../../LICENSES" \
        "$QT_ROOT/../../Docs/Qt-$QT_VERSION/licenses" \
        "$QT_ROOT/../../Docs/Qt-$QT_VERSION/LICENSES"; do
        if [[ -d "$candidate" ]]; then
            QT_LICENSES="$candidate"
            break
        fi
    done
fi
mkdir -p "$APP/Contents/Resources/Licenses/Qt"
if [[ -d "$QT_LICENSES" ]]; then
    ditto "$QT_LICENSES" "$APP/Contents/Resources/Licenses/Qt"
else
    print -u2 "Qt license files were not found beside $QT_ROOT"
    print -u2 "Set POTATO_QT_LICENSES to the matching Qt license directory."
    exit 1
fi

# Official Qt packages may contain universal frameworks. Potato intentionally
# ships arm64-only, so thin every deployed Mach-O before the final signature.
while IFS= read -r -d '' candidate; do
    if ! file -b "$candidate" | grep -q 'Mach-O'; then
        continue
    fi
    ARCHS="$(lipo -archs "$candidate")"
    if [[ " $ARCHS " != *" arm64 "* ]]; then
        print -u2 "Mach-O file has no arm64 slice: $candidate ($ARCHS)"
        exit 1
    fi
    if [[ "$ARCHS" != "arm64" ]]; then
        MODE="$(stat -f '%Lp' "$candidate")"
        lipo "$candidate" -thin arm64 -output "$candidate.potato-thin"
        chmod "$MODE" "$candidate.potato-thin"
        mv "$candidate.potato-thin" "$candidate"
    fi
    for minimum in ${(f)$(vtool -show-build "$candidate" 2>/dev/null \
            | awk '$1 == "minos" { print $2 }')}; do
        if awk -v version="$minimum" 'BEGIN {
            split(version, part, ".");
            exit !((part[1] + 0) > 12 || ((part[1] + 0) == 12 && (part[2] + 0) > 0));
        }'; then
            print -u2 "Mach-O requires macOS $minimum instead of 12.0: $candidate"
            exit 1
        fi
    done
done < <(find "$APP" -type f -print0)

codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict --verbose=2 "$APP"

while IFS= read -r -d '' candidate; do
    if ! file -b "$candidate" | grep -q 'Mach-O'; then
        continue
    fi
    ARCHS="$(lipo -archs "$candidate")"
    if [[ "$ARCHS" != "arm64" ]]; then
        print -u2 "Expected arm64-only Mach-O; found $ARCHS in $candidate"
        exit 1
    fi
    if otool -L "$candidate" \
            | grep -E '/Users/|/private/(tmp|var/folders)|/(\.tools|opt/homebrew|usr/local)/' \
                >/dev/null; then
        print -u2 "Development-only dynamic library path remains in $candidate"
        exit 1
    fi
done < <(find "$APP" -type f -print0)

if find "$APP/Contents/Resources/Skills/hatch-pet" \
        \( -type d -name '__pycache__' -o -type f \( -name '*.pyc' -o -name '*.pyo' \) \) \
        -print -quit | grep -q .; then
    print -u2 "Generated Python cache leaked into the bundled Hatch Pet skill."
    exit 1
fi

mkdir -p "$DIST_DIR"
cmake -E remove_directory "$DIST_DIR/Potato.app"
ditto "$APP" "$DIST_DIR/Potato.app"
print "$DIST_DIR/Potato.app"
