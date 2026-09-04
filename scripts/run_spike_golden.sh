#!/usr/bin/env bash
set -euo pipefail

manifest="tests/golden/cases.tsv"
output_dir="build/golden"
linker_script="linker/rv32i_spike.ld"
spike_memory="0x10000:0x10000"
simulator_memory="0x20000"

find_tool_prefix() {
    if [[ -n "${RISCV_TOOL_PREFIX:-}" ]]; then
        echo "$RISCV_TOOL_PREFIX"
        return
    fi

    local candidate
    for candidate in riscv64-unknown-elf riscv64-elf riscv32-unknown-elf; do
        if command -v "${candidate}-as" >/dev/null 2>&1 &&
           command -v "${candidate}-ld" >/dev/null 2>&1 &&
           command -v "${candidate}-objcopy" >/dev/null 2>&1 &&
           command -v "${candidate}-nm" >/dev/null 2>&1; then
            echo "$candidate"
            return
        fi
    done

    echo "riscv64-unknown-elf"
}

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: required command not found: $1" >&2
        exit 127
    fi
}

symbol_address() {
    local elf_file="$1"
    local symbol="$2"
    local address

    address="$($nm_tool -n "$elf_file" | awk -v target="$symbol" \
        '$3 == target { print "0x" $1; exit }')"
    if [[ -z "$address" ]]; then
        echo "error: symbol '$symbol' not found in $elf_file" >&2
        exit 1
    fi
    echo "$address"
}

tool_prefix="$(find_tool_prefix)"
assembler="${RISCV_AS:-${tool_prefix}-as}"
linker="${RISCV_LD:-${tool_prefix}-ld}"
objcopy="${RISCV_OBJCOPY:-${tool_prefix}-objcopy}"
nm_tool="${RISCV_NM:-${tool_prefix}-nm}"

require_command spike
require_command "$assembler"
require_command "$linker"
require_command "$objcopy"
require_command "$nm_tool"
require_command ./simulator
require_command ./spike_state_adapter

mkdir -p "$output_dir"
pass_count=0

while IFS='|' read -r name source halt_symbol memory_specs max_cycles; do
    if [[ -z "$name" || "$name" == \#* ]]; then
        continue
    fi

    case_dir="$output_dir/$name"
    mkdir -p "$case_dir"

    object_file="$case_dir/$name.o"
    elf_file="$case_dir/$name.elf"
    binary_file="$case_dir/$name.bin"
    debug_commands="$case_dir/spike.cmd"
    spike_raw="$case_dir/spike.raw"
    simulator_log="$case_dir/simulator.log"
    simulator_state="$case_dir/simulator-state.json"
    spike_state="$case_dir/spike-state.json"

    printf "Spike golden test: %-22s" "$name"

    "$assembler" -march=rv32i -mabi=ilp32 -mno-relax \
        "$source" -o "$object_file"
    "$linker" -m elf32lriscv --no-relax --no-warn-rwx-segments \
        -T "$linker_script" "$object_file" -o "$elf_file"
    "$objcopy" -O binary "$elf_file" "$binary_file"

    start_address="$(symbol_address "$elf_file" _start)"
    halt_address="$(symbol_address "$elf_file" "$halt_symbol")"

    state_args=()
    adapter_args=()
    memory_commands=()
    IFS=',' read -r -a memory_entries <<< "$memory_specs"
    for entry in "${memory_entries[@]}"; do
        symbol="${entry%%:*}"
        length="${entry#*:}"
        address="$(symbol_address "$elf_file" "$symbol")"

        if (( length <= 0 || length % 4 != 0 )); then
            echo
            echo "error: golden memory lengths must be positive multiples of 4" >&2
            exit 1
        fi

        state_args+=("--state-memory=${address}:${length}")
        adapter_args+=("--memory-range=${address}:${length}")

        for ((offset = 0; offset < length; offset += 4)); do
            memory_commands+=("mem $(printf '%x' $((address + offset)))")
        done
    done

    {
        echo "until pc 0 ${halt_address#0x}"
        echo "reg 0"
        echo "pc 0"
        printf '%s\n' "${memory_commands[@]}"
        echo "q"
    } >"$debug_commands"

    if ! ./simulator "$binary_file" \
        --load-address="$start_address" \
        --memory-size="$simulator_memory" \
        --max-cycles="$max_cycles" \
        --dump-state="$simulator_state" \
        "${state_args[@]}" >"$simulator_log" 2>&1; then
        echo "FAIL"
        echo "  simulator log: $simulator_log"
        tail -n 40 "$simulator_log"
        exit 1
    fi

    if ! spike -d \
        --debug-cmd="$debug_commands" \
        --isa=rv32i \
        --priv=m \
        --pc="$start_address" \
        -m"$spike_memory" \
        --disable-dtb \
        "$elf_file" >"$spike_raw" 2>&1; then
        echo "FAIL"
        echo "  Spike log: $spike_raw"
        tail -n 40 "$spike_raw"
        exit 1
    fi

    ./spike_state_adapter \
        --input="$spike_raw" \
        --output="$spike_state" \
        --halt-pc="$halt_address" \
        "${adapter_args[@]}"

    if cmp -s "$simulator_state" "$spike_state"; then
        echo "PASS (diff = 0)"
        pass_count=$((pass_count + 1))
    else
        echo "FAIL"
        echo "  simulator state: $simulator_state"
        echo "  Spike state:     $spike_state"
        diff -u "$spike_state" "$simulator_state" || true
        exit 1
    fi
done <"$manifest"

echo "Passed ${pass_count} Spike golden test(s)."
