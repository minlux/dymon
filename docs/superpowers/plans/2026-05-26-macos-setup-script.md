# macOS Setup + Test-Print Script Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Provide a single `macos-setup.sh` script that auto-installs build dependencies, builds dymon, generates a test label, and prints it — defaulting to a no-hardware smoke test against the bundled mock when no printer IP is given.

**Architecture:** One self-contained Bash script at the repo root, written as small sourceable functions guarded by a `BASH_SOURCE`/`$0` check so its pure logic can be unit-tested without running installers. The script orchestrates: dependency install (Xcode CLT, Homebrew, cmake) → CMake build → `txt2pbm` label generation → print. Printing goes to a real network printer via `dymon_pbm --net <IP>` when `--ip` is supplied, otherwise it starts `dymo_wireless_mock.py` (which listens on TCP 9100, the same port `DymonNet::connect` dials) on `127.0.0.1` and prints to it as a build/PBM/protocol smoke test.

**Tech Stack:** Bash (must run on macOS's default bash 3.2), CMake, Homebrew, the project's existing `txt2pbm`/`dymon_pbm` binaries, and `dymo_wireless_mock.py` (Python 3).

---

## Key Constraints (read before starting)

- **macOS ships bash 3.2.** With `set -u`, expanding a possibly-empty array as `"${arr[@]}"` raises "unbound variable". Always use the `${arr[@]+"${arr[@]}"}` idiom for optional-argument arrays. Do not assume bash 4+ features (no associative arrays, no `${var,,}`).
- **No `libusb`, no `make install`.** dymon vendors all libs and defines no install target; the script must only `brew install cmake`.
- **USB is intentionally not used.** macOS uses the Linux USB device-path backend, which does not work with Mac USB devices. The script prints over the network only (real `--net` or the mock).
- **Mock port is fixed at 9100.** `DymonNet::connect` (`src/dymon/dymon_net_linux.cpp:26`) hardcodes port 9100; `dymo_wireless_mock.py` listens on 9100. Do not add a port flag.

## File Structure

- Create: `macos-setup.sh` (repo root) — the entire script: arg parsing, dependency install, build, label generation, print/mock orchestration. One file because it is one linear workflow with shared globals; splitting it would add sourcing complexity for no benefit.
- Create: `tests/test_macos_setup.sh` — plain-bash unit tests for the pure logic (`parse_args`, `select_print_target`, `need_cmd`). Installer/build/print functions are boundary code validated by an actual run, not unit-tested.

Function responsibilities inside `macos-setup.sh`:

| Function | Responsibility | Unit-tested? |
|---|---|---|
| `log`/`warn`/`err` | Colored stdout/stderr output | no |
| `usage` | Print help text | no |
| `parse_args` | Parse CLI flags into globals | yes |
| `need_cmd` | Return 0 if a command exists on PATH | yes |
| `select_print_target` | Decide `net <ip>` vs `mock` from the IP | yes |
| `ensure_xcode_clt` | Install Xcode CLT if missing, wait until present | no (boundary) |
| `ensure_homebrew` | Install Homebrew if missing, load `brew shellenv` | no (boundary) |
| `ensure_cmake` | `brew install cmake` if missing | no (boundary) |
| `build_dymon` | Configure + build with CMake | no (boundary) |
| `make_test_pbm` | Generate the test PBM via `txt2pbm` | no (boundary) |
| `print_to_mock` | Start mock, print to 127.0.0.1, stop mock | no (boundary) |
| `print_label` | Route to `--net` print or `print_to_mock` | no (boundary) |
| `main` | Orchestrate the full sequence | no (boundary) |

---

## Task 1: Script skeleton — defaults, output helpers, usage, arg parsing, source guard

**Files:**
- Create: `macos-setup.sh`
- Test: `tests/test_macos_setup.sh`

- [ ] **Step 1: Write the failing test**

Create `tests/test_macos_setup.sh`:

```bash
#!/usr/bin/env bash
# Unit tests for macos-setup.sh pure logic.
# Sources the script (the source guard prevents main from running),
# then relaxes strict mode so intentional non-zero checks don't abort.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=/dev/null
source "${SCRIPT_DIR}/macos-setup.sh"
set +e +u +o pipefail

fail=0
check() {
  local desc="$1" expected="$2" actual="$3"
  if [[ "${expected}" == "${actual}" ]]; then
    printf 'ok   - %s\n' "${desc}"
  else
    printf 'FAIL - %s\n      expected: %q\n      actual:   %q\n' \
      "${desc}" "${expected}" "${actual}"
    fail=1
  fi
}

# --- parse_args sets globals ---
IP=""; parse_args --ip 9.9.9.9; check "parse --ip sets IP" "9.9.9.9" "${IP}"
WIDTH=272; parse_args --width 100; check "parse --width sets WIDTH" "100" "${WIDTH}"
HEIGHT=252; parse_args --height 50; check "parse --height sets HEIGHT" "50" "${HEIGHT}"
TEXT=""; parse_args --text "hi there"; check "parse --text sets TEXT" "hi there" "${TEXT}"
MODEL=""; parse_args --model 450; check "parse --model sets MODEL" "450" "${MODEL}"

# --- defaults are sane before any parsing override ---
check "default WIDTH is 272" "272" "$(WIDTH=272; printf '%s' "${WIDTH}")"

# --- need_cmd ---
if need_cmd ls; then check "need_cmd finds ls" "0" "0"; else check "need_cmd finds ls" "0" "1"; fi
need_cmd __definitely_not_a_real_cmd__; check "need_cmd missing -> nonzero" "1" "$?"

exit "${fail}"
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `bash tests/test_macos_setup.sh`
Expected: FAIL — `macos-setup.sh` does not exist, so `source` errors with "No such file or directory" and a non-zero exit.

- [ ] **Step 3: Write the minimal script skeleton**

Create `macos-setup.sh`:

```bash
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
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `bash tests/test_macos_setup.sh`
Expected: all lines start `ok`, process exits 0.

- [ ] **Step 5: Lint and commit**

Run (if available): `shellcheck macos-setup.sh tests/test_macos_setup.sh` — fix any warnings; if `shellcheck` is not installed, skip.

```bash
git add macos-setup.sh tests/test_macos_setup.sh
git commit -m "feat: macos-setup.sh skeleton with arg parsing and tests"
```

---

## Task 2: Print-target selection logic

**Files:**
- Modify: `macos-setup.sh` (add `select_print_target` before `main`)
- Test: `tests/test_macos_setup.sh` (add cases)

- [ ] **Step 1: Add the failing test cases**

In `tests/test_macos_setup.sh`, insert before the `exit "${fail}"` line:

```bash
# --- select_print_target ---
check "target with IP -> net <ip>" "net 1.2.3.4" "$(select_print_target 1.2.3.4)"
check "target empty -> mock" "mock" "$(select_print_target "")"
check "target no arg -> mock" "mock" "$(select_print_target)"
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `bash tests/test_macos_setup.sh`
Expected: the three new lines FAIL with `command not found`-style output (`select_print_target` undefined), exit non-zero.

- [ ] **Step 3: Implement `select_print_target`**

In `macos-setup.sh`, add immediately after the `need_cmd` definition:

```bash
# Echo the chosen print target: "net <ip>" when an IP is set, else "mock".
select_print_target() {
  if [[ -n "${1:-}" ]]; then
    printf 'net %s\n' "$1"
  else
    printf 'mock\n'
  fi
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `bash tests/test_macos_setup.sh`
Expected: all `ok`, exit 0.

- [ ] **Step 5: Commit**

```bash
git add macos-setup.sh tests/test_macos_setup.sh
git commit -m "feat: add print-target selection logic"
```

---

## Task 3: Dependency auto-install functions

**Files:**
- Modify: `macos-setup.sh` (add `ensure_xcode_clt`, `ensure_homebrew`, `ensure_cmake`)

These are boundary functions (they shell out to system installers), so there is no unit test; correctness is verified by the end-to-end run in Task 5 and by `shellcheck`.

- [ ] **Step 1: Implement the three ensure_* functions**

In `macos-setup.sh`, add after `select_print_target`:

```bash
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
```

- [ ] **Step 2: Verify the script still parses and tests still pass**

Run: `bash -n macos-setup.sh`
Expected: no output (syntax OK).

Run: `bash tests/test_macos_setup.sh`
Expected: all `ok`, exit 0 (new functions are not exercised by unit tests but must not break sourcing).

- [ ] **Step 3: Lint and commit**

Run (if available): `shellcheck macos-setup.sh` — fix warnings.

```bash
git add macos-setup.sh
git commit -m "feat: auto-install Xcode CLT, Homebrew, and cmake"
```

---

## Task 4: Build and test-label generation

**Files:**
- Modify: `macos-setup.sh` (add `build_dymon`, `make_test_pbm`)

Boundary functions; verified by the Task 5 run.

- [ ] **Step 1: Implement `build_dymon` and `make_test_pbm`**

In `macos-setup.sh`, add after `ensure_cmake`:

```bash
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
```

- [ ] **Step 2: Verify syntax and tests**

Run: `bash -n macos-setup.sh`
Expected: no output.

Run: `bash tests/test_macos_setup.sh`
Expected: all `ok`, exit 0.

- [ ] **Step 3: Commit**

```bash
git add macos-setup.sh
git commit -m "feat: add CMake build and txt2pbm label generation"
```

---

## Task 5: Print/mock orchestration + main wiring + end-to-end run

**Files:**
- Modify: `macos-setup.sh` (add `print_to_mock`, `print_label`; rewrite `main`)

- [ ] **Step 1: Implement `print_to_mock` and `print_label`**

In `macos-setup.sh`, add after `make_test_pbm`:

```bash
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
  sleep 1

  log "Printing to mock (build + PBM + protocol smoke test)..."
  local rc=0
  "${dymon_pbm}" --net 127.0.0.1 "$@" "${pbm}" || rc=$?

  # Always stop and reap the mock, whether the print succeeded or not.
  # NOTE: a `trap ... RETURN` referencing this local is unbound by the time
  # the trap fires under `set -u` on bash 3.2 (macOS default), so clean up
  # explicitly instead.
  kill "${mock_pid}" 2>/dev/null || true
  wait "${mock_pid}" 2>/dev/null || true

  if [[ "${rc}" -eq 0 ]]; then
    log "Mock print completed — toolchain works end to end."
  fi
  return "${rc}"
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
```

- [ ] **Step 2: Rewrite `main` to run the full sequence**

In `macos-setup.sh`, replace the existing `main` function with:

```bash
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
```

- [ ] **Step 3: Verify syntax and that unit tests still pass**

Run: `bash -n macos-setup.sh`
Expected: no output.

Run: `bash tests/test_macos_setup.sh`
Expected: all `ok`, exit 0.

- [ ] **Step 4: End-to-end smoke test (no hardware)**

Pre-req: dependencies already installed (cmake present), so the ensure_* functions are no-ops on the dev machine.

Run: `bash macos-setup.sh`
Expected, in order:
- build output ending with `txt2pbm`, `dymon_pbm`, `dymon_srv` built
- `==> Generating test label (272x252) ...`
- `==> No --ip given: starting bundled mock printer on 127.0.0.1:9100...`
- `==> Mock print completed — toolchain works end to end.`
- `==> Done.`
- exit code 0, and the mock process is no longer running (`pgrep -f dymo_wireless_mock.py` prints nothing).

If the build dir already exists from prior work, the run should still succeed (CMake reconfigures in place).

- [ ] **Step 5: Lint and commit**

Run (if available): `shellcheck macos-setup.sh` — fix warnings.

```bash
git add macos-setup.sh
git commit -m "feat: wire up print/mock orchestration and full setup flow"
```

---

## Self-Review Checklist (completed by plan author)

- **Spec coverage:** auto-install deps (Task 3) ✓; build (Task 4) ✓; test label (Task 4) ✓; print via `--net` IP (Task 5, `print_label`) ✓; mock fallback when no IP (Task 5, `print_to_mock`) ✓. No USB path, matching the decision. ✓
- **Placeholder scan:** every code step contains complete code; no TBD/TODO. ✓
- **Type/name consistency:** `select_print_target` (Tasks 2,5), `need_cmd` (Tasks 1,3,5), `BUILD_DIR`/`REPO_ROOT`/`MODEL`/`IP` globals used consistently; `model_args` expanded with the bash-3.2-safe idiom everywhere it is used. ✓
- **bash 3.2 safety:** optional array `model_args` always expanded as `${model_args[@]+"${model_args[@]}"}`. ✓
```
