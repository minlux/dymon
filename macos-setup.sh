#!/usr/bin/env bash
# macos-setup.sh — install deps, build dymon, and run a test print on macOS.
# NOTE: must run on macOS default bash 3.2; use ${arr[@]+"${arr[@]}"} for
# optional-argument arrays under `set -u`.
set -euo pipefail

# --- defaults (overridable via flags) ---
IP=""
WIDTH=272
HEIGHT=252
TEXT=$'DYMO\nmacOS test'
MODEL=""
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${REPO_ROOT}/build"
MOCK_PORT=9100

log()  { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33mwarning:\033[0m %s\n' "$*" >&2; }
err()  { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; }

usage() {
  cat <<'EOF'
Usage: macos-setup.sh [--ip <ADDRESS>] [--model <NUMBER>] [--width <PX>]
                      [--height <PX>] [--text <STRING>] [--build-dir <DIR>] [-h|--help]

Installs build dependencies (Xcode CLT, Homebrew, cmake), builds dymon,
generates a test label, and prints it.

Options:
  --ip <ADDRESS>     Network printer IP. Prints via 'dymon_pbm --net'.
                     If omitted, a bundled mock printer is started on
                     127.0.0.1 and used for a no-hardware smoke test.
  --model <NUMBER>   DYMO model number passed to dymon_pbm (e.g. 450).
  --width <PX>       Test label width in pixels [default: 272].
  --height <PX>      Test label height in pixels [default: 252].
  --text <STRING>    Text rendered on the test label (\n for new lines).
  --build-dir <DIR>  CMake build directory [default: <repo>/build].
  -h, --help         Show this help and exit.
EOF
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --ip)        IP="$2"; shift 2;;
      --model)     MODEL="$2"; shift 2;;
      --width)     WIDTH="$2"; shift 2;;
      --height)    HEIGHT="$2"; shift 2;;
      --text)      TEXT="$2"; shift 2;;
      --build-dir) BUILD_DIR="$2"; shift 2;;
      -h|--help)   usage; exit 0;;
      *)           err "unknown argument: $1"; usage; exit 2;;
    esac
  done
}

need_cmd() { command -v "$1" >/dev/null 2>&1; }

main() {
  parse_args "$@"
  log "Skeleton ready."
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
fi
