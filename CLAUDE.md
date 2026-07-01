# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

`dymon` is a C++17 project that prints labels on DYMO LabelWriter printers (Wireless, 450, 550) by speaking a reverse-engineered TCP protocol (documented in `protocol.md`). It builds three executables:

- `dymon_pbm` — print a P4 PBM bitmap file from the command line.
- `dymon_srv` — HTTP print server: REST API (`/labels`, `/pbm`) plus a static web UI for printing via a form.
- `txt2pbm` — convert formatted text into a P4 PBM file.

There are no third-party C/C++ dependencies to install: all libs are vendored under `libs/` (`cjson`, `cpp-httplib`, `argtable3`, `base64`). Fonts are vendored as generated headers under `src/bitmap/fonts/`.

## Build

CMake project (`CMakeLists.txt`, version is the single source of truth via `project(dymon VERSION ...)` → `APP_VERSION`). There is no test suite, linter, or `make test` target.

### Linux

```bash
cmake -B build/linux
cmake --build build/linux --parallel
```

Binaries land in `build/linux/`.

### Windows x64 — cross-compile from Linux

Requires `mingw-w64` (`sudo apt-get install mingw-w64`).

```bash
cmake -B build/win64 \
    -DCMAKE_TOOLCHAIN_FILE=cmake/mingw64-linux-cross-toolchain.cmake \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++ -Wl,-Bstatic -lpthread -Wl,-Bdynamic"
cmake --build build/win64 --parallel
```

### Windows x64 — native build on Windows

With MinGW-w64 on `PATH`:

```bat
cmake -B build/win64 -G "MinGW Makefiles"
cmake --build build/win64 --parallel
```

Binaries land in `build/win64/`. See `build.md` for toolchain file options and automated release details.

## Running locally without hardware

`dymo_wireless_mock.py` listens on TCP 9100 and replies to status requests with 32 zero bytes, emulating a printer. Use it as a `--net` target. Alternatively, run `dymon_srv --dir <DIR>` to write each label to a PBM file instead of printing.

```bash
python3 dymo_wireless_mock.py            # in one terminal
./build/dymon_srv --serve www --net 127.0.0.1   # in another
```

## Architecture

### Transport abstraction (`src/dymon/`)

`Dymon` (`dymon.h`) is an abstract base implementing the protocol state machine (`ping`/`start`/`read_status`/`print`/`end`) and status decoding. The protocol is identical across transports — USB tunnels the same TCP bytes — so only four virtuals (`connect`/`send`/`receive`/`close`) are overridden per backend:

- `DymonNet` — TCP socket to a printer IP. Platform split: `dymon_net_linux.cpp` / `dymon_net_win32.cpp`.
- `DymonUsb` — USB device. Platform split: `dymon_usb_linux.cpp` / `dymon_usb_win32.cpp` (Windows also pulls in `usbprint_win32.c` to resolve a `vid_xxxx` to a device path).
- `DymonFile` (`dymon_file.cpp`) — writes labels as PBM files to a directory instead of printing; the `meta` field becomes a PBM comment.

`CMakeLists.txt` selects the `*_linux.cpp` vs `*_win32.cpp` sources via `if(WIN32)`. **macOS uses the Linux sources.** A `lw450flavor` flag changes status-byte interpretation and session handshake for LabelWriter 450-class printers; this is why `--model 450` is often mandatory.

`Dymon::ping(void *arg)` is implemented in the base class: it calls the virtual `connect` + `close` and returns whether the device was reachable. It has no side effects and works across all backends without per-class overrides (`DymonFile` always returns `true`).

### Bitmap generation (`src/bitmap/`, `src/barcode/`)

`bitmap.cpp` is the 1-bpp framebuffer (with 90° rotation for orientation changes). `bitmap_fromText.cpp` is the shared "text processor": it parses the minimal markup language (font size, alignment, horizontal rules, EAN barcodes — see `pbm.md`) using `glyphIterator` + `utf8decoder` and the vendored FreeSans font headers. **Both `txt2pbm` and `dymon_srv` use this same text processor**, so markup behavior is guaranteed consistent between them.

### Server (`src/main_dymon_srv.cpp`)

Built on `cpp-httplib`. HTTP handlers process requests **synchronously** — they block until the operation completes and return an HTTP status code reflecting the actual outcome:

- `200 OK` — success
- `503 Service Unavailable` — printer unreachable (connect failed)
- `500 Internal Server Error` — printer was reached but print job failed mid-way

The JSON response body mirrors the status: `{"status":"OK"}`, `{"status":"Service Unavailable"}`, or `{"status":"Internal Server Error"}`.

**`POST /labels`** accepts a JSON object or array. `TextLabel::fromJson` renders each label into a bitmap → `m_print_labels` drives `DymonPrinter` serially. If the first object in the array has no `"text"` field it is treated as a **ping/test request**: `DymonPrinter::ping` is called and `200`/`503` returned without printing.

**`POST /pbm`** accepts a raw binary P4 PBM or newline-separated base64 PBMs. If the body contains no valid PBM it is treated as a **ping/test request**: same `200`/`503` behaviour.

`DymonPrinter` (`main_dymon_srv.cpp`) is a thin wrapper around a `Dymon *` that resolves the `device`/`ip` argument: if a fixed device was set at startup (via `--usb`/`--net`/`--dir`), it always takes precedence over the `ip` field in the request.

Backend mode is chosen at startup by mutually-exclusive flags (`--usb` / `--net` / `--dir`); with none, the server is in "distributor" mode and routes each label to the `ip` field from its JSON payload. The web UI is served from `--serve <dir>` (typically `www/`), or a built-in page compiled in from `src/labelwriter_html.c` / `src/wpm_html.c`. All endpoints set permissive CORS headers and there is an OPTIONS catch-all; `/wpm` serves a postMessage proxy page so the REST API is reachable from an HTTPS context.

### Web UI (`www/`)

Static HTML/JS demos (`demo1`, `demo2`, `labelpainter`, `labelwriter`, `_wpm`) that POST to the REST API.

## Key references

- `protocol.md` — the reverse-engineered TCP protocol; `doc/wireshark/` has captures.
- `pbm.md` — text markup format and PBM creation (txt2pbm, GIMP, ImageMagick).
- `doc/dymon_srv.yaml` — OpenAPI description of the REST endpoints.
- `addons/briefmarke/` — Python helper to extract a Deutsche Post Internetmarke datamatrix into a PBM.

## Code style

- **Indentation**: 3 spaces (no tabs).
- **Braces**: every `if`, `else`, `for`, `while` body must use curly braces, with the opening brace on its own line:

```cpp
if (x)
{
   // 3-space indent
}
else
{
   // ...
}
```

Single-line braceless bodies are not allowed, even for trivial cases.

## Making a release

1. Bump the version in `CMakeLists.txt` (`project(dymon VERSION x.y.z)`).
2. Draft release notes: inspect `git log` since the last tag, come up with a proposal for what to include (new features, behaviour changes, fixes, breaking changes), and **ask the user to confirm or amend before proceeding**.
3. Update `CHANGELOG.md` with the agreed release notes.
4. Commit and push both files.
5. Create an annotated tag whose message is the full, verbose release notes (this text becomes the GitHub release body), then push the tag:

```bash
git tag -a vx.y.z -m "$(cat <<'EOF'
## Vx.y.z

### Feature or fix title

Description of what changed and why.

### Another title

Description.
EOF
)"
git push origin vx.y.z
```

Pushing a `v*` tag triggers the GitHub Actions workflow (`.github/workflows/release.yml`), which cross-compiles Windows x64 binaries (`dymon_pbm.exe`, `dymon_srv.exe`, `txt2pbm.exe`), packages them into `dymon-windows-x64.zip`, and publishes a GitHub release with the tag annotation as the release body.

See `release.md` for more details.

## Hardware note

LabelWriter 550 uses NFC-based DRM in genuine label rolls and refuses third-party labels; see the README's "Note on LabelWriter 550".
