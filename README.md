# Pokemon Snap: Recompiled

Static recompilation of Pokemon Snap (N64) for PC using [N64Recomp](https://github.com/N64Recomp/N64Recomp).

## Status

**Early development - not yet playable.**

### Recompilation Progress

| Metric | Value |
|--------|-------|
| ROM | Pokemon Snap (US) `edc7c49...` |
| Functions in symbol file | 5,167 |
| Functions successfully recompiled | 3,995 (77%) |
| Lines of generated C code | 572,995 |
| Overlay sections | 51 (48 overlay groups) |

### What Works
- ROM binary scanning and function boundary detection
- Symbol generation from decomp project (prologue + JAL target scanning)
- Static recompilation of main game code, level code, and most overlays
- libultra OS functions auto-handled by N64Recomp built-in lists

### Known Issues
- Some overlay boundary mismatches (shared VRAM addresses cause function size conflicts)
- RSP audio microcode not yet recompiled
- No runtime/rendering integration yet

### Planned Enhancements
- Photo export as PNG/JPEG (capture at native rendering resolution)
- Photo metadata embedding (Pokemon ID, position, score, level)
- Mouse/keyboard/gyro camera aiming
- Widescreen support
- Configurable film limit
- Gallery browser enhancements

## Building

### Prerequisites
- CMake 3.20+
- Clang/MSVC (C++20)
- Python 3.10+ with PyYAML
- Pokemon Snap (US) ROM as `pokemonsnap.us.z64`

### Setup
```bash
git clone --recursive https://github.com/sp00nznet/pokemonsnap.git
cd pokemonsnap
cp /path/to/rom pokemonsnap.us.z64

# Generate symbols from ROM
python tools/gen_symbols.py

# Run N64Recomp
N64Recomp pokemonsnap.us.toml
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
