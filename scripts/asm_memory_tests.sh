#!/usr/bin/env bash
set -euo pipefail

mkdir -p build/asm-tests

pass_count=0

run_case() {
    local name="$1"
    local asm_file="$2"
    local max_cycles="$3"
    shift 3

    local log_file="build/asm-tests/${name}.log"

    printf "ASM memory test: %-14s" "$name"
    if ./scripts/run_asm.sh "$asm_file" --max-cycles="$max_cycles" "$@" \
        >"$log_file" 2>&1; then
        echo "PASS"
        pass_count=$((pass_count + 1))
    else
        echo "FAIL"
        echo "  log: $log_file"
        tail -n 40 "$log_file"
        exit 1
    fi
}

run_case "store_word" \
    "asmFiles/store_word.s" \
    1000 \
    --expect-memory=0x10:24

run_case "load_store" \
    "asmFiles/load_store.s" \
    1000 \
    --expect-memory=0x40:42

run_case "bitwise" \
    "asmFiles/bitwise.s" \
    1000 \
    --expect-memory=0x40:8 \
    --expect-memory=0x44:14 \
    --expect-memory=0x48:6

run_case "branch_equal" \
    "asmFiles/branch_equal.s" \
    1000 \
    --expect-memory=0x40:0

run_case "count_loop" \
    "asmFiles/count_loop.s" \
    1000 \
    --expect-memory=0x40:5

run_case "multiply" \
    "asmFiles/program1.s" \
    1000 \
    --expect-memory=0x40:42

run_case "days_estimate" \
    "asmFiles/program3.s" \
    10000 \
    --expect-memory=0x40:9357

run_case "search" \
    "asmFiles/search.s" \
    10000 \
    --expect-memory=0x80:0x124

run_case "fibonacci" \
    "asmFiles/fibonacci.s" \
    10000 \
    --expect-memory=0xdc:0x6ff1

run_case "merge_sort" \
    "asmFiles/merge_sort.s" \
    100000 \
    --expect-memory=0x500:1 \
    --expect-memory=0x504:3 \
    --expect-memory=0x5f8:100 \
    --expect-memory=0x5fc:100

echo "Passed ${pass_count} ASM memory tests."
