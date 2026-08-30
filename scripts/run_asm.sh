#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "usage: $0 path/to/program.s [simulator args...]"
    echo
    echo "example:"
    echo "  $0 asmFiles/store_word.s --dump-regs --dump-memory=0x10:4"
    exit 2
fi

asm_file="$1"
shift

if [[ ! -f "$asm_file" ]]; then
    echo "error: assembly file not found: $asm_file" >&2
    exit 1
fi

if [[ -n "${RISCV_TOOL_PREFIX:-}" ]]; then
    tool_prefix="$RISCV_TOOL_PREFIX"
else
    tool_prefix=""
    for candidate in riscv64-unknown-elf riscv64-elf riscv32-unknown-elf; do
        if command -v "${candidate}-as" >/dev/null 2>&1 &&
           command -v "${candidate}-ld" >/dev/null 2>&1 &&
           command -v "${candidate}-objcopy" >/dev/null 2>&1; then
            tool_prefix="$candidate"
            break
        fi
    done

    if [[ -z "$tool_prefix" ]]; then
        tool_prefix="riscv64-unknown-elf"
    fi
fi

assembler="${RISCV_AS:-${tool_prefix}-as}"
linker="${RISCV_LD:-${tool_prefix}-ld}"
objcopy="${RISCV_OBJCOPY:-${tool_prefix}-objcopy}"

missing_tools=()
for tool in "$assembler" "$linker" "$objcopy"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        missing_tools+=("$tool")
    fi
done

if [[ ${#missing_tools[@]} -ne 0 ]]; then
    echo "error: missing RISC-V toolchain command(s): ${missing_tools[*]}" >&2
    echo >&2
    echo "Install a bare-metal RISC-V GNU toolchain and make sure these are on PATH:" >&2
    echo "  riscv64-unknown-elf-as" >&2
    echo "  riscv64-unknown-elf-ld" >&2
    echo "  riscv64-unknown-elf-objcopy" >&2
    echo >&2
    echo "Homebrew may provide these with the riscv64-elf prefix:" >&2
    echo "  brew install riscv64-elf-binutils" >&2
    echo >&2
    echo "If your tools use a different prefix, run for example:" >&2
    echo "  RISCV_TOOL_PREFIX=riscv64-elf $0 $asm_file --dump-regs" >&2
    exit 127
fi

mkdir -p build/asm

base_name="$(basename "$asm_file")"
program_name="${base_name%.*}"
object_file="build/asm/${program_name}.o"
elf_file="build/asm/${program_name}.elf"
binary_file="build/asm/${program_name}.bin"

echo "Assembling: $asm_file"
"$assembler" -march=rv32i -mabi=ilp32 "$asm_file" -o "$object_file"

echo "Linking:    $elf_file"
"$linker" -m elf32lriscv -T linker/rv32i.ld "$object_file" -o "$elf_file"

echo "Objcopy:    $binary_file"
"$objcopy" -O binary "$elf_file" "$binary_file"

if [[ ! -x ./simulator ]]; then
    make
fi

echo "Running:    ./simulator $binary_file $*"
./simulator "$binary_file" "$@"
