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

# --- defaults: read the real default from a freshly-sourced script ---
check "default WIDTH is 272" "272" \
  "$(bash -c "source '${SCRIPT_DIR}/macos-setup.sh'; printf '%s' \"\${WIDTH}\"")"

# --- need_cmd ---
if need_cmd ls; then check "need_cmd finds ls" "0" "0"; else check "need_cmd finds ls" "0" "1"; fi
need_cmd __definitely_not_a_real_cmd__; check "need_cmd missing -> nonzero" "1" "$?"

exit "${fail}"
