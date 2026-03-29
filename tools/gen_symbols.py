"""
Generate N64Recomp function symbols TOML from the Pokemon Snap ROM.

Strategy:
1. Parse splat.yaml to find code subsegment ranges (c, asm, hasm types)
2. Only scan those ranges for MIPS function prologues (addiu $sp, $sp, -XX)
3. Use named symbols from symbol_addrs.txt for labeling
4. Exclude RSP microcode ranges
5. Track cop0 functions for stubbing
"""
import struct, yaml, re, os

PROJECT_PATH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECOMP_PATH = os.path.join(PROJECT_PATH, "lib", "pokemonsnap-decomp")
ROM_PATH = os.path.join(PROJECT_PATH, "pokemonsnap.us.z64")
OUTPUT_PATH = os.path.join(PROJECT_PATH, "PokemonSnapSyms", "dump.toml")

with open(ROM_PATH, 'rb') as f:
    rom = f.read()

def is_addiu_sp_neg(word):
    op = (word >> 26) & 0x3F
    rs = (word >> 21) & 0x1F
    rt = (word >> 16) & 0x1F
    imm = word & 0xFFFF
    return op == 0x09 and rs == 29 and rt == 29 and (imm & 0x8000) != 0

def func_has_cop0(rom_data, rom_offset, size):
    for off in range(rom_offset, min(rom_offset + size, len(rom_data)), 4):
        if off + 4 > len(rom_data):
            break
        word = struct.unpack('>I', rom_data[off:off + 4])[0]
        if (word >> 26) & 0x3F == 0x10:  # COP0
            return True
    return False

# VRAM ranges to exclude from function scanning
RSP_EXCLUDE = [
    (0x800332A0, 0x80033A30),  # Exception handler / OS asm (branches outside function bounds)
    (0x8003D8B0, 0x80045670),  # RSP boot + text + data blobs
]

def in_rsp(addr):
    for s, e in RSP_EXCLUDE:
        if s <= addr < e:
            return True
    return False

# Parse splat.yaml
with open(os.path.join(DECOMP_PATH, 'splat.yaml'), 'r') as f:
    data = yaml.safe_load(f)

# Build segment list with code ranges from subsegments
segments = []
all_rom_starts = set()

for seg in data['segments']:
    if isinstance(seg, list):
        all_rom_starts.add(seg[0])
    elif isinstance(seg, dict):
        rom_start = seg.get('start', 0)
        all_rom_starts.add(rom_start)
        if seg.get('type') != 'code':
            continue

        # Parse subsegments to identify code vs data ranges
        code_ranges = []  # (rom_start, rom_end) for code-only subsegments
        all_subs = []
        for sub in seg.get('subsegments', []):
            if isinstance(sub, list) and len(sub) >= 2 and sub[0] != 'auto':
                all_subs.append({'offset': sub[0], 'type': sub[1],
                                'name': sub[2] if len(sub) >= 3 else None})

        all_subs.sort(key=lambda x: x['offset'])
        CODE_TYPES = {'c', 'asm', 'hasm'}

        for i, sub in enumerate(all_subs):
            if sub['type'] in CODE_TYPES:
                sub_start = sub['offset']
                sub_end = all_subs[i + 1]['offset'] if i + 1 < len(all_subs) else None
                code_ranges.append((sub_start, sub_end))

        segments.append({
            'name': seg.get('name', 'unnamed_%X' % rom_start),
            'rom_start': rom_start,
            'vram': seg.get('vram', 0),
            'bss_size': seg.get('bss_size', 0),
            'code_ranges': code_ranges,
        })

all_rom_starts = sorted(all_rom_starts)
for seg in segments:
    for i, r in enumerate(all_rom_starts):
        if r == seg['rom_start']:
            seg['rom_end'] = all_rom_starts[i + 1] if i + 1 < len(all_rom_starts) else 0x1000000
            break
    seg['rom_size'] = seg['rom_end'] - seg['rom_start']
    # Fix code ranges that have None end
    fixed = []
    for start, end in seg['code_ranges']:
        if end is None:
            end = seg['rom_end']
        fixed.append((start, end))
    seg['code_ranges'] = fixed

# Load symbol names
named_syms = {}
with open(os.path.join(DECOMP_PATH, 'tools', 'symbol_addrs.txt'), 'r') as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        m = re.match(r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+)', line)
        if m:
            addr = int(m.group(2), 16)
            if 0x80000000 <= addr < 0x81000000:
                named_syms[addr] = m.group(1)

# GLOBAL PASS 1: Collect all JAL targets across ALL segments
global_jal_targets = set()
for seg in segments:
    rom_start_s = seg['rom_start']
    vram_base_s = seg['vram']
    for cr_start, cr_end in seg['code_ranges']:
        cr_end = min(cr_end, len(rom))
        for off in range(cr_start, cr_end - 4, 4):
            word = struct.unpack('>I', rom[off:off + 4])[0]
            op = (word >> 26) & 0x3F
            if op == 0x03:  # JAL
                target = (word & 0x03FFFFFF) << 2
                target |= (vram_base_s & 0xF0000000)
                if not in_rsp(target):
                    global_jal_targets.add(target)

# GLOBAL PASS 2: Build code VRAM sets per segment for JAL target validation
for seg in segments:
    rom_start_s = seg['rom_start']
    vram_base_s = seg['vram']
    code_vrams = set()
    for cr_start, cr_end in seg['code_ranges']:
        v_start = vram_base_s + (cr_start - rom_start_s)
        v_end = vram_base_s + (min(cr_end, len(rom)) - rom_start_s)
        for v in range(v_start, v_end, 4):
            code_vrams.add(v)
    seg['_code_vrams'] = code_vrams

# Generate symbols
total_funcs = 0
cop0_funcs = []
out = []
out.append('# Pokemon Snap (US) - Function Symbols for N64Recomp')
out.append('# Auto-generated from ROM prologue scan + JAL targets + decomp symbols')
out.append('')

for seg in segments:
    rom_start = seg['rom_start']
    rom_end = seg['rom_end']
    vram_base = seg['vram']

    if rom_start >= len(rom):
        continue

    # Phase 1: Collect prologue-based function starts
    prologue_starts = set()
    for cr_start, cr_end in seg['code_ranges']:
        cr_end = min(cr_end, len(rom))
        if cr_start < cr_end:
            vram = vram_base + (cr_start - rom_start)
            if not in_rsp(vram):
                prologue_starts.add(vram)
        for off in range(cr_start, cr_end - 4, 4):
            word = struct.unpack('>I', rom[off:off + 4])[0]
            if is_addiu_sp_neg(word):
                vram = vram_base + (off - rom_start)
                if not in_rsp(vram):
                    prologue_starts.add(vram)

    # Phase 2: Add global JAL targets that land in this segment's code
    func_starts = set(prologue_starts)
    code_vrams = seg['_code_vrams']

    for target in global_jal_targets:
        if target in code_vrams and target not in func_starts:
            # Verify it's not a jr $ra (would mean data, not code)
            t_rom = rom_start + (target - vram_base)
            if 0 <= t_rom < len(rom) - 4:
                word = struct.unpack('>I', rom[t_rom:t_rom + 4])[0]
                if word == 0x03E00008:  # jr $ra at function start = stub/nop, skip
                    continue
            func_starts.add(target)

    func_starts = sorted(func_starts)
    if not func_starts:
        continue

    # Build code address set for size calculation
    code_vram_ranges = []
    for cr_start, cr_end in seg['code_ranges']:
        v_start = vram_base + (cr_start - rom_start)
        v_end = vram_base + (cr_end - rom_start)
        code_vram_ranges.append((v_start, v_end))

    def next_boundary(vram):
        """Find the end of the current code range or next function."""
        for v_start, v_end in code_vram_ranges:
            if v_start <= vram < v_end:
                return v_end
        return vram_base + seg['rom_size']

    functions = []
    for i, vram in enumerate(func_starts):
        if i + 1 < len(func_starts):
            # Size is to next function, but capped at end of current code range
            next_func = func_starts[i + 1]
            boundary = next_boundary(vram)
            size = min(next_func, boundary) - vram
        else:
            size = next_boundary(vram) - vram

        if size <= 0:
            continue

        # Use named symbol if available, otherwise generate unique name with section prefix
        if vram in named_syms:
            name = named_syms[vram]
        else:
            name = '%s_func_%08X' % (seg['name'], vram)
        functions.append((name, vram, size))

        # Check for cop0
        rom_off = rom_start + (vram - vram_base)
        if func_has_cop0(rom, rom_off, size):
            cop0_funcs.append(name)

    # Post-process: merge functions that branch outside their boundaries
    # Check each function for backward branches (to addresses before function start)
    merged = True
    while merged:
        merged = False
        new_functions = []
        skip_next = False
        for i, (name, vram, size) in enumerate(functions):
            if skip_next:
                skip_next = False
                continue
            rom_off = rom_start + (vram - vram_base)
            branches_outside = False
            for off in range(rom_off, min(rom_off + size, len(rom)) - 4, 4):
                word = struct.unpack('>I', rom[off:off + 4])[0]
                op = (word >> 26) & 0x3F
                if op in (0x04, 0x05, 0x06, 0x07, 0x01):  # BEQ, BNE, BLEZ, BGTZ, REGIMM
                    imm = word & 0xFFFF
                    if imm & 0x8000:
                        imm -= 0x10000
                    branch_target = (off - rom_start + vram_base) + 4 + (imm << 2)
                    if branch_target < vram or branch_target >= vram + size:
                        branches_outside = True
                        break
            if branches_outside:
                if branch_target < vram and len(new_functions) > 0:
                    # Backward branch - merge with previous function
                    prev_name, prev_vram, prev_size = new_functions[-1]
                    new_size = vram + size - prev_vram
                    new_functions[-1] = (prev_name, prev_vram, new_size)
                    merged = True
                elif branch_target >= vram + size and i + 1 < len(functions):
                    # Forward branch past end - extend to include next function
                    next_name, next_vram, next_size = functions[i + 1]
                    new_size = next_vram + next_size - vram
                    new_functions.append((name, vram, new_size))
                    skip_next = True
                    merged = True
                else:
                    new_functions.append((name, vram, size))
            else:
                new_functions.append((name, vram, size))
        functions = new_functions

    if not functions:
        continue

    total_funcs += len(functions)

    out.append('[[section]]')
    out.append('name = "%s"' % seg['name'])
    out.append('rom = 0x%08X' % rom_start)
    out.append('vram = 0x%08X' % vram_base)
    out.append('size = 0x%X' % seg['rom_size'])
    out.append('')
    out.append('functions = [')
    for name, vram, size in functions:
        out.append('    { name = "%s", vram = 0x%08X, size = 0x%X },' % (name, vram, size))
    out.append(']')
    out.append('')

with open(OUTPUT_PATH, 'w') as f:
    f.write('\n'.join(out))

# Write cop0 stubs list
cop0_path = os.path.join(PROJECT_PATH, "PokemonSnapSyms", "cop0_stubs.txt")
with open(cop0_path, 'w') as f:
    for name in cop0_funcs:
        f.write(name + '\n')

# Write TOML-formatted stubs for easy copy-paste
stubs_toml_path = os.path.join(PROJECT_PATH, "PokemonSnapSyms", "cop0_stubs_toml.txt")
with open(stubs_toml_path, 'w') as f:
    f.write('stubs = [\n')
    for name in sorted(set(cop0_funcs)):
        f.write('    "%s",\n' % name)
    f.write(']\n')

print('Generated %d function entries across %d sections' % (total_funcs, len([s for s in segments if s.get('rom_size', 0) > 0])))
print('Found %d functions with cop0 instructions' % len(cop0_funcs))
