# Building dymon

## Linux (native)

```bash
cmake -B build/linux
cmake --build build/linux --parallel
```

Binaries are placed in `build/linux/`.

---

## Windows x64 — cross-compilation from Linux

Cross-compilation uses the MinGW-w64 toolchain installed on Linux to produce native Windows `.exe` files without needing a Windows machine.

### 1. Install the toolchain

On Debian/Ubuntu:

```bash
sudo apt-get install mingw-w64
```

This installs the `x86_64-w64-mingw32-gcc` / `g++` cross-compilers along with the Windows system headers and import libraries.

> **Thread model note:** Ubuntu's `mingw-w64` package ships two thread model variants (`posix` and `win32`). The `posix` variant (default) provides `pthread` support via `libwinpthread`. The build links this statically so the resulting `.exe` files have no external DLL dependencies.

### 2. Configure

```bash
cmake -B build/win64 \
    -DCMAKE_TOOLCHAIN_FILE=cmake/mingw64-linux-cross-toolchain.cmake \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++ -Wl,-Bstatic -lpthread -Wl,-Bdynamic"
```

The `-DCMAKE_EXE_LINKER_FLAGS` ensures `libgcc`, `libstdc++`, and `libwinpthread` are linked statically, so the produced `.exe` files run standalone on any Windows machine.

### 3. Build

```bash
cmake --build build/win64 --parallel
```

Binaries are placed in `build/win64/`:

| Binary | Description |
|---|---|
| `dymon_pbm.exe` | Print PBM bitmap files from the command line |
| `dymon_srv.exe` | HTTP print server with REST API and web UI |
| `txt2pbm.exe` | Convert text to PBM format |

---

## Windows x64 — native build on Windows

Builds natively on Windows using MinGW-w64. CMake and Make must be available on the system.

### 1. Install the toolchain

Download a MinGW-w64 release from the [niXman/mingw-builds-binaries](https://github.com/niXman/mingw-builds-binaries/releases) repository. Choose an `x86_64` UCRT build, for example:

```
x86_64-15.2.0-release-mcf-seh-ucrt-rt_v13-rev0.7z
```

Extract the archive to a directory of your choice, e.g. `C:\mingw64`.

### 2. Configure

**Option A — MinGW on PATH (recommended)**

Add `C:\mingw64\bin` to your `PATH`, then run:

```bat
cmake -B build/win64 -G "MinGW Makefiles"
```

No toolchain file is required when the compilers are on `PATH`.

**Option B — toolchain file (MinGW not on PATH)**

```bat
cmake -B build/win64 -DCMAKE_TOOLCHAIN_FILE=cmake/mingw64-windows-toolchain.cmake
```

If MinGW is installed somewhere other than `C:\mingw64`, set the `MINGW64_ROOT` environment variable before running cmake:

```bat
set MINGW64_ROOT=D:\tools\mingw64
cmake -B build/win64 -DCMAKE_TOOLCHAIN_FILE=cmake/mingw64-windows-toolchain.cmake
```

### 3. Build

```bat
cmake --build build/win64 --parallel
```

Binaries are placed in `build/win64/`:

| Binary | Description |
|---|---|
| `dymon_pbm.exe` | Print PBM bitmap files from the command line |
| `dymon_srv.exe` | HTTP print server with REST API and web UI |
| `txt2pbm.exe` | Convert text to PBM format |

---

## Automated releases

Pushing a version tag triggers the GitHub Actions workflow, which cross-compiles Windows x64 binaries and attaches them as a release asset (`dymon-windows-x64.zip`):

```bash
git tag v1.0.0
git push origin v1.0.0
```
