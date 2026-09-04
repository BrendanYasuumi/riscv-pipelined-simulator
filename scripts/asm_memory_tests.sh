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

    printf "ASM memory test: %-22s" "$name"
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

run_case "branch_demo" \
    "asmFiles/branch_demo.s" \
    1000 \
    --expect-memory=0x40:1

run_case "count_loop" \
    "asmFiles/count_loop.s" \
    1000 \
    --expect-memory=0x40:5

run_case "hazard_demo" \
    "asmFiles/hazard_demo.s" \
    1000 \
    --expect-memory=0x40:42

run_case "forwarding_demo" \
    "asmFiles/forwarding_demo.s" \
    1000 \
    --expect-memory=0x40:84

run_case "instruction_coverage" \
    "asmFiles/instruction_coverage.s" \
    2000 \
    --expect-memory=0x400:17 \
    --expect-memory=0x404:7 \
    --expect-memory=0x408:4 \
    --expect-memory=0x40c:13 \
    --expect-memory=0x410:9 \
    --expect-memory=0x414:20 \
    --expect-memory=0x418:3 \
    --expect-memory=0x41c:0xfffffffc \
    --expect-memory=0x420:1 \
    --expect-memory=0x424:0 \
    --expect-memory=0x428:8 \
    --expect-memory=0x42c:13 \
    --expect-memory=0x430:3 \
    --expect-memory=0x434:1 \
    --expect-memory=0x438:1 \
    --expect-memory=0x43c:40 \
    --expect-memory=0x440:0x3ffffffc \
    --expect-memory=0x444:0xfffffffc \
    --expect-memory=0x448:0x12345000 \
    --expect-memory=0x44c:0x200 \
    --expect-memory=0x450:0xffffff80 \
    --expect-memory=0x454:0x80 \
    --expect-memory=0x458:0xffff8001 \
    --expect-memory=0x45c:0x8001 \
    --expect-memory=0x460:0x89abcdef \
    --expect-memory=0x464:0xf0 \
    --expect-memory=0x468:0xfff0 \
    --expect-memory=0x46c:0x89abcdef

run_case "control_flow_coverage" \
    "asmFiles/control_flow_coverage.s" \
    2000 \
    --expect-memory=0x500:255 \
    --expect-memory=0x504:0x80 \
    --expect-memory=0x508:0x94

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
