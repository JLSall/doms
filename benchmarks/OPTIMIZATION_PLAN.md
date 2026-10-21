# Optimization history

## Next optimization: combine crossing-count subset transforms

Smart line ordering subtracts three transformed scope histograms when scoring
each partial-line cut. A scope confined to the current line contributes to
both the selected-mask and complement-mask sums. Insert that scope into both
the before and after histograms before transforming them. By linearity, two
transforms now give exactly the same crossing counts as the previous three.
The two crossing-count calls per eligible line use four transforms instead of
six; the separate old-neighbor transform is unchanged. One temporary integer
array per crossing-count call is also eliminated.

Every crossing score, ordering tie break, chosen order, and subsequent search
is preserved. The objective, pruning certificates, and search limits are
unchanged. Existing workspace changes were included in the baseline snapshot
and preserved.

### Validation

Both builds used MSYS2 UCRT64 GCC 16.1.0 with identical
`-std=c++17 -O3 -DNDEBUG -pthread -Wall -Wextra -Wpedantic` flags.
The changed build compiled without diagnostics.

- A temporary harness extracted the actual baseline smart-order function and
  compared it with the changed function on 300 seeded boards (100 per standard
  difficulty), before and after candidate reduction. All 2,400 complete orders
  matched across row/column orientations and both sweep directions. This also
  exercised unreduced lines near the 20-candidate subset limit.
- All non-timing JSON fields matched in 36 baseline/changed comparisons:
  three difficulties; seeds 1, 42, 100, 2026, 123456, and 8675309; both solve
  modes. All 18 changed replays passed built-in legal-action simulation.
- Four complete `--benchmark` invocations validated 12,000 solves over the
  same 3,000 distinct boards against unchanged board, 3BV, and optimal-click
  references. Every run passed.

### Benchmark results

Runs were sequential in baseline/changed/changed/baseline order. Compilation
and other solver jobs from this session did not overlap these timing runs.
Each invocation ran three fixed 1,000-board Expert sets on one thread in
count-only mode. Each cell below is total seconds for one fixed set:

| Fixed set | Baseline 1 | Changed 1 | Changed 2 | Baseline 2 |
| --- | ---: | ---: | ---: | ---: |
| 1 | 20.591916 | 19.980619 | 20.003988 | 20.132177 |
| 2 | 19.418958 | 19.350945 | 18.986695 | 19.440555 |
| 3 | 18.780016 | 18.733552 | 18.125754 | 18.936448 |
| Total | 58.790890 | 58.065116 | 57.116437 | 58.509180 |

Mean total time decreased from **58.650035 s to 57.590777 s**, a **1.8%**
reduction. Both pairs improved (1.2% and 2.4%), as did all six corresponding
set totals. This is a modest local gain from two repetitions on one machine;
some individual differences are small and machine activity affects timings.
Memory consumption was not measured. Raw total/median/p5/p95 measurements
are recorded in `benchmarks/crossing_transform_benchmark.json`.

Source snapshots, binaries, the full-order differential harness, comparison
scripts, solver JSON outputs, and raw benchmark logs are saved under
`C:\Users\happy\AppData\Local\Temp\doms-crossing-20261021`.
`doms.exe` was updated from the measured changed build; its previous version
is saved there as `original-doms.exe`. The installed executable matched the
measured build by SHA-256 and passed an Expert replay smoke check.
`git diff --check` passed.

## Goal and evidence

Reduce solve time while preserving the exact optimal click count and valid
optimal replays. Keep all existing pruning certificates and the DP search.

The C++ solver already merges equivalent states, reduces candidates, caches
connectivity transitions, and applies three exact dominance passes. Baseline
profiles of Expert seeds 1, 42, 100, 2026, 123456, and 8675309 show dominance
taking approximately as much time as transitions. In the compensated
connectivity pass, a signature's target list still includes states removed by
the forward pass or earlier compensated comparisons. Signature feasibility
checks also use maxima that include those removed targets.

## Implementation

1. Build a sorted list of still-undominated targets for each target signature.
2. Compute maximum target cost, quasi score, and primary key from that list.
   Use the existing necessary inequalities with these tighter maxima before
   computing connectivity relations.
3. After a successful comparison, compact the target list and refresh its
   maxima. Stop scanning candidate signatures when the list becomes empty.
4. Preserve the complete witness lists until the existing deferred erasure.
   Apply the same implementation to count-only/replay and compact/wide masks.

## Why optimality is preserved

Already-dominated states need no additional deletion proof. Filtering them from
the target list changes neither the available witnesses nor any dominance
certificate. Every remaining target is bounded by the refreshed maxima, so
failure of a necessary inequality still proves that the candidate signature
cannot dominate any remaining target. The order of all actual witness checks
and all final pruning decisions stays the same.

## Validation and acceptance

- Compile baseline and changed sources with the same C++17 optimization flags.
- Compare deterministic bulk Expert outputs and representative profiles,
  including processed states and each pruning counter.
- Compare replay and count-only results, using replay's built-in simulation.
- Exercise the wide-mask specialization and exhausted/partly exhausted target
  cases in a focused differential check against the original pruning code.
- Measure repeated sequential bulk runs; record results and limitations here.
  Keep the change only if the measured workloads support a speed benefit.

## Results

Implemented in `prune_dominated` and documented in the README. The maximum-cost
array that was used only by the old compensated pass has also been removed.

Both sources were compiled with MSYS2 UCRT64 GCC 16.1.0 using
`-std=c++17 -O3 -DNDEBUG -pthread -Wall -Wextra -Wpedantic`.
The compiler reports an existing C++20-extension warning about capturing a
structured binding in the transition code; both baseline and changed builds
have that warning.

Four sequential runs per version, alternating execution order, gave these
median solve times on this machine:

| Workload | Baseline | Changed | Time reduction |
| --- | ---: | ---: | ---: |
| 200 Expert boards, seed 8675309, auto order, one thread | 6.0305 s | 5.7435 s | 4.8% |
| Expert seed 100, explicit rows order, count-only | 1.3615 s | 1.2450 s | 8.6% |

The second workload deliberately exercises a larger frontier: 78,659 retained
states at peak. Its compensated-pass median fell from 0.226 s to 0.128 s
(43.4%). These are local measurements with some run-to-run variation, not a
guarantee for every board. The algorithm remains exponential and peak state
counts are unchanged.

Validation completed successfully:

- All 200 bulk board encodings, 3BV values, and optimal click counts matched
  exactly across versions and repeated runs.
- 36 complete solver comparisons covered Beginner, Intermediate, and Expert,
  seeds 1, 42, 100, 2026, 123456, and 8675309, in both count-only and replay
  modes. All non-timing JSON fields and pruning counters matched. The 18 replay
  cases also passed the built-in legal-action simulation.
- A temporary differential harness compared the original and changed pruning
  functions on 2,000 seeded randomized tables. It covered all four combinations
  of compact/wide factor masks and count-only/replay records, plus one through
  three connectivity words, including overlapping reaches. Surviving states,
  costs, chosen histories, and all three pruning counters matched exactly.
- `git diff --check` passed.

Baseline snapshots, validation harnesses, and compiled comparison executables
were kept in the system temporary directory. The existing workspace
`doms.exe`, `compiler_check.cpp`, and `expert-results.csv` were left untouched.
Rebuild the executable from the changed source to use the optimization.

## Next optimization: release completed layer lookup indexes

A fresh profile of Expert seed 100 with `--order rows --count-only` spent
0.704 s in transitions, 0.754 s in dominance, including 0.029 s erasing states.
Each completed layer retained its hash index even though duplicate merging was
finished. Dominance erased by iterator but still rehashed each deleted state
and searched for its hash slot. Later transitions only iterate that layer.

Release indexes of at least 64 KiB after transitions have finished constructing
the layer, before dominance scratch allocations. Erase directly using the iterator's
dense-entry index when the lookup index is absent. Preserve the original
indexed erase path, linear find fallback, alive flags, and wide-storage
cleanup. Rebuild an appropriately sized index if a caller inserts again.
Keep smaller indexes: an initial version releasing every index improved the
large-frontier median by 4.0% but regressed the bulk median by 1.4%, consistent
with allocator reuse mattering more on small layers. The size threshold is
only a storage choice; it places no limit on states or dominance comparisons.

The state entries and iterator order stay identical. This removes only unused
lookup storage and redundant hash work; it changes no cost, connectivity,
branching, merging, dominance certificate, pruning schedule, or search limit.
On the profiled 112,112-state layer the released 262,144-slot index saves
1 MiB during dominance pruning. Other layer sizes have different savings.

Two connectivity-interning alternatives were evaluated first and discarded.
Using try_emplace alone regressed the bulk median by about 1.3%. A compact
connectivity pool showed an initial gain that did not reproduce in a separate
timing pass. Neither alternative remains in the source.

Validation of the retained index-lifecycle optimization:

- 36 baseline/changed full-solver comparisons across three standard
  difficulties, six seeds, and both solve modes matched every non-timing JSON
  field. All 18 replay cases passed the solver's legal-action simulation.
- 100 additional seeded random boards of size 2 through 4 in each dimension
  matched independent Python exhaustive search in both solve modes.
- A focused C++ harness with standard-library assertions checked all four
  combinations of compact/wide masks and count-only/replay records. It covered
  indexed and unindexed erasure, repeated discard and erase, linear find,
  iteration, reinsertion with index rebuilding, complete erasure, and overflow
  storage in six-word masks and seven-word chosen histories. The final harness
  used 12,000 states per specialization to exceed the release threshold.
- All 15 existing Python tests passed. Both C++ versions were compiled with
  GCC 16.1.0 and identical `-std=c++17 -O3 -DNDEBUG -pthread -Wall -Wextra
  -Wpedantic` flags. The existing structured-binding capture warning remains.

Benchmark and validation artifacts are in the system temporary directory under
`doms-opt-20261021`; the baseline snapshot includes all earlier working-tree
optimizations. Timing results below compare against that snapshot.

Four final sequential runs per version, with alternating execution order:

| Workload | Baseline median | Changed median |
| --- | ---: | ---: |
| 200 Expert boards, seed 8675309, auto order, one thread | 5.8586 s | 5.8377 s |
| Expert seed 100, explicit rows order, count-only | 1.3765 s | 1.3733 s |

These end-to-end differences (0.4% and 0.2%) are within observed timing noise;
there is no demonstrated material overall speedup on these workloads. A final
paired profile reduced erasure time from 0.025 s to 0.009 s (64%), with exactly
matching processed/generated states and all pruning counters. The retained
change removes redundant erasure work and lookup memory on large layers, but
dominance comparisons and transitions still dominate total runtime. Larger
overall gains require optimizing those phases. All bulk CSV rows matched.

`doms.exe` was rebuilt from the final source and smoke-tested. Its previous
version is saved as `original-doms.exe` in the temporary artifact directory;
`compiler_check.cpp` and `expert-results.csv` remain untouched.

## Next optimization: linear interval width estimation

### Plan and implementation

Automatic ordering repeatedly calls `width_estimate`, including for partial
sweeps when selecting dynamic bands. The old estimator materialized graph and
factor intervals, then scanned every interval again at every requested cut.
An Expert seed 100 profile attributed about 0.010 s of a roughly 0.038 s solve
to ordering, making this a useful target independent of the DP state search.

Replace interval lists with graph and factor difference arrays. An interval
covering cuts `first` through `last - 1` increments its array at `first` and
decrements it at `last`. Prefix sums give the exact active counts at each cut.
For partial sweeps, accumulate from cut zero but score only the requested cuts.
Keep all maximum calculations, saturated work accumulation, candidate orders,
tie-breaking, pruning rules, and DP behavior unchanged.

For `q` candidates and `I` intervals, the boundary-scoring work changes from
O(q * I) to O(q + I); constructing the interval endpoints still requires the
same graph and scope traversal. Temporary endpoint storage is O(q).

Three earlier experiments in this session were discarded: caching dominance
comparison data, bypassing scalar dominance count filters, and inlining
portable population counts. None showed a consistent overall improvement.
Caching opening-absorption matches helped one large-frontier workload but had
no demonstrated bulk benefit and required extra per-signature storage, so it
was also discarded. Only the interval-estimation change remains.

### Exactness and validation

Each interval contributes one to exactly the same cuts under the old scan and
the difference-array prefix sum. Thus every returned maximum and work estimate
is identical, including partial ranges and saturated work sums. This preserves
the chosen order as well as the exact optimization objective.

- Added `tests/width_estimate.cpp`, which compares 19,000 estimates with the
  original scanning function on 1,000 deterministic randomized models. Cases
  include empty orders/scopes, singleton orders, arbitrary partial ranges,
  dense graphs, and work saturation.
- All non-timing JSON fields matched the session baseline in 36 full-solver
  comparisons: three difficulties, six seeds, and both solve modes. The 18
  replay cases passed the built-in legal-action simulation.
- 100 small random boards matched independent Python exhaustive search in
  both count-only and replay modes.
- All 15 existing Python tests and `git diff --check` passed.
- Both executables used GCC 16.1.0 with identical
  `-std=c++17 -O3 -DNDEBUG -pthread -Wall -Wextra -Wpedantic` flags. The existing
  structured-binding capture extension warning remains.

### Measurements

Four sequential runs per version, alternating execution order, against a
snapshot containing all prior working-tree optimizations:

| Workload | Baseline median | Changed median | Change |
| --- | ---: | ---: | ---: |
| 200 Expert boards, seed 8675309, auto order, one thread | 5.9652 s | 5.2690 s | 11.7% faster |
| Expert seed 100, explicit rows, count-only | 1.4311 s | 1.4535 s | 1.6% slower |

All four bulk runs were faster with the change, and every CSV row matched.
The explicit plain-row control bypasses width estimation altogether; its
small timing difference reflects run-to-run variation in unchanged solve
code. This optimization benefits ordering and does not accelerate DP for an
already specified plain order. Performance remains board-dependent.

Artifacts and baseline snapshots are in the system temporary directory under
`doms-opt-next-20261021`. `doms.exe` was updated from the validated build and
smoke-tested with an Expert replay; its previous version was saved there as
`original-doms.exe`. Existing `compiler_check.cpp` and `expert-results.csv`
were preserved.

## Next optimization: defer DP hash table growth until insertion

`FactorTable::find_or_emplace` previously ensured insertion capacity before every
lookup. In duplicate-heavy layers, that could rehash the table even when the
successor was already present. The lookup now probes first and grows only after
it finds an empty slot for a new state. If a tombstone is available, insertion
reuses it without increasing occupied-slot count or growing the table. This
changes only index capacity and probing; state equality, record updates, and
all DP and dominance decisions are unchanged.

Validation and measurements:

- GCC 16.1.0 rebuilt the C++17 solver with `-O3 -DNDEBUG -pthread`; only the
  existing structured-binding capture extension warning appeared.
- Full replay JSON for Expert seed 100 with row order matched the prior build
  in every non-timing field, including the 121-click optimum.
- All 200 Expert board encodings, 3BV values, and optimal click counts matched
  on seed 8675309.
- A larger sequential bulk comparison on 1,000 Expert boards, seed 20261021,
  took 26.581 s before and 25.582 s after (3.8% faster). Five 200-board paired
  runs had medians of 5.431 s and 5.332 s (1.8% faster); per-run timings varied
  with machine load.
- `git diff --check` passed.

## Next optimization: sort one-word connectivity components directly

### Plan and implementation

Connectivity transitions previously sorted component offsets with an indirect
lexicographic comparator, then copied those components into a canonical word
buffer before interning it. When a component occupies one word, the outgoing
word buffer already contains exactly those components contiguously.

Sort that buffer directly and intern it, skipping the offset sort and canonical
copy. Keep the existing path for signatures with multiple words. This change
applies to both count-only and replay modes and both factor-mask types.

Every retained component contributes exactly one word to the buffer; empty
components have already been removed and charged. Sorting those words produces
the same ordered multiset as sorting offsets by their referenced words. Equal
components retain their multiplicity. Thus the interned signatures, component
closure charges, state insertion order, costs, and all pruning decisions are
identical. No heuristic or search limit is introduced.

A greedy matching shortcut in connectivity dominance was evaluated first and
discarded because initial timings did not support a benefit. It is absent from
the final source. The final timing runs below ran sequentially without other
compilation or validation jobs from this session running concurrently.

### Validation and measurements

Both versions were compiled with MSYS2 UCRT64 GCC 16.1.0 and identical
`-std=c++17 -O3 -DNDEBUG -pthread` flags. The existing structured-binding capture
extension warning remains.

Four runs per version, alternating execution order, against a snapshot of the
source at the start of this session, including all preceding optimizations:

| Workload | Baseline median | Changed median | Time reduction |
| --- | ---: | ---: | ---: |
| 200 Expert boards, seed 8675309, auto order, one thread | 5.4155 s | 5.1850 s | 4.3% |
| Expert seed 100, explicit rows, count-only | 1.4460 s | 1.3340 s | 7.7% |

A separate single-thread run of 500 Expert boards with seed 20261021 took
13.229 s before and 12.668 s after (4.2% faster); all 500 board encodings,
3BV values, and optimal click counts matched.

All four paired runs improved on both workloads. These are local measurements;
performance depends on the board and machine. The optimization saves work for
one-word component signatures; wider signatures retain the original algorithm.

Validation passed:

- All non-timing JSON fields and processed/generated state and pruning counters
  matched in 36 baseline/changed comparisons across three difficulties, six
  seeds, and both modes. The 18 replay cases passed built-in legal-action
  simulation.
- All four 200-board bulk comparisons matched board encodings, 3BV values, and
  optimal click counts. Row-order JSON and pruning counters also matched in
  every timing run.
- 100 small random boards matched the independent Python exhaustive solver in
  both count-only and replay modes.
- A temporary differential C++ harness compared the actual old/new transition
  bodies on 20,000 randomized cases, checking outgoing signatures and closure
  charges for both branches. It covered empty signatures, duplicate and
  overlapping components, slot removal, zero future reach, and one-, two-,
  three-, and seven-word components. Every comparison matched.
- `git diff --check` passed.

Session snapshots, comparison scripts, focused harnesses, and executables are
in the system temporary directory under `doms-matching-20261021`. The initial
executable is saved there as `original-doms.exe`. Existing unrelated workspace
changes were preserved. `doms.exe` was updated from the validated build and
smoke-tested with an Expert replay.

## Next optimization: binary-search one-word opening absorption matches

### Plan and implementation

Opening-chain absorption compares each active opening pattern against every
component in a successor connectivity signature. One-word signatures are
already sorted, so replace that linear component scan with `lower_bound` and
skip any already-removed duplicates at the matching value. Keep the existing
word-by-word linear scan for wider signatures.

The signature is a sorted multiset, and absorption requires exact equality
with the opening's remaining border. Binary search finds the same first equal
component the old scan found; advancing across removed equal entries preserves
the old behavior when signatures or opening borders contain duplicates. Factor
checks, component removals, cost changes, and subsequent interning are
unchanged. This changes transition work only and adds no search limit or
heuristic.

### Validation and measurements

Both versions were compiled with MSYS2 UCRT64 GCC 16.1.0 using
`-std=c++17 -O3 -DNDEBUG -pthread`. The compiler runtime directory must be on
`PATH` when compiling and running these builds.

Four sequential runs, alternating versions, solved the same 200 Expert boards
(seed 8675309, one thread). The median wall times were 5.185 s with binary
search and 5.300 s with the prior scan, a 2.3% reduction. All board encodings,
3BV values, and optimal click counts matched in both paired comparisons. These
measurements show a modest gain on this workload; the effect may vary with
opening absorption frequency and component counts. Wider signatures retain
the original linear scan.

The binaries, isolated prior-scan source, and raw CSV/log files are in the
system temporary directory under `doms-absorption-20261021`. The existing
workspace executable and unrelated files were left untouched.

## Avoid clearing opening-absorption component markers

Each successor that entered opening-chain absorption zero-filled a byte for
every component in its connectivity signature. This happened even when no
opening matched or no component was removed. Replaced that clear with reusable
generation stamps: each transition advances a generation, and a component is
considered removed only when its stamp matches that generation. Matching,
duplicate handling, component removal, and connectivity interning remain the
same. This is scratch bookkeeping only; it does not change any DP state,
cost, pruning certificate, or search limit.

The prior and changed sources were built with MSYS2 UCRT64 GCC 16.1.0 and
identical `-std=c++17 -O3 -DNDEBUG -pthread` flags. Two sequential paired runs
of 100 Expert boards (seed 20261021, one thread) produced identical board
encodings, 3BV values, and optimal click counts in every row. Their total solve
times were 2.609 s before and 2.513 s after in the first pair, then 2.514 s
before and 2.556 s after in the reverse-order pair. The medians were 2.562 s
and 2.535 s respectively, a 1.0% reduction; this is a small gain within likely
run-to-run variation, so treat the speed result as provisional. The changed
build also completed an Expert replay smoke check. `doms.exe` was left
untouched. Build variants and CSV/logs are in the system temporary directory
under `doms-before-stamps` and `stamps-*.csv` / `stamps-*.log`.

## Next optimization: direct one-word connectivity transitions

### Plan and implementation

For connectivity signatures with one 64-bit word per component, construct
successors directly from those words. Previously, even this common case used
the general projection buffer, populated an offset list, and ran per-word
copying and nonempty checks before sorting the resulting words.

The specialized path clears the current candidate's bit, merges touching
components on the selected branch, counts closed components, and sorts and
interns the same component multiset. It preserves duplicate components and
the seed charge when the selected component has no future reach. With a
one-word signature, every assigned connectivity slot is below 64; an absent
slot uses a zero mask. Wider signatures retain the general transition path.
No objective, state representation, pruning rule, order, or search limit changes.

### Validation and measurements

Baseline and changed sources were built with MSYS2 UCRT64 GCC 16.1.0 and
identical `-std=c++17 -O3 -DNDEBUG -pthread` flags. The baseline snapshot
includes all working-tree optimizations present before this change.

- All non-timing JSON fields matched in 36 comparisons across Beginner,
  Intermediate, and Expert boards, six seeds, and both solve modes. The 18
  replay cases completed built-in legal-action validation.
- A temporary C++ harness extracted the actual baseline and changed
  transition bodies and compared both branches on 50,000 randomized cases.
  Cases included duplicate components, empty signatures, no future neighbors,
  absent slots, and slots 0 through 63. All signatures and closure charges
  matched. The harness used standard-library assertions; this toolchain lacks
  the UBSan runtime, so no sanitizer result is claimed.
- Expert seed 100 with explicit rows order and count-only profiling produced
  identical processed/generated state counts and every pruning counter.
  Transition time was 0.546 s before and 0.520 s after in this single pair;
  this phase measurement is illustrative rather than a repeated benchmark.
- Four alternating baseline/changed pairs each solved 500 Expert boards with
  seed 20261021 and one worker. Every board encoding, 3BV value, and optimal
  click count matched in all eight CSVs.

| Pair | Baseline wall time | Changed wall time |
| --- | ---: | ---: |
| 1 | 11.6379 s | 11.4198 s |
| 2 | 11.7907 s | 11.6902 s |
| 3 | 11.9170 s | 11.2489 s |
| 4 | 11.8631 s | 11.6902 s |
| Median | 11.8269 s | 11.5550 s |

The median decreased by 2.3%. These are local measurements on one workload;
ancillary profiling and harness compilation overlapped portions of the run,
so the exact speed gain needs confirmation under controlled machine load.
The change is retained for removing demonstrably redundant transition work
while preserving every compared state and result.

Sources, executables, comparison scripts, the randomized harness, JSON output,
and raw CSV/log files are in the system temporary directory under
`doms-word-transitions-20261021`. `doms.exe` was updated from the changed build
and completed an Expert replay smoke check. Its previous version is saved as
`original-doms.exe` in that directory. Existing unrelated changes were preserved.

## Next optimization: compact connectivity-signature lookup

### Plan and implementation

Replace the connectivity pool's node-based unordered-map index with a compact,
open-addressed array of 32-bit IDs. Store signatures and their cached hashes
in a deque, with a contiguous pointer array for fast ID lookup. This avoids
separate hash-node allocation and repeated hashing during index growth while
preserving references across insertion and rehashing. Grow only for new
signatures, keep the index at most half full, and reset its logical size at
each layer. The index has no tombstones because signatures are never erased
within a layer.

Full vector equality still decides identity, including under hash collisions.
Insertion order still assigns IDs, the empty signature remains ID zero, and
component multiplicities and every signature word are preserved. The state
representation, objective, candidate order, pruning rules, and search remain
unchanged. The ID-capacity check reports resource exhaustion rather than
silently overflowing into the reserved empty-slot value.

### Validation and measurements

Baseline and changed binaries used MSYS2 UCRT64 GCC 16.1.0 with identical
`-std=c++17 -O3 -DNDEBUG -pthread` flags. The baseline snapshot includes every
working-tree optimization present at the beginning of this session. Timing
runs were sequential, alternated execution order, and did not overlap other
compilation or solver jobs from this optimization session.

Four paired runs each solved the same 500 Expert boards, seed 20261021,
automatic order, count-only, one worker:

| Pair | Baseline wall time | Changed wall time |
| --- | ---: | ---: |
| 1 | 11.4183 s | 11.0245 s |
| 2 | 12.5869 s | 11.9345 s |
| 3 | 12.4465 s | 11.4440 s |
| 4 | 12.4614 s | 11.7106 s |
| Median | 12.4539 s | 11.5773 s |

The median time decreased by **7.0%**, with improvement in all four pairs.
Four alternating pairs on Expert seed 100, explicit rows order, count-only
with profiling, had medians of 1.1121 s before and 1.0537 s after, a **5.3%**
reduction. These are local workload measurements; machine activity and board
structure affect the gain. Memory consumption was not measured.

Validation passed:

- 100,000 randomized differential cases against the actual prior pool design,
  including duplicate and wide signatures, reserve, reset, and swapping pools.
- Forced hash collisions, repeated index growth, empty ID zero, and stable
  signature references across insertion and reserve.
- 100 small random boards matched the independent exhaustive Python solver in
  both modes, including 100 built-in legal-action replay validations.
- All non-timing JSON fields matched in 36 baseline/changed comparisons across
  three difficulties, six seeds, and both modes, including 18 replay checks.
- All non-timing JSON fields and every processed/generated state and pruning
  counter matched across eight explicit-row profile runs.
- Board encodings, 3BV values, and optimal counts matched in all eight 500-board
  timing CSVs and in a separate 1,000-board comparison using seed 8675309 and
  12 workers.

The regression test is `benchmarks/test_connectivity_pool.cpp`:

```console
g++ -std=c++17 -O2 -pthread benchmarks/test_connectivity_pool.cpp -o connectivity_pool_test
./connectivity_pool_test
```

Keep assertions enabled for this test. Raw timing values and validation counts
are recorded in `benchmarks/connectivity_pool_results.json`. Source snapshots,
executables, comparison scripts, and raw logs are in the system temporary
directory under `doms-opening-cache-20261021`. Earlier opening-match caching
and single-lookup experiments were discarded; their source snapshots remain
there for reference. Concurrent benchmark-CLI edits in the workspace were
preserved and are included in the final build.

`doms.exe` was rebuilt from the final workspace source, compared with the
measured build on an Expert replay, and smoke-tested after installation.
`git diff --check` passed. The previous executable is saved as
`original-doms.exe` in the session artifact directory.

## Next optimization: contiguous dominance survivor storage

### Plan and implementation

Every dominance pass previously reserved a separate pointer vector for each
connectivity signature. Replace these allocations with one contiguous pointer
buffer and a small list view per signature. The already computed incoming
state counts assign disjoint slices of the buffer. A signature can retain no
more survivors than its incoming states, so its slice cannot overflow.

Appending witnesses preserves the original sorted order. The buffer never
grows during pruning, so pointers remain stable. Exact, coarsening, and
compensated dominance still receive the same witness lists and retain the
same deferred deletion behavior. This changes storage allocation only, with
no change to the objective, search, pruning rules, or optimality guarantees.
The implementation applies to compact and wide masks and both solve modes.

### Benchmark results

Both binaries were built with MSYS2 UCRT64 GCC 16.1.0 using
`-std=c++17 -O3 -DNDEBUG -pthread`, without native CPU tuning. The changed
build also enabled `-Wall -Wextra -Wpedantic` and compiled without diagnostics.
The baseline is a snapshot of the source immediately before this change.

Four complete `--benchmark` invocations ran sequentially in the order
baseline, changed, changed, baseline. No other compilation or solver work
from this session overlapped the timing runs. Every invocation solved the
same three fixed sets of 1,000 Expert boards on one thread in count-only mode.
Every board encoding, 3BV, and optimal count matched the committed references.
These are 12,000 solves over 3,000 distinct boards.

Each cell below is total seconds for one 1,000-board set:

| Fixed set | Baseline 1 | Changed 1 | Changed 2 | Baseline 2 |
| --- | ---: | ---: | ---: | ---: |
| 1 | 21.298796 | 20.572758 | 19.968219 | 22.358321 |
| 2 | 20.609504 | 20.071294 | 19.347290 | 20.583918 |
| 3 | 19.991003 | 19.957048 | 19.406504 | 19.805018 |
| Total | 61.899303 | 60.601100 | 58.722013 | 62.747257 |

Mean total benchmark time decreased from **62.323280 s to 59.661557 s**,
a **4.3%** reduction. The corresponding total reductions in the two pairs
were 2.1% and 6.4%. All six corresponding set totals improved, although one
difference was very small. Two repetitions on one machine support retaining
the change but do not establish a universal or statistically precise gain.
Memory use was not measured.

### Additional validation

- A temporary differential harness compared the actual baseline pruning
  function with the changed function on 4,000 deterministic randomized tables.
  Retained states in iteration order, costs, replay histories, removed counts,
  and all three dominance counters matched. Cases covered compact and
  eight-word factor masks, count-only and replay records with eight-word
  histories, empty/singleton tables, unused signature IDs, and one through
  three connectivity words.
- All non-timing JSON fields matched in 36 complete baseline/changed solver
  comparisons across Beginner, Intermediate, and Expert; seeds 1, 42, 100,
  2026, 123456, and 8675309; and both solve modes. All 18 changed replay
  cases passed built-in legal-action simulation, and count/replay optima
  matched for every board.
- `git diff --check` passed.

Source snapshots, binaries, raw benchmark logs including median/p5/p95,
machine-readable timing results, the differential harness, and full-solver
comparison outputs are saved under
`C:\Users\happy\AppData\Local\Temp\doms-survivors-20261021`.
`doms.exe` was updated from the measured build; its previous version is saved
there as `original-doms.exe`.

## Next optimization: contiguous component-size storage

### Plan and implementation

Dominance checking builds sorted component-size lists for live connectivity
signatures. Replace the separately growing vectors with one integer buffer.
Prefix offsets assign each signature exactly as many entries as it has
components; signatures without surviving states receive an empty slice.
Population counting, per-signature sorting, and indexed comparisons use the
same values in the same order. Minimum and maximum population filters are
unchanged. This removes repeated allocation and growth without changing any
dominance rule, candidate order, objective, or search limit. It applies to
compact and wide masks and both solve modes.

### Validation

- Baseline and changed sources compiled without diagnostics using MSYS2 UCRT64
  GCC 16.1.0 and identical `-std=c++17 -O3 -DNDEBUG -pthread -Wall -Wextra
  -Wpedantic` flags. The baseline includes all working-tree changes present
  before this optimization, including contiguous survivor storage.
- A temporary differential harness extracted the actual baseline pruning
  function and compared it with the changed function on 4,000 deterministic
  randomized tables. Retained states in iteration order, costs, replay
  histories, removed counts, and all three pruning counters matched. Cases
  covered compact and eight-word factor masks, both solve modes, unused IDs,
  empty signatures/tables, and one through three connectivity words with
  nonzero high words.
- All non-timing JSON fields matched in 36 baseline/changed comparisons:
  Beginner, Intermediate, and Expert boards; seeds 1, 42, 100, 2026, 123456,
  and 8675309; and both solve modes. All 18 changed replay cases passed
  built-in legal-action validation, and replay/count-only optima matched.

### Benchmark results

Four complete `--benchmark` invocations ran sequentially in the order
baseline, changed, changed, baseline. Each invocation solved the same three
fixed sets of 1,000 Expert boards on one thread in count-only mode. No other
compilation or solver work from this session overlapped these timing runs.
All 12,000 solves matched the committed board, 3BV, and optimal-click
references. References were unchanged.

Each cell below is total seconds for one 1,000-board set:

| Fixed set | Baseline 1 | Changed 1 | Changed 2 | Baseline 2 |
| --- | ---: | ---: | ---: | ---: |
| 1 | 23.839667 | 22.085089 | 21.446997 | 22.279099 |
| 2 | 22.403302 | 20.549677 | 20.477444 | 21.729241 |
| 3 | 21.272249 | 20.169525 | 20.048479 | 20.756029 |
| Total | 67.515218 | 62.804291 | 61.972920 | 64.764369 |

Mean total benchmark time decreased from **66.139794 s to 62.388606 s**,
a **5.7%** reduction. All six corresponding set totals improved. These are
two repetitions on one machine; the gain depends on workload and machine
activity. Memory consumption was not measured.

Source snapshots, binaries, the differential harness, full-solver comparison
outputs, raw benchmark logs including median/p5/p95, and `results.json` are in
`C:\Users\happy\AppData\Local\Temp\doms-populations-20261021`.
`doms.exe` was updated from the measured changed build, with its previous
version saved there as `original-doms.exe`. An installed-build Expert replay
check and `git diff --check` passed.


## Next optimization: contiguous subset-zeta updates

### Plan and implementation

Smart row/column ordering computes subset sums seven times per eligible line.
The original transform scanned every mask for each bit and branched on whether
that bit was set. Replace it with contiguous blocks: add each block's lower
half into its upper half. This visits precisely the original destination masks
in the same ascending order, with the same source and addition for every bit.
It eliminates the mask branch and enables inner-loop vectorization.

Both crossing-scope counts and old-neighbor counts use `subset_zeta`.
All ordering scores, tie breaks, chosen orders, search states, pruning rules,
and optimality guarantees are preserved. Explicit orders that do not use
smart ordering are unaffected. Existing workspace edits were preserved.

### Validation

- Both builds use MSYS2 UCRT64 GCC 16.1.0 and identical
  `-std=c++17 -O3 -DNDEBUG -pthread -Wall -Wextra -Wpedantic` flags;
  the changed build compiled without diagnostics.
- `benchmarks/subset_zeta_regression.cpp` compares against the original
  transform for every size from zero through 20 bits and 500 randomized
  arrays, including signed entries. Independent direct submask sums cover
  all masks through 10 bits, and closed-form sums cover all sizes through
  the supported 20-bit maximum. Empty input is also checked.
- All non-timing JSON fields matched in 36 complete baseline/changed solver
  comparisons over three difficulties, seeds 1, 42, 100, 2026, 123456, and
  8675309, and both solve modes. All 18 changed replays passed built-in
  legal-action validation; replay/count-only optima matched.
- Four complete `--benchmark` invocations validated 12,000 solves over the
  same 3,000 distinct boards against the unchanged committed board, 3BV,
  and optimal-click references. Every run passed.

Build the regression test with assertions enabled:

```console
g++ -std=c++17 -O2 -pthread benchmarks/subset_zeta_regression.cpp -o subset_zeta_test
./subset_zeta_test
```

### Benchmark results

Runs were sequential in baseline/changed/changed/baseline order, with no
compilation or other solver jobs from this session overlapping the timings.
Each invocation runs the three fixed 1,000-board Expert sets on one thread
in count-only mode. Each cell is total seconds for one fixed set:

| Fixed set | Baseline 1 | Changed 1 | Changed 2 | Baseline 2 |
| --- | ---: | ---: | ---: | ---: |
| 1 | 21.304849 | 20.281375 | 19.679126 | 20.930389 |
| 2 | 19.811962 | 19.066739 | 19.099255 | 20.373338 |
| 3 | 19.159131 | 18.407553 | 18.572140 | 19.826737 |
| Total | 60.275942 | 57.755667 | 57.350521 | 61.130464 |

Mean total time decreased from **60.703203 s to 57.553094 s**, a
**5.2%** reduction. Both pairs improved (4.2% and 6.2%), as did all six
corresponding set totals. Two repetitions on one machine support retaining
this change; they do not establish a universal or statistically precise gain.
Memory consumption was not measured. Raw total/median/p5/p95 measurements
are recorded in `benchmarks/subset_zeta_benchmark.json`.

Two earlier experiments were discarded after full paired benchmarks:
contiguous cached-bucket entries improved mean total time by only 0.9%, with
one pair regressing; scalar one-word union keys regressed mean time by 0.8%.
Both passed differential and reference checks, but neither justified retention.
Their artifacts are under the system temporary directory in
`doms-buckets-20261021` and `doms-union-keys-20261021` respectively.

The retained optimization's source snapshots, binaries, comparison script,
JSON outputs, and raw logs are under `C:\Users\happy\AppData\Local\Temp\doms-zeta-20261021`.
`doms.exe` was updated from the measured changed build; the previous executable
is saved there as `original-doms.exe`.

The installed executable matched the measured build byte-for-byte and passed
an Expert replay smoke check. `git diff --check` passed.


## Next optimization: preclassify scope sweep extents

### Plan and implementation

Smart row/column ordering used to scan every scope's candidates for every
sweep line. Cache each candidate's sweep-line number, then compute each
scope's first and last sweep lines once per ordering invocation.

Scopes strictly outside the current line contribute zero at every internal
cut and can be skipped. Scopes extending to both earlier and later lines
contribute one at every cut, so increment the always-crossing count directly.
Only scopes touching their first or last line need the original detailed scan.
Empty scopes remain ignored. The subset transforms and cut-score DP are
unchanged. Every score, tie break, chosen order, solver state, and pruning rule
is preserved. Both sweep directions and both orientations use the cache;
explicit orders without smart ordering are unaffected. Additional memory is
one integer per candidate and two integers per scope, local to each invocation.
Existing workspace changes, including the CSV reference loader, are preserved.

Three earlier experiments were discarded after four full `--benchmark`
invocations each: combined closed-vertex counts averaged 2.3% slower,
per-cut score caching averaged 2.1% slower, and candidate-line caching alone
averaged 1.2% slower. All 36,000 reference-validated solves passed. Another
`doms.exe` process overlapped part of the score-cache timing runs, so those
measurements do not isolate its performance effect. None was retained alone.
Their source snapshots, tests, and raw logs remain outside the repository
under `doms-live-counts-20261021`, `doms-order-scores-20261021`, and
`doms-cached-lines-20261021` in the system temporary directory.

### Handoff status ? stopped at user request, 2026-10-21

**This is an unfinished experiment, not a measured speed improvement.** The
scope-extents implementation remains in `minesweeper_chord_dp.cpp`, in
`smart_line_order_indices` (candidate `sweep_lines`, `scope_extents`, and the
`crossing_counts` fast classifications). Neither closed-vertex counting nor
per-cut score caching remains in the source. Candidate-line caching remains
only as part of this broader scope-extents experiment.

Completed validation for the current implementation:

- Compiled without diagnostics using MSYS2 UCRT64 GCC 16.1.0 and
  `-std=c++17 -O3 -DNDEBUG -pthread -Wall -Wextra -Wpedantic`.
- 240 complete smart orders matched the unmodified baseline, covering three
  difficulties, ten generated boards each, original and reduced models,
  row and column orientations, and both sweep directions.
- All non-timing JSON fields matched in 36 baseline/changed solver comparisons:
  Beginner, Intermediate, Expert; seeds 1, 42, 100, 2026, 123456, 8675309;
  count-only and replay. All 18 changed replays passed built-in legal-action
  validation.
- The first baseline `--benchmark` had just started when the user requested
  this handoff. It was interrupted. There are **no completed benchmark timing
  results for the scope-extents implementation**. No solver/test process from
  this session remains running.
- This session did not replace the repository's `doms.exe` with an experiment
  build. Do not assume that executable contains the scope-extents change.

Artifacts for the current experiment are in
`C:\Users\happy\AppData\Local\Temp\doms-scope-extents-20261021`:
`baseline.cpp`, `baseline.exe`, `changed.cpp`, `changed.exe`,
`order_differential.cpp`, `order_differential.exe`, `measure.py`, and
`summarize.py`. The current source matched `changed.cpp` byte-for-byte at
handoff. The baseline includes the existing solver optimizations and the
CSV benchmark-reference loader, but excludes candidate-line caching and
scope-extents classification. Both executables use identical compiler flags.

`measure.py` reruns the 36 full-solver comparisons, then executes four complete
`--benchmark` invocations sequentially in baseline/changed/changed/baseline
order, saving logs and `raw_results.json` in that temporary directory.
`summarize.py` writes `results.json` there. Run from the repository root so
`benchmarks/expert_benchmark_reference.csv` can be opened:

```powershell
python "$env:TEMP/doms-scope-extents-20261021/measure.py"
python "$env:TEMP/doms-scope-extents-20261021/summarize.py"
```

Before resuming, inspect `AGENTS.md`, the working tree, and source snapshots:
other work was occurring in this repository during the session. In particular,
the CSV reference migration, README changes, `.gitignore`, `AGENTS.md`, tests,
and existing solver optimizations were not introduced by this experiment and
must be preserved. Do not regenerate reference answers. Keep temporary
scripts, binaries, logs, and result JSON outside version control per `AGENTS.md`.
Avoid overlapping timing runs with other solver or compilation jobs.

Retain the scope-extents change only if further measurements support it.
If rejecting it, revert only the sweep-line cache, extent construction, and
`crossing_counts` changes introduced here; use the temporary baseline for a
focused comparison rather than overwriting concurrent edits. If retaining it,
rebuild `doms.exe` from the final source, run an Expert replay smoke check,
check `git diff --check`, and document the actual benchmark measurements.

For context, the completed rejected experiments had these mean sums of the
three fixed 1,000-board benchmark set totals (two invocations per variant):

| Experiment | Baseline mean seconds | Changed mean seconds | Outcome |
| --- | ---: | ---: | --- |
| Combined closed-vertex counts | 55.065695 | 56.323052 | Discarded; 2.3% slower |
| Cached per-cut scores | 57.976058 | 59.200909 | Discarded; 2.1% slower; concurrent solver activity |
| Cached candidate lines alone | 56.132824 | 56.825385 | Discarded alone; 1.2% slower |

All four full `--benchmark` invocations passed for each experiment: 36,000
validated solves over the same 3,000 distinct Expert boards across the three
experiments. These timings do not measure the current scope-extents change.
The first two experiments used an older baseline executable with compiled-in
references; their changed score-cache build included the concurrent CSV-loader
migration. The candidate-line and scope-extents experiments use the same
CSV-loader code in both variants. This further limits interpretation of the
earlier small timing differences.

### Completed scope-extents benchmark

The scope-extents experiment was resumed and measured sequentially in
baseline/changed/changed/baseline order using the archived binaries and the
CSV reference data. Four complete `--benchmark` invocations passed, validating
12,000 solves across the same 3,000 boards. Mean total time was 55.981259 s
for baseline and 54.697802 s for changed, a 2.3% reduction. Set totals were
faster in all six changed-versus-baseline comparisons. This is a modest local
gain from two repetitions; machine activity can affect timings.

The 36 complete JSON comparisons also passed, including 18 legal replay
simulations, and 240 smart-order comparisons matched the original orders. The
working source matched the measured changed snapshot by SHA-256. Raw runs and
logs are in `C:\Users\happy\AppData\Local\Temp\doms-scope-extents-20261021`.

## Connectivity matching: reject unrepresented fine components early

The inline (up to 64 components) path of `connectivity_coarsens` previously
built the entire eligibility graph before checking whether every fine
component had at least one containing coarse component. Build one fine
component's eligibility column at a time and reject as soon as its column is
empty, matching the existing general path's early rejection strategy.

An empty column makes matching impossible. For comparisons that pass this
check, every eligibility bit is identical to the original graph, and the
matching code and traversal order are unchanged. No pruning certificate,
search limit, state order, objective, or replay choice changes. Earlier working
tree modifications and the benchmark reference data were preserved.

### Validation

- MSYS2 UCRT64 builds used identical `-std=c++17 -O3 -DNDEBUG -pthread
  -Wall -Wextra -Wpedantic` flags and compiled without diagnostics.
- A temporary differential harness compared the actual original function with
  the changed function on 90,000 deterministic randomized signature pairs,
  including 13,334 accepted relationships. It covered one through three words,
  empty signatures, overlapping reaches, constructed containments, and 64- and
  65-component cases. Assertions were enabled; every result matched.
- All non-timing JSON fields matched in 36 full-solver comparisons across
  Beginner, Intermediate, and Expert; seeds 1, 42, 100, 2026, 123456, and
  8675309; count-only and replay modes. All 18 replays passed built-in
  legal-action simulation.
- Four sequential full `--benchmark` invocations passed all fixed board, 3BV,
  and optimal-click references: 12,000 solves over 3,000 distinct Expert boards.

### Measurements

Runs were sequential in baseline/changed/changed/baseline order, without other
solver or compilation jobs from this session overlapping the measurements.
Each cell is the total seconds for one fixed 1,000-board Expert set:

| Fixed set | Baseline 1 | Changed 1 | Changed 2 | Baseline 2 |
| --- | ---: | ---: | ---: | ---: |
| 1 | 18.004070 | 18.404164 | 18.597040 | 18.286924 |
| 2 | 18.462028 | 17.573956 | 17.865946 | 17.630166 |
| 3 | 17.548591 | 16.974183 | 17.304989 | 17.364964 |
| Total | 54.014689 | 52.952303 | 53.767975 | 53.282054 |

Mean total time decreased from **53.648372 s to 53.360139 s (0.5%)**.
One pair improved and the other regressed; this is within observed timing
variation and does **not demonstrate a reliable end-to-end speedup**. The
retained change skips unnecessary graph construction for impossible matches;
larger overall speed gains remain unproven. Memory consumption was not measured.

Source snapshots, binaries, the differential harness, all CLI comparison
outputs, raw results, and benchmark logs are outside version control under
`C:\Users\happy\AppData\Local\Temp\doms-coarsens-early-20261021`.
The measured changed executable was installed as `doms.exe`, checked by SHA-256,
and passed an Expert replay smoke check. Its previous version is archived as
`original-doms.exe`. `git diff --check` passed.
