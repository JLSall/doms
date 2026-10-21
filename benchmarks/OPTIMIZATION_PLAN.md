# Optimization history

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
