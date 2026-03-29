"""
Fix undefined label errors in N64Recomp-generated C files.

N64Recomp generates goto statements to labels that may be in other functions
within the same file. C doesn't allow cross-function gotos. This script
replaces such gotos with return statements.
"""
import os
import re
import sys

def fix_file(filepath):
    """Replace goto to labels not defined in the same function."""
    with open(filepath, 'r') as f:
        content = f.read()

    # Parse functions: find RECOMP_FUNC boundaries
    # Pattern: "RECOMP_FUNC void funcname(...) {" to the matching "}"
    # Simpler: split by function signatures
    lines = content.split('\n')

    # Find function start/end line ranges
    functions = []  # list of (start_line, end_line)
    brace_depth = 0
    func_start = None

    for i, line in enumerate(lines):
        if 'RECOMP_FUNC' in line and '{' in line:
            func_start = i
            brace_depth = 1
            continue
        if func_start is not None:
            brace_depth += line.count('{') - line.count('}')
            if brace_depth <= 0:
                functions.append((func_start, i))
                func_start = None
                brace_depth = 0

    if not functions:
        return 0

    # For each function, find defined labels and referenced labels
    fixes = 0
    new_lines = list(lines)

    for start, end in functions:
        func_lines = lines[start:end + 1]
        func_text = '\n'.join(func_lines)

        # Labels defined in this function (word followed by colon at start of line, not in comments)
        defined = set()
        for line in func_lines:
            stripped = line.strip()
            m = re.match(r'^(\w+)\s*:', stripped)
            if m and not stripped.startswith('//'):
                defined.add(m.group(1))

        # Gotos referenced in this function
        for i in range(start, end + 1):
            m = re.search(r'goto\s+(\w+)\s*;', new_lines[i])
            if m and m.group(1) not in defined:
                label = m.group(1)
                indent = len(new_lines[i]) - len(new_lines[i].lstrip())
                new_lines[i] = ' ' * indent + '// goto ' + label + '; // branch outside function\n' + ' ' * indent + 'return;'
                fixes += 1

    if fixes > 0:
        with open(filepath, 'w') as f:
            f.write('\n'.join(new_lines))

    return fixes

def main():
    funcs_dir = sys.argv[1] if len(sys.argv) > 1 else 'RecompiledFuncs'

    total_fixes = 0
    for filename in sorted(os.listdir(funcs_dir)):
        if filename.endswith('.c'):
            filepath = os.path.join(funcs_dir, filename)
            fixes = fix_file(filepath)
            if fixes > 0:
                print(f'  Fixed {fixes} cross-function goto(s) in {filename}')
                total_fixes += fixes

    print(f'Total: {total_fixes} fixes')

if __name__ == '__main__':
    main()
