# Ollama adapter — Implementation prompt

Context
- Hive agents call a small, C-first inference API (see the interface spec in this folder). Ollama can be used as a local HTTP inference service or via a CLI binary; this adapter should support both.

Goal
- Implement an Ollama adapter for the Hive inference abstraction that prefers HTTP (libcurl) and falls back to invoking the `ollama` CLI when the HTTP endpoint is unavailable.

Constraints
- C-first: adapter code must be usable from C and expose a simple vtable that the core can register.
- Config: accept a single JSON blob from `inf_create(backend_name, config_json)` (host, timeout_ms, api_key optional, model_map).
- Memory: all strings returned to core must be heap-allocated and freed by the caller via `inf_free()`.
- Threading: document whether the adapter supports concurrent calls; core may call concurrently.
- Secrets: do not log API keys; prefer reading them from environment if config permits.

Deliverables (PR-sized)
- `src/inference/adapters/ollama/ollama.h` and `src/inference/adapters/ollama/ollama.c` — the adapter implementation and registration.
- `tests/inference/ollama_contract.c` — contract tests for sync, stream, cancel, and list-models.
- `docs/inference/adapters/ollama.md` — short adapter README and example config JSON.

Acceptance criteria
- `inf_create("ollama", config_json)` initializes and returns a context bound to the Ollama adapter.
- `inf_complete_sync()` returns the full text reply (caller frees via `inf_free`).
- `inf_complete_stream()` invokes the provided `inf_stream_cb_t` with ordered token/fragment chunks and returns a cancel token.
- `inf_cancel()` cancels an in-flight stream and the adapter returns `INF_ERR_CANCELLED`.
- `inf_list_models()` returns a non-empty array when Ollama is reachable.
- Contract tests run under ASAN/UBSAN with no leaks.

Config JSON example
```json
{
  "host": "http://localhost:11434",
  "timeout_ms": 30000,
  "api_key": null,
  "model_map": { "hive:default": "llama-2" }
}
```

Implementation notes & starter snippet
- Use `libcurl` for HTTP; prefer the easy API for sync calls and the multi/perform or streaming read-callbacks for SSE/ndjson streaming.
- Fallback: if `host` is unreachable, try executing the `ollama` binary (configurable path) and speak a small JSON protocol on stdin/stdout.
- Per-context state (`void *handle`) should contain parsed config, a `CURL *` handle pool or mutex, and an error buffer for `inf_last_error()`.

Minimal vtable-style skeleton (concept)
```c
/* conceptual snippet to copy into adapter */
static int ollama_init(const char *config_json, void **out_handle);
static void ollama_shutdown(void *handle);
static int ollama_complete_sync(void *handle, const char *req_json, char **resp_json);
static int ollama_complete_stream(void *handle, const char *req_json,
                                  inf_stream_cb_t cb, void *user, int *cancel_out);
static int ollama_list_models(void *handle, char ***models_out, size_t *count_out);
static int ollama_cancel(void *handle, int cancel_token);
static const char *ollama_last_error(void *handle);

/* register at module init */
/* inf_register_adapter("ollama", &ollama_vtable); */
```

Tests
- Provide a small local test harness that can either hit a locally running Ollama instance (integration) or use a deterministic test double (unit). For CI, use the test double to avoid network reliance.

Notes
- Keep HTTP timeouts configurable per-request; map provider errors to the small INF_* error codes documented in the interface spec.
