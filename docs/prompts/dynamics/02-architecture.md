Title: Orchestration Architecture — Signals, Waggle Dance, Quorum (Implementation Prompt)

Context:
- We need a small, robust runtime subsystem to express ephemeral signals (pheromones), targeted adverts (waggle dance), and quorum decisions.

Goal:
- Design and implement a `signal` subsystem and integrate it into the orchestrator so agents can publish/subscribe to short-lived signals and make quorum-driven choices.

Constraints & Design Principles:
- Signals are short-lived, memory-bounded, and frequently overwritten.
- Use a publish/subscribe pattern with efficient in-memory structures; first pass may use mutex-protected lists/queues.
- Avoid large heap churn; prefer pools or fixed-size buffers for high-frequency signals.

API Sketch (suggested):

`signal.h`

```c
typedef enum { SIGNAL_PHEROMONE, SIGNAL_WAGGLE, SIGNAL_ALARM } signal_type_t;

typedef struct signal_t signal_t;

signal_t *signal_create(signal_type_t type, const void *payload, size_t payload_size);
void signal_publish(signal_t *s);
void signal_subscribe(signal_type_t type, void (*cb)(const signal_t*, void*), void *ctx);
void signal_release(signal_t *s);
```

Integration points:
- Orchestrator: subscribe to `SIGNAL_WAGGLE` and maintain a short list of advertised tasks/resources.
- Agent: publish pheromones when seeking resources; workers respond to high pheromone values.

Signal lifetime & ownership:
- Define explicit TTL semantics for signals and a bounded subscriber set. Signals should carry a `timestamp_ns` and an expiry TTL; the runtime should discard expired signals and prune the pheromone map.
- Suggested, tunable defaults (examples): `PHEROMONE_TTL_MS=5000`, `WAGGLE_TTL_MS=15000`, `SIGNAL_MAX_SUBSCRIBERS=64`, `SIGNAL_POOL_SIZE=1024`.
- Ownership: `signal_publish()` SHOULD copy the provided payload into a pooled signal buffer so subscriber callbacks receive a const pointer valid for the signal's TTL. Subscribers MUST NOT free the payload; `signal_release()` returns the signal to the pool.
- Use a pooled allocator for high-frequency signals to avoid heap churn; expose config knobs to tune pool size and max subscribers.

Quorum decisions:
- Implement quorum as a threshold over positive votes collected in a bounded time window. Provide a helper API: `int quorum_try_decide(quorum_t *q);` that returns true when threshold met.

Quorum tuning:
- Define a configurable time window for vote collection (suggested default: `QUORUM_WINDOW_MS=200`). Express thresholds either as a fixed count or fraction of active voters. Record timing metadata for reproducible tests and tuning.
- Keep the pheromone map bounded (e.g., LRU or expiry-based eviction) to avoid unbounded memory usage.

Deliverables:
- `src/core/signal.h` + `src/core/signal.c` (safe, documented API)
- Orchestrator changes to use signals (small patch)
- Tests for publish/subscribe, signal expiry, and quorum decisions

Performance & Safety Checks:
- Run ASAN/UBSAN.
- Add microbenchmark that publishes N signals/sec and measures latency to delivery.

Clarifying choices for implementer:
- First pass: mutex-protected, pooled signals.
- Second pass: lock-free ring buffer for high-throughput communication.
