# Pokemon Snap: Recompiled

Static recompilation of Pokemon Snap (N64) for PC using [N64Recomp](https://github.com/N64Recomp/N64Recomp).

## Status

**In development - boots to main game loop, not yet rendering graphics.**

### Runtime Progress

| Milestone | Status |
|-----------|--------|
| Build & link | Done |
| ROM loading & RDRAM init | Done |
| BSS clear + entrypoint | Done |
| OS thread creation (6 threads) | Done |
| VI retrace system | Done |
| Audio initialization | Done |
| RSP audio task execution | Done |
| Dynamic overlay loading | Done |
| Scene manager main loop | Done |
| GFX display list submission | In Progress |
| RT64 rendering | Not Started |

### Recompilation Stats

| Metric | Value |
|--------|-------|
| ROM | Pokemon Snap (US) |
| Functions recompiled | ~5,800 |
| Lines of generated C code | ~573,000 |
| Overlay sections | 25 code sections |
| Split function fixes | ~390 auto-patched |

### What Works
- Full game boot sequence through all 6 OS threads
- Scheduler receiving VI retrace interrupts at 60Hz
- Audio subsystem: heap, banks, players, RSP audio tasks
- Dynamic overlay loading (intro, menu scenes) via DMA detection
- Scene manager enters main loop, processes objects
- Controller input polling
- SEH crash handler with N64 address translation
- SDL2 window + RT64 GPU initialization

### Current Blocker
The GTL (Graphics Task List) frame processor blocks waiting for a scheduler
message on queue 0x80049780. The scheduler receives VI retraces but doesn't
dispatch them to the GTL client queue. This is likely a missing connection
in the scheduler's client notification system (`scAddClient` / `scExecuteBlocking`).

### Key Technical Fixes Applied
- **Split function pattern**: The N64Recomp tool incorrectly splits functions
  that lack standard prologues. ~390 functions fixed with automated fallthrough
  calls via Python script.
- **Hardware register stubs**: SP IMEM/DMEM checks, AI_LENGTH register reads
  patched to use runtime equivalents.
- **Dynamic overlay system**: `check_and_load_overlay()` detects code section
  loads during DMA and updates the function map.
- **RSP microcode**: 8 missing indirect jump labels added to aspMain switch table.
- **Mid-function entry points**: Dead code after returns extracted as separate
  functions with overlay table entries.

## Building

### Prerequisites
- CMake 3.20+
- MSVC (C++20) or Clang
- Python 3.10+
- Pokemon Snap (US) ROM as `pokemonsnap_us.z64`

### Build
```bash
git clone --recursive https://github.com/sp00nznet/pokemonsnap.git
cd pokemonsnap
cmake -B build
cmake --build build --config Release
cp pokemonsnap_us.z64 build/bin/Release/
# Copy SDL2.dll from lib/rt64/src/contrib/mupen64plus-win32-deps/SDL2-2.26.3/lib/x64/
build/bin/Release/PokemonSnap.exe
```

## Architecture

Built on the N64Recomp ecosystem:
- **N64Recomp** - Static MIPS-to-C recompiler
- **N64ModernRuntime** - librecomp + ultramodern runtime
- **RT64** - D3D12/Vulkan renderer
- **RmlUi** - UI framework

Decomp reference: [ethteck/pokemonsnap](https://github.com/ethteck/pokemonsnap) (~96% decompiled)

## License

TBD
