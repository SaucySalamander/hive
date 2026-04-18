# Sapphire adapter — Implementation prompt

Context
- Sapphire is our custom inference engine: https://github.com/SaucySalamander/sapphire. Prefer an FFI/shared-library integration for maximum performance and low-latency streaming. If a shared library is not available on the target system, provide a process-based fallback that uses a framed JSON protocol over stdin/stdout or a unix socket.

Goal
- Implement a high-performance Sapphire adapter that exposes streaming and sync completions to Hive with clear ownership rules and minimal copying.

Constraints
- Memory-safety: do not return pointers into Sapphire-managed memory unless ownership is transferred or explicitly copied; Hive owns all returned strings and frees them with `inf_free()`.
- Performance: avoid unnecessary copies for streaming tokens — call `inf_stream_cb_t` with buffers that are valid for the duration of the callback; copy only when longer-term storage is needed.
- Linking: support both static/dynamic linking (`dlopen`/`dlsym`) and a process-mode when linking isn't possible.

Deliverables (PR-sized)
- `src/inference/adapters/sapphire/sapphire.h` and `src/inference/adapters/sapphire/sapphire.c` — FFI shim + adapter.
- `src/inference/adapters/sapphire/process_fallback.c` — small process-mode fallback (if applicable).
- `tests/inference/sapphire_contract.c` and `bench/sapphire_bench.c` — contract tests + microbenchmark harness.
- `docs/inference/adapters/sapphire.md` — adapter README and linking/build notes.

Acceptance criteria
- `inf_create("sapphire", config_json)` binds to the Sapphire shared lib or starts the process fallback.
- `inf_complete_stream()` delivers incremental tokens with minimal copies and supports cancellation.
- `inf_complete_sync()` returns a single string result owned by the caller (freed via `inf_free`).
- Bench harness measures tokens/sec and reports reasonable throughput compared to a process-based adapter.

Config JSON example
```json
{
  "lib_path": "/usr/local/lib/libsapphire.so",
  "process_cmd": "/opt/sapphire/bin/sapphired",
  "model_map": { "hive:default": "sapphire-base" },
  "num_threads": 4
}
```

Implementation notes & starter snippet
- Preferred path: dynamic-load `libsapphire.so` via `dlopen` and `dlsym` to discover a small, stable ABI:

```c
typedef void *(*sapphire_create_fn)(const char *cfg_json);
typedef int (*sapphire_stream_fn)(void *handle, const char *req_json,
                                 void (*token_cb)(const char *token, size_t len, void *ud),
                                 void *ud);

static int sapphire_init(const char *config_json, void **out_handle) {
  void *dl = dlopen("/usr/local/lib/libsapphire.so", RTLD_NOW);
  if (!dl) return INF_ERR_BACKEND;
  sapphire_create_fn create = dlsym(dl, "sapphire_create");
  sapphire_stream_fn stream = dlsym(dl, "sapphire_stream");
  /* store function pointers and dl handle in adapter state */
  return INF_OK;
}
```

- Fallback path: start the process and speak framed JSON. The process should accept a request and stream token frames; tests should include a fake process that emits reproducible tokens and respects SIGINT for cancellation.

Tests & bench
- Contract tests must validate ordering of tokens, cancellation, memory ownership, and that `inf_list_models()` returns mapped models.
- Run bench/sapphire_bench.c with local lib and process fallback and compare latency and throughput.

Build notes
- If building against the shared lib, add a small Makefile fragment or flags to link `-lsapphire` for the test/bench targets. For dynamic-load path, prefer not adding a hard link dependency.
