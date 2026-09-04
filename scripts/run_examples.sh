#!/usr/bin/env bash
set -euo pipefail

mkdir -p build/examples

run_example() {
    local name="$1"
    local asm_file="$2"
    local max_cycles="$3"
    local log_file="build/examples/${name}.log"

    if ! ./scripts/run_asm.sh "$asm_file" \
        --dump-written-memory \
        --max-cycles="$max_cycles" \
        >"$log_file" 2>&1; then
        printf "%-22s FAIL\n" "$name"
        echo "  log: $log_file"
        tail -n 40 "$log_file"
        exit 1
    fi

    local cycles
    local retired
    local stalls
    local halted
    local ranges

    cycles="$(awk -F: '/Total cycles:/ { gsub(/^[ \t]+/, "", $2); print $2; exit }' "$log_file")"
    retired="$(awk -F: '/Instructions retired:/ { gsub(/^[ \t]+/, "", $2); print $2; exit }' "$log_file")"
    stalls="$(awk -F: '/Stall cycles:/ { gsub(/^[ \t]+/, "", $2); print $2; exit }' "$log_file")"
    halted="$(awk -F: '/Halted:/ { gsub(/^[ \t]+/, "", $2); print $2; exit }' "$log_file")"
    ranges="$(awk '
        /^  Range \[/ {
            range = $0
            sub(/^  Range /, "", range)
            if (result != "") {
                result = result ", "
            }
            result = result range
        }
        END {
            if (result == "") {
                result = "<none>"
            }
            print result
        }
    ' "$log_file")"

    printf "%-22s halted=%-3s cycles=%-6s retired=%-6s stalls=%-4s\n" \
        "$name" "$halted" "$cycles" "$retired" "$stalls"
    echo "  writes: $ranges"
    echo "  log:    $log_file"
}

echo "Running assembly examples..."
echo

run_example "store_word" "asmFiles/store_word.s" 1000
run_example "load_store" "asmFiles/load_store.s" 1000
run_example "bitwise" "asmFiles/bitwise.s" 1000
run_example "branch_equal" "asmFiles/branch_equal.s" 1000
run_example "branch_demo" "asmFiles/branch_demo.s" 1000
run_example "count_loop" "asmFiles/count_loop.s" 1000
run_example "hazard_demo" "asmFiles/hazard_demo.s" 1000
run_example "forwarding_demo" "asmFiles/forwarding_demo.s" 1000
run_example "instruction_coverage" "asmFiles/instruction_coverage.s" 2000
run_example "control_flow_coverage" "asmFiles/control_flow_coverage.s" 2000
run_example "multiply" "asmFiles/program1.s" 1000
run_example "days_estimate" "asmFiles/program3.s" 10000
run_example "search" "asmFiles/search.s" 10000
run_example "fibonacci" "asmFiles/fibonacci.s" 10000
run_example "merge_sort" "asmFiles/merge_sort.s" 100000

echo
echo "Done. Full logs are in build/examples/."
