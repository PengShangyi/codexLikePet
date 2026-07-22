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

cleanup() {
    if [[ -n "${PID:-}" ]]; then
        kill "$PID" 2>/dev/null || true
        for _ in {1..20}; do
            kill -0 "$PID" 2>/dev/null || break
            sleep 0.1
        done
    fi
    defaults delete com.peng.PotatoRuntimeCheck >/dev/null 2>&1 || true
    cmake -E remove_directory "$HOME/Library/Application Support/PotatoRuntimeCheck"
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

open -n "$APP" --args --potato-runtime-check
PID=""
READY="false"
for _ in {1..50}; do
    PID="$(find_pid | tail -1)"
    if [[ -n "$PID" ]]; then
        INITIAL_RSS="$(ps -p "$PID" -o rss= | tr -d ' ')"
        if [[ "$INITIAL_RSS" == <-> ]] && (( INITIAL_RSS > 1024 )); then
            READY="true"
            break
        fi
    fi
    sleep 0.1
done
if [[ "$READY" != "true" ]]; then
    print -u2 "Potato did not finish launching through Launch Services."
    exit 1
fi
sleep 8
STATE="$(ps -p "$PID" -o state= | tr -d ' ')"
if [[ -z "$STATE" || "$STATE" == Z* ]]; then
    print -u2 "Potato exited during the runtime check."
    exit 1
fi

CPU_TOTAL=0
RSS_KIB=0
for _ in {1..5}; do
    read SAMPLE_CPU SAMPLE_RSS <<< "$(ps -p "$PID" -o %cpu=,rss=)"
    CPU_TOTAL="$(awk -v total="$CPU_TOTAL" -v sample="$SAMPLE_CPU" 'BEGIN { print total + sample }')"
    if (( SAMPLE_RSS > RSS_KIB )); then RSS_KIB="$SAMPLE_RSS"; fi
    sleep 1
done
CPU="$(awk -v total="$CPU_TOTAL" 'BEGIN { printf "%.2f", total / 5.0 }')"
NETWORK="$(lsof -nP -a -p "$PID" -i 2>/dev/null || true)"
print "cpu_percent=$CPU"
print "rss_kib=$RSS_KIB"
if (( CPU >= 2.0 )); then
    print -u2 "Idle CPU is above the 2% acceptance limit."
    exit 1
fi
if (( RSS_KIB >= 160 * 1024 )); then
    print -u2 "Resident memory is above the 160MiB acceptance limit."
    exit 1
fi
if [[ -n "$NETWORK" ]]; then
    print -u2 "$NETWORK"
    print -u2 "Unexpected network socket detected."
    exit 1
fi
print "network_sockets=0"
