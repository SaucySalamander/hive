# Inference backend abstraction — Tests & benchmarks

Contract tests (must exist for every adapter)
- Sync completion: same request => valid response, no leaks.
- Streaming completion: deliver token chunks in order; `done` event ends stream.
- Cancellation: start a stream, cancel, verify adapter returns `INF_ERR_CANCELLED` and resources freed.
- Model listing: `inf_list_models()` returns non-empty for configured adapters.

Integration tests
- End-to-end against mock adapter and one remote adapter (OpenAI or internal staging endpoint). Use recorded fixtures for CI to avoid rate/billing.

Concurrency tests
- Fire N concurrent completion requests (N=10/50/200 depending on infra) and validate responses and resource usage.

Sanitizers & memory
- Run unit tests with ASAN/LSAN/UBSAN to ensure no leaks or UB in the core or adapters.

Benchmarks
- Latency (p50/p95/p99) for sync completions.
- Streaming throughput: tokens/sec consumed and delivered.
- Resource usage: memory footprint and CPU per request.

Example bench harness
- A small benchmark program that loads a backend, sends X requests with fixed prompt, and prints aggregated stats. Use it to compare local vs remote backends.
