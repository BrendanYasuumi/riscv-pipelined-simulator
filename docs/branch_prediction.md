# Branch Prediction

## Purpose

A conditional branch chooses between two possible next PCs. Waiting until EX
to learn the condition leaves the fetch stage without the correct next address,
so the simulator predicts a direction in IF and continues fetching before the
branch resolves.

The default configuration uses a 64-entry Branch History Table. Each entry is
an independent two-bit saturating counter indexed by:

```text
index = (branch PC >> 2) % 64
```

The shift removes the two always-zero alignment bits. Different branch PCs can
map to the same entry; this is normal direct-mapped predictor aliasing.

## Counter States

```text
00  Strongly Not Taken  -> predict not taken
01  Weakly Not Taken    -> predict not taken (initial state)
10  Weakly Taken        -> predict taken
11  Strongly Taken      -> predict taken
```

A taken result increments the counter, stopping at `11`. A not-taken result
decrements it, stopping at `00`. Requiring two opposing outcomes to move from
one strong prediction to the other prevents one unusual iteration from fully
reversing a learned direction.

## Pipeline Operation

In IF, the simulator decodes enough of a conditional branch to calculate its
PC-relative target. The selected predictor chooses either the target or
`PC + 4`, and IF records that predicted next PC in the IF/ID latch.

ID carries the prediction into ID/EX. In EX, forwarded register operands
determine the real branch condition. EX compares the actual next PC with the
saved predicted next PC, trains the counter, and increments the prediction
statistics.

If the addresses differ, EX increments `branch_mispredictions`, redirects the
PC, and clears the younger instruction before it can change architectural
state. A correct prediction continues without a control-hazard bubble.

Direct `jal` and indirect `jalr` instructions are not predicted by this table.
They continue to redirect when resolved in EX; conditional branches are the
only operations included in branch-prediction statistics.

## Compare Predictors

```bash
make run ASM=asmFiles/count_loop.s \
  SIM_ARGS="--max-cycles=1000 --branch-predictor=two-bit"

make run ASM=asmFiles/count_loop.s \
  SIM_ARGS="--max-cycles=1000 --branch-predictor=not-taken"
```

For the current five-iteration loop:

```text
Predictor           Predictions  Mispredictions  Accuracy  Cycles
two-bit                       5               2    60.00%      21
always-not-taken              5               4    20.00%      23
```

Both runs store the same final value. Only their microarchitectural timing and
prediction statistics differ.
