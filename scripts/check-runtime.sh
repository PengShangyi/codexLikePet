#!/bin/zsh
set -euo pipefail

if (( $# != 1 )); then
    print -u2 "usage: $0 /absolute/path/to/Potato.app"
    exit 2
fi
APP_PARENT="$(cd "$(dirname "$1")" && pwd -P)"
APP="$APP_PARENT/$(basename "$1")"
EXECUTABLE="$APP/Contents/MacOS/Potato"
if [[ ! -x "$EXECUTABLE" ]]; then
    print -u2 "Potato executable not found: $EXECUTABLE"
    exit 1
fi

# Not com.peng.PotatoRuntimeCheck. main.cpp sets organizationDomain to "com.peng",
# which is already reverse-DNS, and Qt's macOS backend transforms it again -- so the
# real domain is com.com-peng.PotatoRuntimeCheck. The old name here deleted nothing,
# which meant every run of this script measured a profile still carrying the window
# position, onboarding flag and settings geometry left by the previous one.
DOMAIN="com.com-peng.PotatoRuntimeCheck"
SUPPORT_DIR="$HOME/Library/Application Support/PotatoRuntimeCheck"

# Seconds of steady state to average CPU over, after the launch burst has settled.
SETTLE_SECONDS=8
SAMPLE_SECONDS=10

# Peak physical footprint, not resident size. RSS counts shared Qt framework pages
# -- ~73MiB of the ~110MiB it reports is text shared with every other Qt process --
# so it neither reflects what Potato costs nor moves when Potato's own allocations
# do. Footprint is what Activity Monitor shows and what the OS bills the process.
# Peak rather than current, because the compressor steadily deflates the current
# figure, so the current number rewards a process for having been idle a while.
#
# Know what this does not see: the peak is set during launch, by Qt and AppKit
# coming up, not by anything Potato allocates afterwards. Making the atlas decode
# 13.6MiB cheaper moved it by nothing at all -- that showed up only in the max RSS
# of potato_animation_test, which does the decode and nothing else. Resource work
# that is not on the launch path needs its own measurement, not this gate.
FOOTPRINT_LIMIT_MIB=64

PID=""
cleanup() {
    if [[ -n "${PID:-}" ]]; then
        kill "$PID" 2>/dev/null || true
        for _ in {1..20}; do
            kill -0 "$PID" 2>/dev/null || break
            sleep 0.1
        done
    fi
    defaults delete "$DOMAIN" >/dev/null 2>&1 || true
    cmake -E remove_directory "$SUPPORT_DIR"
}
trap cleanup EXIT INT TERM

find_pid() {
    while read -r candidate command; do
        if [[ "$command" == "$EXECUTABLE" || "$command" == "$EXECUTABLE "* ]]; then
            print "$candidate"
        fi
    done < <(ps -axo pid=,command=)
}

if [[ -n "$(find_pid)" ]]; then
    print -u2 "Quit the existing Potato process before running this check."
    exit 1
fi

# Milliseconds of CPU the process has consumed since it started.
cpu_millis() {
    ps -p "$1" -o cputime= | awk -F'[:.]' '{ print (($1 * 60) + $2) * 1000 + $3 * 10 }'
}

# vmmap reports the footprint with a K/M/G suffix; normalise to MiB.
peak_footprint_mib() {
    vmmap -summary "$1" 2>/dev/null | awk '
        /Physical footprint \(peak\)/ {
            value = $4
            unit = substr(value, length(value))
            sub(/[KMG]$/, "", value)
            if (unit == "K") value /= 1024
            else if (unit == "G") value *= 1024
            printf "%.1f", value
        }'
}

# run_phase <label> <cpu-ceiling-percent> [key=value ...]
# Each phase starts from a deleted domain, seeds the settings it names, and leaves
# the domain deleted. Keys use QSettings' native-backend spelling, which flattens
# "appearance/scale" to "appearance.scale" -- a nested -dict-add write is never read.
run_phase() {
    local label="$1" cpu_ceiling="$2"
    shift 2

    defaults delete "$DOMAIN" >/dev/null 2>&1 || true
    local pair
    for pair in "$@"; do
        defaults write "$DOMAIN" "${pair%%=*}" -float "${pair##*=}"
    done

    open -n "$APP" --args --potato-runtime-check
    PID=""
    local ready="false"
    for _ in {1..50}; do
        PID="$(find_pid | tail -1)"
        if [[ -n "$PID" ]]; then
            local initial_rss="$(ps -p "$PID" -o rss= | tr -d ' ')"
            if [[ "$initial_rss" == <-> ]] && (( initial_rss > 1024 )); then
                ready="true"
                break
            fi
        fi
        sleep 0.1
    done
    if [[ "$ready" != "true" ]]; then
        print -u2 "Potato did not finish launching through Launch Services."
        exit 1
    fi

    sleep "$SETTLE_SECONDS"
    local state="$(ps -p "$PID" -o state= | tr -d ' ')"
    if [[ -z "$state" || "$state" == Z* ]]; then
        print -u2 "Potato exited during the runtime check."
        exit 1
    fi

    # CPU time consumed over a fixed window, not ps -o %cpu. That column is a
    # decayed average over the whole lifetime, so it falls towards zero the longer
    # a process lives: a build with an expensive launch and an idle steady state
    # scores better the longer you wait, and one measured minutes in scores ~0
    # whatever it is doing. A delta over a known window is the actual idle cost.
    local before="$(cpu_millis "$PID")"
    sleep "$SAMPLE_SECONDS"
    local after="$(cpu_millis "$PID")"
    local cpu="$(awk -v a="$before" -v b="$after" -v w="$SAMPLE_SECONDS" \
        'BEGIN { printf "%.2f", (b - a) / (w * 1000) * 100 }')"

    local footprint="$(peak_footprint_mib "$PID")"
    local rss="$(ps -p "$PID" -o rss= | awk '{ printf "%.1f", $1 / 1024 }')"
    local network="$(lsof -nP -a -p "$PID" -i 2>/dev/null || true)"

    kill "$PID" 2>/dev/null || true
    for _ in {1..20}; do
        kill -0 "$PID" 2>/dev/null || break
        sleep 0.1
    done
    PID=""
    defaults delete "$DOMAIN" >/dev/null 2>&1 || true

    print "[$label] cpu_percent=$cpu peak_footprint_mib=$footprint rss_mib=$rss"

    local failed="false"
    if (( $(awk -v c="$cpu" -v l="$cpu_ceiling" 'BEGIN { print (c >= l) }') )); then
        print -u2 "[$label] idle CPU $cpu% is at or above the $cpu_ceiling% limit."
        failed="true"
    fi
    if (( $(awk -v f="$footprint" -v l="$FOOTPRINT_LIMIT_MIB" 'BEGIN { print (f >= l) }') )); then
        print -u2 "[$label] peak footprint ${footprint}MiB is at or above the ${FOOTPRINT_LIMIT_MIB}MiB limit."
        failed="true"
    fi
    if [[ -n "$network" ]]; then
        print -u2 "$network"
        print -u2 "[$label] unexpected network socket detected."
        failed="true"
    fi
    [[ "$failed" == "false" ]] || exit 1
}

# Defaults, i.e. what a first run costs. Measures ~0.6% since the render size was
# rebased so a frame is presented without resampling; it was ~1.15% before. The 2%
# ceiling is the contract limit rather than a tight regression alarm -- there is a lot
# of headroom in it now, and tightening it is a deliberate decision, not a tidy-up.
run_phase "default" 2.0

# Both sliders at maximum -- a configuration this check never exercised before, and
# one that was over the limit before the render size was rebased: 2.80% for the size
# alone, 5.50% with the frame rate doubled on top. The maximum is now the old default
# size and measures 2.6% (four runs, +/-0.05 on an idle machine), so the ceiling here
# leaves about a third of headroom: enough to survive some load, tight enough that a
# real regression trips it. The 2% figure is a promise about the shipped default, not
# about every configuration a user can dial in, hence a separate ceiling.
run_phase "max scale and speed" 3.5 "appearance.scale=2.0" "appearance.animationSpeed=2.0"

print "network_sockets=0"

# Deliberately no idle-wakeup budget. top's IDLEW counts only wakeups that brought
# an idle core back up, so on any machine that is doing something else it reads
# near zero -- measured 1 wakeup over 18 seconds while the pet animated at 5.5fps.
# It tracks host load, not our timer rate, and would have been a gate that could
# never fail. Wakeup regressions surface in the CPU delta above, because every
# wakeup here does real work; a cheap-wakeup regression needs powermetrics, which
# needs root and so cannot run unattended.
