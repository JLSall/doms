# Latest optimization review — 2026-10-21

Keep linear width estimation, direct one-word component sorting, and tighter
dominance-target scanning: each improved all four requested metrics in every
comparison round. Provisionally keep deferred hash growth, whose median gain
was useful but inconsistent. Completed-index release is optional for runtime:
its small gain was inconsistent and overlaps the observed variation.

None of these changes is required for correctness. This review evaluates their
performance value; it does not remove or alter any solver optimization.

## Scope and method

Reviewed the five optimizations in the working tree after commit
`1ff1b8d69407c698df834c9149c48b3f74c9741e`. Earlier committed optimizations remain
enabled in every build. Per-board CSV timing instrumentation remains identical
in every build, including the baseline.

- Every invocation solved **10,000 Expert boards**, 30 × 16 with 99 mines.
- Seed **20261021**, automatic ordering, count-only mode, **12 worker threads**.
- Seven variants: all five enabled, each disabled separately with the other
  four enabled, and all five disabled.
- Three runs per variant, **21 runs / 210,000 board solves**. These repeat the
  same 10,000 distinct boards, rather than sampling 210,000 distinct boards.
- Run order shuffled deterministically within each round; no solver runs or
  compilation jobs from this review overlapped.
- MSYS2 UCRT64 GCC 16.1.0, identical
  `-std=c++17 -O3 -DNDEBUG -pthread` flags, without native CPU tuning.
- The all-enabled generated source was checked against the working source
  byte-for-byte after newline normalization. Each disabled variant restores
  the relevant original source hunks; source snapshots and the diff are saved.

Total runtime below is the solver's bulk elapsed time, including generation,
worker setup, solving, and CSV writing. Full process elapsed time is also
recorded in the JSON. Median/p5/p95 are individual board elapsed solve times,
covering model construction and solving under concurrent worker load; they
exclude queue waiting and are not single-thread latency measurements.
Percentiles use linear interpolation. Each table cell is the median of that
metric across the three runs, rather than a percentile of pooled runs.

## Results

Lower is better. The percent column compares each variant's median total
runtime with the all-enabled median; positive means reverting made it slower.

| Configuration | Total runtime (s) | Median (ms) | p5 (ms) | p95 (ms) | Total change |
| --- | ---: | ---: | ---: | ---: | ---: |
| All five enabled | 39.121 | 36.018 | 15.584 | 84.126 | reference |
| Revert width estimation | 44.707 | 42.189 | 20.035 | 93.520 | +14.28% |
| Revert one-word sorting | 41.922 | 38.518 | 16.260 | 91.110 | +7.16% |
| Revert deferred hash growth | 41.743 | 38.015 | 16.118 | 91.415 | +6.70% |
| Revert dominance-target scanning | 41.639 | 37.668 | 15.754 | 90.977 | +6.44% |
| Revert completed-index release | 39.818 | 36.375 | 15.630 | 87.325 | +1.78% |
| All five reverted | 47.630 | 44.222 | 20.158 | 100.566 | +21.75% |

Together, the five changes reduced median total runtime **17.86%** versus the
all-five-reverted baseline. Individual effects are conditional on the other
four changes and must not be added together.

## Individual judgments

1. **Linear width estimation — keep.** Replace repeated interval scans with
   endpoint deltas and prefix sums while preserving every interval's cut
   contribution. Reverting cost 12.40–14.28% total runtime across rounds and
   worsened median, p5, and p95 in all three. This has the largest demonstrated
   benefit and a straightforward equivalence argument. Explicit plain sweep
   orders that bypass this estimator are outside this benchmark.

2. **Direct one-word connectivity sorting — keep.** Sorting contiguous
   one-word components produces the same canonical multiset while avoiding
   indirect offset sorting and copying. Wider signatures retain their original
   path. Reverting cost 6.86–7.97% total runtime and worsened all three
   per-board metrics in every round.

3. **Live dominance targets and tighter maxima — keep.** Previously dominated
   states remain available as witnesses, but are removed from the target scan.
   Stable compaction preserves key order; maxima over the remaining targets
   retain the necessary feasibility inequalities. Reverting cost 5.36–6.44%
   total runtime and worsened all three per-board metrics in every round.

4. **Deferred hash-table growth — provisional keep; medium confidence.**
   Probing before reserving insertion capacity avoids growing for duplicate
   states. New insertions still enforce the load limit, and tombstone reuse
   does not increase occupied-slot count. Median total runtime favors it by
   the equivalent of a 6.70% reversion penalty, with favorable median/p5/p95
   summaries. However, round 2 favored reverting it by 5.75%, while rounds 1
   and 3 favored retaining it by 6.86% and 8.83%. This is useful evidence, not
   a consistently reproduced benefit or proof that the change is necessary.

5. **Completed-layer hash-index release — optional; low confidence for
   runtime.** After merging, layers are only pruned and iterated. Releasing
   indexes of at least 64 KiB permits erasure by dense entry index; reinsertion
   rebuilds the lookup index. Median reversion penalty was only 1.78%; round
   penalties were +1.93%, +2.68%, and −1.09%. This is the first candidate to
   remove if reducing implementation complexity is the priority. Its reduction
   in live hash storage is real by construction, but peak memory was not
   measured here and should not be presented as a measured memory saving.

The third and fourth changes add bookkeeping and control flow; their benefit
depends on the mix of dominance targets and duplicate successor states. The
index-release threshold also depends on allocation behavior. None changes the
optimization objective or introduces a search limit.

## Variation and validation

Total elapsed seconds in execution rounds 1, 2, and 3:

| Configuration | Round 1 | Round 2 | Round 3 |
| --- | ---: | ---: | ---: |
| All enabled | 39.064 | 40.341 | 39.121 |
| Revert width estimation | 44.352 | 45.345 | 44.707 |
| Revert one-word sorting | 41.922 | 43.555 | 41.803 |
| Revert deferred growth | 41.743 | 38.022 | 42.575 |
| Revert dominance targets | 41.493 | 42.502 | 41.639 |
| Revert index release | 39.818 | 41.422 | 38.693 |
| All reverted | 47.630 | 49.012 | 44.920 |

All 21 CSVs contained exactly 10,000 rows. Every board encoding, 3BV value,
and optimal click count matched across all variants and repeats. These runs
validate this seeded Expert workload in count-only mode; they do not establish
correctness for every board, replay mode, or the wider-mask specializations.
The prior optimization plan records broader validation already performed.

Three repetitions on one machine and one seeded workload are not enough to
claim statistical significance for small gains. Rounds were shuffled rather
than adjacent A/B pairs, so within-round percentage comparisons can still be
affected by machine load and timing drift. The contradictory deferred-growth
and index-release results are retained in the report and raw measurements.

## Reproduction and artifacts

Run from the repository root while the reviewed source and HEAD diff are
present:

```powershell
python benchmarks\review_latest_optimizations.py
```

The harness deliberately recognizes only the reviewed source regions and
rejects an unexpected diff. It leaves the working solver and existing
`doms.exe` untouched.

[Machine-readable results](optimization_review_results.json) contain compiler
flags, source hash, run order, all 21 measurements, and summary metrics.
[The benchmark harness](review_latest_optimizations.py) regenerates each variant,
builds it, runs the measurements, and checks matching results.

Full sources, executables, build logs, 21 raw CSVs, and stderr logs are saved in
`C:\Users\happy\AppData\Local\Temp\doms-review-20261021`.
