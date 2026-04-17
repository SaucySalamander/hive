Title: Tests & Benchmarks (Implementation Prompt)

Goal:
- Provide unit tests and microbenchmarks to validate correctness, memory-safety and performance of the dynamics subsystems (queue, signals, orchestrator).

Test suite suggestions:
- Unit tests (function-level):
  - `test_agent_roles`: create agent, set roles, verify getters, run transitions.
  - `test_bqueue`: push/pop with multiple threads; verify no data loss, then close.
  - `test_signal_pubsub`: publish signals, ensure subscribers receive payloads; verify expiry.
  - `test_spawn_policy`: simulate orchestrator under varying backlog/resource conditions and verify spawn respects `min/max_workers`, `spawn_rate_per_min`, and `spawn_cooldown_s`.
  - `test_retirement`: verify eviction order and that retired agents free role-state and resources.
  - `test_drone_proposal_flow`: simulate drone proposals and quorum voting; verify adoption/no-adoption semantics.

Microbenchmark ideas:
- `bench_signal_throughput`: spawn N publisher threads sending M signals each; measure signals/sec and end-to-end latency to subscriber callback.
- `bench_queue_latency`: measure push->pop latency at various queue capacities and thread counts.
 - `bench_spawn_rate`: simulate many spawn/retire cycles and measure spawn/retire throughput and decision latency under load.

How to run (recommendation):

Enable sanitizers during development:

```bash
make clean
CFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' make
./build/hive --run-tests
```

Deterministic benchmarks:
- Use a seeded PRNG and make benchmark runs reproducible (example: `--benchmark-seed=123456789` or call `srand(123456789)` in harness setup).
- Fix thread counts and affinities where possible to reduce noise; record the seed and thread configuration in the baseline file.

CI pass/fail thresholds and baselines:
- Save benchmark baselines to `tests/benchmarks/baseline.txt` (include seed, CPU, threads, and measured metrics).
- CI requirements: unit tests must pass under ASAN/UBSAN. Performance-sensitive PRs should run the benchmark harness and ensure no regression beyond an acceptable threshold (suggested default: 25% regression threshold vs baseline, configurable per PR).
- Example CI commands:

```bash
CFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' make
./build/hive --run-tests
./build/hive --bench-signal-throughput --benchmark-seed=123456789 | tee tests/benchmarks/last-run.txt
```

Spawn/retire CI example:

```bash
./build/hive --bench-spawn-rate --benchmark-seed=123456789 | tee tests/benchmarks/last-spawn-run.txt
```

Benchmark harness snippet (C) — measure elapsed ns

```c
#include <time.h>
static inline uint64_t now_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ULL + t.tv_nsec;
}
```

Acceptance criteria for tests/benchmarks:
- Unit tests pass under ASAN/UBSAN.
- Benchmarks produce repeatable numbers; document baseline in PR.
