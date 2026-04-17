# Inference backend abstraction — API & interface

Purpose
- Specify a minimal, C-first public API that Hive agents call. Keep the surface area small: create/destroy context, sync completion, streaming completion, list models, cancel, and minimal adapter registration.

Design summary
- Single opaque `inference_ctx_t` represents a runtime context bound to a selected backend.
- A backend adapter implements a small function table (init/shutdown/sync/stream/list-models) and registers itself with the core.
- Requests and responses can be represented as compact C structs for convenience and as JSON for adapter portability.

Core types (conceptual)
- `inference_ctx_t` — opaque handle returned by `inf_create()`.
- `inf_message_t` — single chat/message (role + content).
- `inf_request_t` — model, array of messages, temperature, max_tokens, stream flag, user metadata.
- `inf_stream_cb_t` — callback invoked for streaming chunks.

Memory & ownership rules
- Strings returned by the API are heap-allocated by the callee and must be freed by caller with provided helper (`inf_free()`), or adapters must return strdup'd buffers that callers free with `free()`; choose one pattern and document it.
- All callbacks must not free memory passed to them; the core manages lifetimes.
- API is thread-safe: multiple threads may call completion APIs concurrently on the same context if adapter supports it; adapters must document thread-safety.

Error model
- Small errno-like set: `INF_OK` (0), `INF_ERR` (-1), `INF_ERR_TIMEOUT` (-2), `INF_ERR_CANCELLED` (-3), `INF_ERR_BACKEND` (-4). Adapters map provider errors to these codes and may provide textual detail via `inf_last_error()`.

Proposed C header (minimal example)
```c
#pragma once
#include <stddef.h>

typedef struct inference_ctx inference_ctx_t;

typedef enum { INF_OK = 0, INF_ERR = -1, INF_ERR_TIMEOUT = -2, INF_ERR_CANCELLED = -3 } inf_result_t;

typedef struct { const char *role; const char *content; } inf_message_t;

typedef struct {
  const char *model;
  const inf_message_t *messages;
  size_t n_messages;
  double temperature;
  int max_tokens;
  const char *user;
  int stream; /* 0 or 1 */
} inf_request_t;

typedef void (*inf_stream_cb_t)(const char *chunk, size_t len, void *user);

inference_ctx_t *inf_create(const char *backend_name, const char *config_json);
void inf_destroy(inference_ctx_t *ctx);

int inf_list_models(inference_ctx_t *ctx, char ***models_out, size_t *count_out);
void inf_free_model_list(char **models, size_t count);

int inf_complete_sync(inference_ctx_t *ctx, const inf_request_t *req, char **out_text);
int inf_complete_stream(inference_ctx_t *ctx, const inf_request_t *req, inf_stream_cb_t cb, void *user, int *cancel_token_out);
int inf_cancel(inference_ctx_t *ctx, int cancel_token);

void inf_free(char *ptr);

/* Adapter registration (optional dynamic plugins) */
int inf_register_adapter(const void *adapter_vtable);

/* Retrieve text of last error (thread-local or per-ctx) */
const char *inf_last_error(inference_ctx_t *ctx);
```

Adapter vtable (JSON wire option)
- To keep adapters language-agnostic, core may send/receive JSON for requests/responses to adapters that are external processes. Example request JSON shape:

```json
{
  "model": "gpt-4o-mini",
  "messages": [{"role":"system","content":"You are helpful."},{"role":"user","content":"Hello"}],
  "temperature":0.7,
  "max_tokens":512
}
```

Lifecycle and threading notes
- `inf_create()` initializes the selected adapter (calls adapter init). `inf_destroy()` shuts it down.
- Streaming calls run on adapter-managed threads; `inf_cancel()` signals cancellation.

Compatibility & migration
- Start with this minimal API; keep adapters small and self-contained so new features (function-call responses, multimodal) can be added later by extending request/response JSON.
