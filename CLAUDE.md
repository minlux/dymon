# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

`dymon` is a C++17 project that prints labels on DYMO LabelWriter printers (Wireless, 450, 550) by speaking a reverse-engineered TCP protocol (documented in `protocol.md`). It builds three executables:

- `dymon_pbm` — print a P4 PBM bitmap file from the command line.
- `dymon_srv` — HTTP print server: REST API (`/labels`, `/pbm`) plus a static web UI for printing via a form.
- `txt2pbm` — convert formatted text into a P4 PBM file.

There are no third-party C/C++ dependencies to install: all libs are vendored under `libs/` (`cjson`, `cpp-httplib`, `argtable3`, `base64`). Fonts are vendored as generated headers under `src/bitmap/fonts/`.

## Build

CMake project (`CMakeLists.txt`, version is the single source of truth via `project(dymon VERSION ...)` → `APP_VERSION`).

```bash
# Native build (macOS/Linux). Binaries land in build/.
cmake -B build
cmake --build build --parallel
```

Windows x64 is produced by cross-compiling from Linux with MinGW-w64, or natively with MinGW; both flows and the required linker flags are in `build.md`. Pushing a `v*` git tag triggers `.github/workflows/` to cross-compile and attach `dymon-windows-x64.zip` to a GitHub release.

There is no test suite, linter, or `make test` target.

## Running locally without hardware

`dymo_wireless_mock.py` listens on TCP 9100 and replies to status requests with 32 zero bytes, emulating a printer. Use it as a `--net` target. Alternatively, run `dymon_srv --dir <DIR>` to write each label to a PBM file instead of printing.

```bash
python3 dymo_wireless_mock.py            # in one terminal
./build/dymon_srv --serve www --net 127.0.0.1   # in another
```

## Architecture

### Transport abstraction (`src/dymon/`)

`Dymon` (`dymon.h`) is an abstract base implementing the protocol state machine (`start`/`read_status`/`print`/`end`) and status decoding. The protocol is identical across transports — USB tunnels the same TCP bytes — so only four virtuals (`connect`/`send`/`receive`/`close`) are overridden per backend:

- `DymonNet` — TCP socket to a printer IP. Platform split: `dymon_net_linux.cpp` / `dymon_net_win32.cpp`.
- `DymonUsb` — USB device. Platform split: `dymon_usb_linux.cpp` / `dymon_usb_win32.cpp` (Windows also pulls in `usbprint_win32.c` to resolve a `vid_xxxx` to a device path).
- `DymonFile` (`dymon_file.cpp`) — writes labels as PBM files to a directory instead of printing; the `meta` field becomes a PBM comment.

`CMakeLists.txt` selects the `*_linux.cpp` vs `*_win32.cpp` sources via `if(WIN32)`. **macOS uses the Linux sources.** A `lw450flavor` flag changes status-byte interpretation and session handshake for LabelWriter 450-class printers; this is why `--model 450` is often mandatory.

### Bitmap generation (`src/bitmap/`, `src/barcode/`)

`bitmap.cpp` is the 1-bpp framebuffer (with 90° rotation for orientation changes). `bitmap_fromText.cpp` is the shared "text processor": it parses the minimal markup language (font size, alignment, horizontal rules, EAN barcodes — see `pbm.md`) using `glyphIterator` + `utf8decoder` and the vendored FreeSans font headers. **Both `txt2pbm` and `dymon_srv` use this same text processor**, so markup behavior is guaranteed consistent between them.

### Server (`src/main_dymon_srv.cpp`)

Built on `cpp-httplib`. HTTP handlers do not print directly — they parse/enqueue and return `200` immediately. Two `MessageQueue<T,64>` (`msg_queue.h`) + worker threads drain the queues and drive the single `DymonPrinter` serially:

- `POST /labels` → JSON (single object or array) → `m_LabelQueue` → `TextLabel::fromJson` renders each → print.
- `POST /pbm` → raw binary P4 PBM, or newline-separated base64 PBMs → `m_PbmQueue` → print.

Backend mode is chosen at startup by mutually-exclusive flags (`--usb` / `--net` / `--dir`); with none, the server is in "distributor" mode and routes each label to the `ip` field from its JSON payload. The web UI is served from `--serve <dir>` (typically `www/`), or a built-in page compiled in from `src/labelwriter_html.c` / `src/wpm_html.c`. All endpoints set permissive CORS headers and there is an OPTIONS catch-all; `/wpm` serves a postMessage proxy page so the REST API is reachable from an HTTPS context.

### Web UI (`www/`)

Static HTML/JS demos (`demo1`, `demo2`, `labelpainter`, `labelwriter`, `_wpm`) that POST to the REST API.

## Key references

- `protocol.md` — the reverse-engineered TCP protocol; `doc/wireshark/` has captures.
- `pbm.md` — text markup format and PBM creation (txt2pbm, GIMP, ImageMagick).
- `doc/dymon_srv.yaml` — OpenAPI description of the REST endpoints.
- `addons/briefmarke/` — Python helper to extract a Deutsche Post Internetmarke datamatrix into a PBM.

## Hardware note

LabelWriter 550 uses NFC-based DRM in genuine label rolls and refuses third-party labels; see the README's "Note on LabelWriter 550".
