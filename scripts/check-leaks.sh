#!/bin/zsh
set -euo pipefail

# Leak gate for the test binaries.
#
# Why leaks(1) and not LeakSanitizer: LSan is not supported on Darwin/arm64, so
# -fsanitize=address gives us use-after-free and overflow detection but no leak
# reporting at all. The platform's own leaks(1) is the only leak checker that
# works here. POTATO_SANITIZE=ON covers the other half; the two are complementary
# and this script deliberately runs against a NON-sanitized build, because ASan
# replaces the allocator that leaks(1) inspects.
#
# The recorded baseline is currently zero for every binary -- Qt turned out to
# clean up after itself at exit here, so there is no tolerated residue to work
# around. The per-binary ceiling in scripts/leak-baseline.txt therefore exists as
# a safety valve, not a concession: if some future Qt or macOS version starts
# reporting unavoidable noise, it can be recorded rather than disabling the gate.
#
# Re-record after an intentional ownership change:
#     scripts/check-leaks.sh --update
# Any non-zero entry appearing in that file needs a reason in the commit message.
# Treat a silent rise as the bug it almost certainly is.

ROOT_DIR="${0:a:h:h}"
BASELINE="$ROOT_DIR/scripts/leak-baseline.txt"
BUILD_DIR="$ROOT_DIR/build"
UPDATE="false"

while (( $# > 0 )); do
    case "$1" in
        --update) UPDATE="true"; shift ;;
        -h|--help)
            print "usage: $0 [--update] [build-dir]"
            exit 0 ;;
        *) BUILD_DIR="${1:a}"; shift ;;
    esac
done

TEST_DIR="$BUILD_DIR/tests"
if [[ ! -d "$TEST_DIR" ]]; then
    print -u2 "No test directory at $TEST_DIR -- configure and build first."
    exit 1
fi

# leaks(1) inspects the default malloc zone; ASan replaces it and the report
# becomes meaningless. Refuse rather than print a reassuring wrong answer.
if grep -q "POTATO_SANITIZE:BOOL=ON" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
    print -u2 "$BUILD_DIR is a sanitizer build; leaks(1) cannot inspect ASan's allocator."
    print -u2 "Use a plain build directory for the leak gate."
    exit 1
fi

typeset -a BINARIES
for candidate in "$TEST_DIR"/*(N.x); do
    BINARIES+=("${candidate:t}")
done
if (( ${#BINARIES} == 0 )); then
    print -u2 "No test executables found in $TEST_DIR."
    exit 1
fi

# Offscreen for the widget tests; harmless for the guiless ones.
export QT_QPA_PLATFORM=offscreen

typeset -A RECORDED
if [[ -f "$BASELINE" ]]; then
    while read -r name count; do
        [[ -z "$name" || "$name" == \#* ]] && continue
        RECORDED[$name]="$count"
    done < "$BASELINE"
fi

STATUS=0
typeset -a MEASURED
for binary in ${(o)BINARIES}; do
    # MallocStackLogging makes the report name the allocation site, which is the
    # whole point when a count does rise. Scoped to this one command rather than
    # exported: the shell helpers in this pipeline inherit the environment too,
    # and each one prints its own several-line MSL preamble to stderr.
    #
    # leaks --atExit exits non-zero when it finds anything, so the count is parsed
    # out rather than trusted to the exit code.
    REPORT="$(MallocStackLogging=1 leaks --atExit -- "$TEST_DIR/$binary" 2>/dev/null || true)"
    COUNT="$(print -r -- "$REPORT" | sed -n 's/.*: \([0-9][0-9]*\) leaks* for .*/\1/p' | tail -1)"
    if [[ -z "$COUNT" ]]; then
        print -u2 "$binary: could not parse a leak count from leaks(1)."
        STATUS=1
        continue
    fi
    MEASURED+=("$binary $COUNT")

    if [[ "$UPDATE" == "true" ]]; then
        print "$binary: $COUNT (recorded)"
        continue
    fi

    ALLOWED="${RECORDED[$binary]:-}"
    if [[ -z "$ALLOWED" ]]; then
        print -u2 "$binary: $COUNT leaks, no baseline recorded. Run $0 --update."
        STATUS=1
    elif (( COUNT > ALLOWED )); then
        print -u2 "$binary: $COUNT leaks, baseline $ALLOWED -- regression."
        print -u2 "$REPORT"
        STATUS=1
    else
        print "$binary: $COUNT leaks (baseline $ALLOWED)"
    fi
done

if [[ "$UPDATE" == "true" ]]; then
    {
        print "# Recorded by scripts/check-leaks.sh --update."
        print "# Ceilings, not targets. Every binary reports zero today; a non-zero entry"
        print "# needs a reason in the commit message that introduced it."
        for entry in ${(o)MEASURED}; do print -r -- "$entry"; done
    } > "$BASELINE"
    print "Wrote $BASELINE"
fi

exit $STATUS
