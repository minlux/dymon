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

# Echo the chosen print target: "net <ip>" when an IP is set, else "mock".
select_print_target() {
  if [[ -n "${1:-}" ]]; then
    printf 'net %s\n' "$1"
  else
    printf 'mock\n'
  fi
}

ensure_xcode_clt() {
  if xcode-select -p >/dev/null 2>&1; then
    log "Xcode Command Line Tools already installed."
    return 0
  fi
  log "Installing Xcode Command Line Tools (a GUI prompt may appear)..."
  xcode-select --install || true
  log "Waiting for Xcode Command Line Tools to finish installing..."
  until xcode-select -p >/dev/null 2>&1; do
    sleep 5
  done
  log "Xcode Command Line Tools installed."
}

ensure_homebrew() {
  if need_cmd brew; then
    log "Homebrew already installed."
  else
    log "Installing Homebrew..."
    NONINTERACTIVE=1 /bin/bash -c \
      "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  fi
  # Put brew on PATH for this session (Apple Silicon vs Intel locations).
  if [[ -x /opt/homebrew/bin/brew ]]; then
    eval "$(/opt/homebrew/bin/brew shellenv)"
  elif [[ -x /usr/local/bin/brew ]]; then
    eval "$(/usr/local/bin/brew shellenv)"
  fi
}

ensure_cmake() {
  if need_cmd cmake; then
    log "cmake already installed."
    return 0
  fi
  log "Installing cmake..."
  brew install cmake
}

build_dymon() {
  log "Configuring and building dymon in ${BUILD_DIR}..."
  cmake -B "${BUILD_DIR}" -S "${REPO_ROOT}"
  cmake --build "${BUILD_DIR}" --parallel
}

make_test_pbm() {
  local out="$1"
  local txt2pbm="${BUILD_DIR}/txt2pbm"
  if [[ ! -x "${txt2pbm}" ]]; then
    err "txt2pbm not found at ${txt2pbm} (did the build succeed?)"
    return 1
  fi
  log "Generating test label (${WIDTH}x${HEIGHT}) at ${out}..."
  printf '%b\n' "${TEXT}" | "${txt2pbm}" -w "${WIDTH}" -h "${HEIGHT}" -o "${out}"
}

print_to_mock() {
  local pbm="$1"; shift
  local dymon_pbm="$1"; shift
  # remaining args ("$@") are optional model args
  local mock="${REPO_ROOT}/dymo_wireless_mock.py"

  if ! need_cmd python3; then
    err "python3 is required for the mock smoke test. Install it or pass --ip."
    return 1
  fi
  if [[ ! -f "${mock}" ]]; then
    err "mock not found at ${mock}"
    return 1
  fi

  log "No --ip given: starting bundled mock printer on 127.0.0.1:${MOCK_PORT}..."
  python3 "${mock}" &
  local mock_pid=$!
  trap 'kill "${mock_pid}" 2>/dev/null || true' RETURN
  sleep 1

  log "Printing to mock (build + PBM + protocol smoke test)..."
  "${dymon_pbm}" --net 127.0.0.1 "$@" "${pbm}"
  log "Mock print completed — toolchain works end to end."
}

print_label() {
  local pbm="$1"
  local dymon_pbm="${BUILD_DIR}/dymon_pbm"
  if [[ ! -x "${dymon_pbm}" ]]; then
    err "dymon_pbm not found at ${dymon_pbm} (did the build succeed?)"
    return 1
  fi

  # Build optional model args as an array; expand bash-3.2-safely below.
  local model_args=()
  if [[ -n "${MODEL}" ]]; then
    model_args=(--model "${MODEL}")
  fi

  local target
  target="$(select_print_target "${IP}")"
  if [[ "${target}" == net* ]]; then
    log "Printing to network printer ${IP}..."
    "${dymon_pbm}" --net "${IP}" ${model_args[@]+"${model_args[@]}"} "${pbm}"
    log "Print job sent to ${IP}."
  else
    print_to_mock "${pbm}" "${dymon_pbm}" ${model_args[@]+"${model_args[@]}"}
  fi
}

main() {
  parse_args "$@"
  ensure_xcode_clt
  ensure_homebrew
  ensure_cmake
  build_dymon
  local pbm="${BUILD_DIR}/macos-test.pbm"
  make_test_pbm "${pbm}"
  print_label "${pbm}"
  log "Done."
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
fi
